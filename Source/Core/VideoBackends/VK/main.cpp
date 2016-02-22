// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#define VK_USE_PLATFORM_WIN32_KHR

#include "Common/Common.h"
#include "Common/VK/VkLoader.h"

#include "Core/ConfigManager.h"
#include "Core/Host.h"

#include "VideoBackends/VK/Renderer.h"
#include "VideoBackends/VK/TextureCache.h"
#include "VideoBackends/VK/VertexManager.h"
#include "VideoBackends/VK/VideoBackend.h"

#include "VideoCommon/BPStructs.h"
#include "VideoCommon/CommandProcessor.h"
#include "VideoCommon/Fifo.h"
#include "VideoCommon/GeometryShaderManager.h"
#include "VideoCommon/IndexGenerator.h"
#include "VideoCommon/OnScreenDisplay.h"
#include "VideoCommon/OpcodeDecoding.h"
#include "VideoCommon/PixelEngine.h"
#include "VideoCommon/PixelShaderManager.h"
#include "VideoCommon/VertexLoaderManager.h"
#include "VideoCommon/VertexShaderManager.h"
#include "VideoCommon/VideoConfig.h"


namespace VK {

VkInstance s_vulkan_instance;
std::vector<VkPhysicalDevice> s_physical_devices;
VkSurfaceKHR s_surface;
VkDebugReportCallbackEXT s_debug_callback;

VkBool32 debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location,
                              int32_t messageCode,  const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		ERROR_LOG(HOST_GPU, "[%s] Code %i: %s", pLayerPrefix, messageCode, pMessage);
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		WARN_LOG(HOST_GPU, "[%s] Code %i: %s", pLayerPrefix, messageCode, pMessage);
	return false;
}

static bool CreateInstance()
{
	if (!VulkanLoad())
	{
		PanicAlert("Vulkan Loader not found, have you installed drivers with vulkan support?");
		return false;
	}

	VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr };
	appInfo.pApplicationName = "Dolphin Emulator";
	appInfo.applicationVersion = 0; // TODO: provide meaningful version number
	appInfo.pEngineName = nullptr;
	appInfo.engineVersion = 0;
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 2); // Technically we implement 1.0.3, but AMDs vulkan drivers only implement 1.0.2

	VkInstanceCreateInfo instanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0 };
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledLayerCount = 1;
	const char* layers[] = { "VK_LAYER_LUNARG_standard_validation" }; // Debug layers
	instanceInfo.ppEnabledLayerNames = layers;
	instanceInfo.enabledExtensionCount = 3;
	const char* extensions[] = {
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME
	};
	instanceInfo.ppEnabledExtensionNames = extensions;

	VkResult ret = vkCreateInstance(&instanceInfo, nullptr, &s_vulkan_instance);
	if (ret != VK_SUCCESS)
	{
		ERROR_LOG(VIDEO, "Couldn't create Vulkan instance, error %i", ret);
		return false;
	}

	VulkanLoadInstanceFunctions(s_vulkan_instance);

	VkDebugReportCallbackCreateInfoEXT callbackInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT, nullptr };
	callbackInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugCallback;
	callbackInfo.pUserData = nullptr;
	callbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

	vkCreateDebugReportCallbackEXT(s_vulkan_instance, &callbackInfo, nullptr, &s_debug_callback);

	u32 num_physical_devices;
	vkEnumeratePhysicalDevices(s_vulkan_instance, &num_physical_devices, nullptr);
	s_physical_devices.assign(num_physical_devices, VkPhysicalDevice());
	vkEnumeratePhysicalDevices(s_vulkan_instance, &num_physical_devices, s_physical_devices.data());

	return true;
}

// Draw messages on top of the screen
unsigned int VideoBackend::PeekMessages()
{
	return 0; // TODO
}

std::string VideoBackend::GetName() const
{
	return "VK";
}

std::string VideoBackend::GetDisplayName() const
{
	return "Vulkan";
}

static void InitBackendInfo()
{
	g_Config.backend_info.APIType = API_OPENGL; // We want OpenGL's shaders
	g_Config.backend_info.bSupportsExclusiveFullscreen = false;
	g_Config.backend_info.bSupportsOversizedViewports = true; // ?
	g_Config.backend_info.bSupportsGeometryShaders = false; // not yet
	g_Config.backend_info.bSupports3DVision = false;
	g_Config.backend_info.bSupportsPostProcessing = false; // TODO
	g_Config.backend_info.bSupportsSSAA = false; // not yet

	g_Config.backend_info.Adapters.clear();

	if(s_physical_devices.empty())
	{
		g_Config.backend_info.Adapters.push_back("No display adapters found");
	}

	for (auto physicalDevice : s_physical_devices)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		g_Config.backend_info.Adapters.push_back(properties.deviceName);

		// TODO, fill out more backend info for the currently selected device.
	}

	// aamodes - 1 is to stay consistent with D3D (means no AA)
	g_Config.backend_info.AAModes = { 1 };
}

void VideoBackend::ShowConfig(void* parent_handle)
{
	if (!m_initialized)
	{
		CreateInstance();
		InitBackendInfo();
	}

	Host_ShowVideoConfig(parent_handle, GetDisplayName(), "gfx_vulkan");
}

bool VideoBackend::Initialize(void* window_handle)
{
	if (!m_initialized)
	{
		if (!CreateInstance() || s_physical_devices.empty())
			return false;
		InitBackendInfo();
	}

	if (window_handle != nullptr)
	{
		VkWin32SurfaceCreateInfoKHR surfaceInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr, 0 };
		surfaceInfo.hinstance = 0; // I hope the vulkan implementation doesn't actually need this.
		surfaceInfo.hwnd = reinterpret_cast<HWND>(window_handle);

		if (vkCreateWin32SurfaceKHR(s_vulkan_instance, &surfaceInfo, nullptr, &s_surface) != VK_SUCCESS) {
			PanicAlert("Couldn't create a surface for vulkan.");
			s_surface = VK_NULL_HANDLE;
		}
	}

	InitializeShared("gfx_vulkan.ini");

	// Do our OSD callbacks
	OSD::DoCallbacks(OSD::CallbackType::Initialization);

	return true;
}

// This is called after Initialize() from the Core
// Run from the graphics thread
void VideoBackend::Video_Prepare()
{
	g_renderer = std::make_unique<Renderer>(s_physical_devices[g_Config.iAdapter], s_surface);

	CommandProcessor::Init();
	PixelEngine::Init();

	BPInit();
	g_vertex_manager = std::make_unique<VertexManager>();
	//g_perf_query = GetPerfQuery();
	Fifo::Init(); // must be done before OpcodeDecoder::Init()
	OpcodeDecoder::Init();
	IndexGenerator::Init();
	VertexShaderManager::Init();
	PixelShaderManager::Init();
	GeometryShaderManager::Init();
	//ProgramShaderCache::Init();
	g_texture_cache = std::make_unique<TextureCache>();
	//g_sampler_cache = std::make_unique<SamplerCache>();
	//Renderer::Init();
	VertexLoaderManager::Init();
	//TextureConverter::Init();
	//BoundingBox::Init();

	// Notify the core that the video backend is ready
	Host_Message(WM_USER_CREATE);
}

void VideoBackend::Shutdown()
{
	vkDestroySurfaceKHR(s_vulkan_instance, s_surface, nullptr);
	s_physical_devices.clear();

	vkDestroyDebugReportCallbackEXT(s_vulkan_instance, s_debug_callback, nullptr);

	vkDestroyInstance(s_vulkan_instance, nullptr);
	m_initialized = false;

	// Do our OSD callbacks
	OSD::DoCallbacks(OSD::CallbackType::Shutdown);
}

void VideoBackend::Video_Cleanup()
{
	if (!g_renderer)
		return;

	Fifo::Shutdown();

	// The following calls are NOT Thread Safe
	// And need to be called from the video thread
	//Renderer::Shutdown();
	//BoundingBox::Shutdown();
	//TextureConverter::Shutdown();
	VertexLoaderManager::Shutdown();
	//g_sampler_cache.reset();
	g_texture_cache.reset();
	//ProgramShaderCache::Shutdown();
	VertexShaderManager::Shutdown();
	PixelShaderManager::Shutdown();
	GeometryShaderManager::Shutdown();

	g_perf_query.reset();
	g_vertex_manager.reset();

	OpcodeDecoder::Shutdown();
	g_renderer.reset();
}

}
