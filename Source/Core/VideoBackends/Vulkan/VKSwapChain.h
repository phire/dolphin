// Copyright 2016 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <vector>

#include "Common/CommonTypes.h"
#include "Common/WindowSystemInfo.h"
#include "VideoBackends/Vulkan/Constants.h"
#include "VideoCommon/TextureConfig.h"

namespace Vulkan
{
class CommandBufferManager;
class ObjectCache;
class VKTexture;
class VKFramebuffer;

class SwapChain
{
public:
  SwapChain(WindowSystemInfo wsi, bool fullscreen_supported, bool vsync) :
    m_wsi(wsi), m_vsync_enabled(vsync), m_fullscreen_supported(fullscreen_supported) {}
  virtual ~SwapChain() {}

  AbstractTextureFormat GetTextureFormat() const { return m_texture_format; }

  virtual VkSwapchainKHR GetSwapChain() const { return VK_NULL_HANDLE; }
  u32 GetWidth() const { return m_width; }
  u32 GetHeight() const { return m_height; }

  u32 GetCurrentImageIndex() const { return m_current_swap_chain_image_index; }
  virtual VkImage GetCurrentImage() const { return VK_NULL_HANDLE; }
  virtual VKTexture* GetCurrentTexture() const { return nullptr; }
  virtual VKFramebuffer* GetCurrentFramebuffer() const { return nullptr; }

  virtual VkResult AcquireNextImage() { return VK_SUCCESS; }

  virtual bool RecreateSurface(void* native_handle) { return true; }
  virtual bool ResizeSwapChain() { return true; }
  virtual bool RecreateSwapChain() { return true; }

  // Change vsync enabled state. This may fail as it causes a swapchain recreation.
  virtual bool SetVSync(bool enabled);

  // Is exclusive fullscreen supported?
  bool IsFullscreenSupported() const { return m_fullscreen_supported; }

  // Retrieves the "next" fullscreen state. Safe to call off-thread.
  bool GetCurrentFullscreenState() const { return m_current_fullscreen_state; }
  bool GetNextFullscreenState() const { return m_next_fullscreen_state; }
  void SetNextFullscreenState(bool state) { m_next_fullscreen_state = state; }

  // Updates the fullscreen state. Must call on-thread.
  bool SetFullscreenState(bool state);

protected:
  WindowSystemInfo m_wsi;
  AbstractTextureFormat m_texture_format = AbstractTextureFormat::Undefined;
  bool m_vsync_enabled = false;
  bool m_fullscreen_supported = false;
  bool m_current_fullscreen_state = false;
  bool m_next_fullscreen_state = false;

  u32 m_current_swap_chain_image_index = 0;

  u32 m_width = 0;
  u32 m_height = 0;
  u32 m_layers = 0;
};

class VKSwapChain : public SwapChain
{
public:
  VKSwapChain(const WindowSystemInfo& wsi, VkSurfaceKHR surface, bool vsync);
  virtual ~VKSwapChain() override;

  // Creates a vulkan-renderable surface for the specified window handle.
  static VkSurfaceKHR CreateVulkanSurface(VkInstance instance, const WindowSystemInfo& wsi);

  // Create a new swap chain from a pre-existing surface.
  static std::unique_ptr<SwapChain> Create(const WindowSystemInfo& wsi, VkSurfaceKHR surface,
                                           bool vsync);

  VkSurfaceKHR GetSurface() const { return m_surface; }
  VkSurfaceFormatKHR GetSurfaceFormat() const { return m_surface_format; }
  virtual VkSwapchainKHR GetSwapChain() const override { return m_swap_chain; }

  virtual VkImage GetCurrentImage() const override
  {
    return m_swap_chain_images[m_current_swap_chain_image_index].image;
  }
  virtual VKTexture* GetCurrentTexture() const override
  {
    return m_swap_chain_images[m_current_swap_chain_image_index].texture.get();
  }
  virtual VKFramebuffer* GetCurrentFramebuffer() const override
  {
    return m_swap_chain_images[m_current_swap_chain_image_index].framebuffer.get();
  }
  virtual VkResult AcquireNextImage() override;

  virtual bool RecreateSurface(void* native_handle) override;
  virtual bool ResizeSwapChain() override;
  virtual bool RecreateSwapChain() override;

private:
  bool SelectSurfaceFormat();
  bool SelectPresentMode();

  bool CreateSwapChain();
  void DestroySwapChain();

  bool SetupSwapChainImages();
  void DestroySwapChainImages();

  void DestroySurface();

  struct SwapChainImage
  {
    VkImage image{};
    std::unique_ptr<VKTexture> texture;
    std::unique_ptr<VKFramebuffer> framebuffer;
  };

  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  VkSurfaceFormatKHR m_surface_format = {};
  VkPresentModeKHR m_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

  VkSwapchainKHR m_swap_chain = VK_NULL_HANDLE;
  std::vector<SwapChainImage> m_swap_chain_images;
};

}  // namespace Vulkan
