// Copyright 2016 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "VideoBackends/Vulkan/VideoBackend.h"

#include <vector>

#include "Common/Logging/LogManager.h"
#include "Common/MsgHandler.h"
#include "Common/ScopeGuard.h"

#include "VideoBackends/Vulkan/CommandBufferManager.h"
#include "VideoBackends/Vulkan/Constants.h"
#include "VideoBackends/Vulkan/ObjectCache.h"
#include "VideoBackends/Vulkan/StateTracker.h"
#include "VideoBackends/Vulkan/VKPerfQuery.h"
#include "VideoBackends/Vulkan/VKRenderer.h"
#include "VideoBackends/Vulkan/VKSwapChain.h"
#include "VideoBackends/Vulkan/VKVertexManager.h"
#include "VideoBackends/Vulkan/VulkanContext.h"

#include "VideoCommon/FramebufferManager.h"
#include "VideoCommon/TextureCacheBase.h"
#include "VideoCommon/VideoBackendBase.h"
#include "VideoCommon/VideoConfig.h"

#if defined(VK_USE_PLATFORM_METAL_EXT)
#include <objc/message.h>
#endif

namespace Vulkan
{
void VideoBackend::InitBackendInfo()
{
  WindowSystemInfo temp_wsi;
  temp_wsi.type = WindowSystemType::Headless;
  temp_wsi.enable_surface = false;

  // Temporally acquire context to ensure backend infomation has been populated.
  AcquireContext(temp_wsi);
  ReleaseContext();
}

// Helper method to check whether the Host GPU logging category is enabled.
static bool IsHostGPULoggingEnabled()
{
  return Common::Log::LogManager::GetInstance()->IsEnabled(Common::Log::LogType::HOST_GPU,
                                                           Common::Log::LogLevel::LERROR);
}

// Helper method to determine whether to enable the debug report extension.
static bool ShouldEnableDebugReports(bool enable_validation_layers)
{
  // Enable debug reports if the Host GPU log option is checked, or validation layers are enabled.
  // The only issue here is that if Host GPU is not checked when the instance is created, the debug
  // report extension will not be enabled, requiring the game to be restarted before any reports
  // will be logged. Otherwise, we'd have to enable debug reports on every instance, when most
  // users will never check the Host GPU logging category.
  return enable_validation_layers || IsHostGPULoggingEnabled();
}

bool VideoBackend::AcquireContext(const WindowSystemInfo& wsi)
{
  if (m_context_semaphore++)
    return true;

  Common::ScopeGuard context_guard([=] { ReleaseContext(); });

  if (!LoadVulkanLibrary())
  {
    PanicAlertFmt("Failed to load Vulkan library.");
    return false;
  }

  // Check for presence of the validation layers before trying to enable it
  bool enable_validation_layer = g_Config.bEnableValidationLayer;
  if (enable_validation_layer && !VulkanContext::CheckValidationLayerAvailablility())
  {
    WARN_LOG_FMT(VIDEO, "Validation layer requested but not available, disabling.");
    enable_validation_layer = false;
  }

  // Create Vulkan instance, needed before we can create a surface, or enumerate devices.
  // We use this instance to fill in backend info, then re-use it for the actual device.
  bool enable_surface = wsi.enable_surface;
  bool enable_debug_reports = ShouldEnableDebugReports(enable_validation_layer);
  VkInstance instance =
      VulkanContext::CreateVulkanInstance(wsi, enable_debug_reports, enable_validation_layer);
  if (instance == VK_NULL_HANDLE)
  {
    PanicAlertFmt("Failed to create Vulkan instance.");
    ReleaseContext();
    return false;
  }

  // Load instance function pointers.
  if (!LoadVulkanInstanceFunctions(instance))
  {
    PanicAlertFmt("Failed to load Vulkan instance functions.");
    ReleaseContext();
    return false;
  }

  // Obtain a list of physical devices (GPUs) from the instance.
  // We'll re-use this list later when creating the device.
  VulkanContext::GPUList gpu_list = VulkanContext::EnumerateGPUs(instance);
  if (gpu_list.empty())
  {
    PanicAlertFmt("No Vulkan physical devices available.");
    ReleaseContext();
    return false;
  }

  // Tell the window system about our vulkan instance
  if (wsi.vk_set_instance)
  {
    wsi.vk_set_instance(instance);
  }

  // Populate BackendInfo with as much information as we can at this point.
  VulkanContext::PopulateBackendInfo(&g_Config);
  VulkanContext::PopulateBackendInfoAdapters(&g_Config, gpu_list);

  // We need the surface before we can create a device, as some parameters depend on it.
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  if (enable_surface)
  {
    surface = SwapChain::CreateVulkanSurface(instance, wsi);
    if (surface == VK_NULL_HANDLE)
    {
      PanicAlertFmt("Failed to create Vulkan surface.");
      return false;
    }
  }

  Common::ScopeGuard surface_guard([&, surface] {
    if (surface != VK_NULL_HANDLE)
      SwapChain::DestroyVulkanSurface(instance, wsi, surface);
  });

  // Since we haven't called InitializeShared yet, iAdapter may be out of range,
  // so we have to check it ourselves.
  size_t selected_adapter_index = static_cast<size_t>(g_Config.iAdapter);
  if (selected_adapter_index >= gpu_list.size())
  {
    WARN_LOG_FMT(VIDEO, "Vulkan adapter index out of range, selecting first adapter.");
    selected_adapter_index = 0;
  }

  // Now we can create the Vulkan device. VulkanContext takes ownership of the instance
  g_vulkan_context = VulkanContext::Create(instance, wsi, gpu_list[selected_adapter_index], surface,
                                           enable_debug_reports, enable_validation_layer);
  if (!g_vulkan_context)
  {
    PanicAlertFmt("Failed to create Vulkan device");
    return false;
  }

  if (wsi.vk_set_device)
  {
    wsi.vk_set_device(g_vulkan_context->GetPhysicalDevice(), g_vulkan_context->GetDevice(),
                      g_vulkan_context->GetGraphicsQueueFamilyIndex(), 0);
  }

  // Since VulkanContext maintains a copy of the device features and properties, we can use this
  // to initialize the backend information, so that we don't need to enumerate everything again.
  VulkanContext::PopulateBackendInfoFeatures(&g_Config, g_vulkan_context->GetPhysicalDevice(),
                                             g_vulkan_context->GetDeviceProperties(),
                                             g_vulkan_context->GetDeviceFeatures());
  VulkanContext::PopulateBackendInfoMultisampleModes(
      &g_Config, g_vulkan_context->GetPhysicalDevice(), g_vulkan_context->GetDeviceProperties());
  g_Config.backend_info.bSupportsExclusiveFullscreen =
      enable_surface && g_vulkan_context->SupportsExclusiveFullscreen(wsi, surface);

  // Activate the config, some of the following classes depend on it
  UpdateActiveConfig();

  // Create command buffers. We do this separately because the other classes depend on it.
  g_command_buffer_mgr = std::make_unique<CommandBufferManager>(g_Config.bBackendMultithreading);
  if (!g_command_buffer_mgr->Initialize())
  {
    PanicAlertFmt("Failed to create Vulkan command buffers");
    return false;
  }

  // Remaining classes are also dependent on object cache.
  g_object_cache = std::make_unique<ObjectCache>();
  if (!g_object_cache->Initialize())
  {
    PanicAlertFmt("Failed to initialize Vulkan object cache.");
    return false;
  }

  // Create swap chain. This has to be done early so that the target size is correct for auto-scale.
  if (surface != VK_NULL_HANDLE)
  {
    m_swap_chain = SwapChain::Create(wsi, surface, g_ActiveConfig.bVSyncActive);
    if (!m_swap_chain)
    {
      PanicAlertFmt("Failed to create Vulkan swap chain.");
      return false;
    }

    // SwapChain now has ownership of the surface.
    surface_guard.Dismiss();
  }

  if (!StateTracker::CreateInstance())
  {
    PanicAlertFmt("Failed to create state tracker");
    return false;
  }

  context_guard.Dismiss();
  return true;
}

bool VideoBackend::Initialize(const WindowSystemInfo& wsi)
{
  bool already_active = IsActive();

  // Acquire context, which Initializes it if necessary.
  if (!AcquireContext(wsi))
    return false;

  // With the backend information populated, we can now initialize videocommon.
  InitializeShared();

  if (already_active)
  {
    // Make sure these objects have the latest config
    // TODO: make these listen to config changes even when Renderer isn't initialized
    if (m_swap_chain)
      m_swap_chain->RecreateSwapChain();
    g_object_cache->ReloadPipelineCache();
  }

  // Create main wrapper instances.
  g_renderer = std::make_unique<Renderer>(m_swap_chain.get(), wsi.render_surface_scale);
  g_vertex_manager = std::make_unique<VertexManager>();
  g_shader_cache = std::make_unique<VideoCommon::ShaderCache>();
  g_framebuffer_manager = std::make_unique<FramebufferManager>();
  g_texture_cache = std::make_unique<TextureCacheBase>();
  g_perf_query = std::make_unique<PerfQuery>();

  if (!g_vertex_manager->Initialize() || !g_shader_cache->Initialize() ||
      !g_renderer->Initialize() || !g_framebuffer_manager->Initialize() ||
      !g_texture_cache->Initialize() || !PerfQuery::GetInstance()->Initialize())
  {
    PanicAlertFmt("Failed to initialize renderer classes");
    Shutdown();
    return false;
  }

  g_shader_cache->InitializeShaderCache();
  return true;
}

void VideoBackend::ReleaseContext()
{
  if (--m_context_semaphore)
    return;

  if (g_object_cache)
    g_object_cache->Shutdown();

  // Swapchain holds textures, which reference StateTracker and Command buffer manager, so it needs
  // to be destroyed before them.
  m_swap_chain.reset();

  StateTracker::DestroyInstance();
  g_object_cache.reset();
  g_command_buffer_mgr.reset();
  g_vulkan_context.reset();
  UnloadVulkanLibrary();
}

void VideoBackend::Shutdown()
{
  if (g_vulkan_context)
    vkDeviceWaitIdle(g_vulkan_context->GetDevice());

  if (g_object_cache && m_context_semaphore > 1)
  {
    // something else is holding the context, so it won't shutdown now.
    // But we want to make sure our pipeline cache has been saved
    g_object_cache->SavePipelineCache();
  }

  ShutdownShared();
  ReleaseContext();
}

void VideoBackend::PrepareWindow(WindowSystemInfo& wsi)
{
#if defined(VK_USE_PLATFORM_METAL_EXT)
  // This is kinda messy, but it avoids having to write Objective C++ just to create a metal layer.
  id view = reinterpret_cast<id>(wsi.render_surface);
  Class clsCAMetalLayer = objc_getClass("CAMetalLayer");
  if (!clsCAMetalLayer)
  {
    ERROR_LOG_FMT(VIDEO, "Failed to get CAMetalLayer class.");
    return;
  }

  // [CAMetalLayer layer]
  id layer = reinterpret_cast<id (*)(Class, SEL)>(objc_msgSend)(objc_getClass("CAMetalLayer"),
                                                                sel_getUid("layer"));
  if (!layer)
  {
    ERROR_LOG_FMT(VIDEO, "Failed to create Metal layer.");
    return;
  }

  // [view setWantsLayer:YES]
  reinterpret_cast<void (*)(id, SEL, BOOL)>(objc_msgSend)(view, sel_getUid("setWantsLayer:"), YES);

  // [view setLayer:layer]
  reinterpret_cast<void (*)(id, SEL, id)>(objc_msgSend)(view, sel_getUid("setLayer:"), layer);

  // NSScreen* screen = [NSScreen mainScreen]
  id screen = reinterpret_cast<id (*)(Class, SEL)>(objc_msgSend)(objc_getClass("NSScreen"),
                                                                 sel_getUid("mainScreen"));

  // CGFloat factor = [screen backingScaleFactor]
  double factor =
      reinterpret_cast<double (*)(id, SEL)>(objc_msgSend)(screen, sel_getUid("backingScaleFactor"));

  // layer.contentsScale = factor
  reinterpret_cast<void (*)(id, SEL, double)>(objc_msgSend)(layer, sel_getUid("setContentsScale:"),
                                                            factor);

  // Store the layer pointer, that way MoltenVK doesn't call [NSView layer] outside the main thread.
  wsi.render_surface = layer;
#endif
}
}  // namespace Vulkan
