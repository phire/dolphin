// Copyright 2018 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <memory>

enum class WindowSystemType
{
  Headless,
  Windows,
  MacOS,
  Android,
  Xlib,
  Xcb,
  Wayland,
  FBDev,
  Haiku,
};

class GLContext;
typedef struct VkInstance_T* VkInstance;
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;
typedef struct VkPhysicalDevice_T* VkPhysicalDevice;
typedef struct VkDevice_T* VkDevice;

struct WindowSystemInfo
{
  WindowSystemInfo() = default;
  WindowSystemInfo(WindowSystemType type_, void* display_connection_, void* render_window_,
                   void* render_surface_)
      : type(type_), display_connection(display_connection_), render_window(render_window_),
        render_surface(render_surface_)
  {
  }

  // Window system type. Determines which GL context or Vulkan WSI is used.
  WindowSystemType type = WindowSystemType::Headless;

  // Connection to a display server. This is used on X11 and Wayland platforms.
  void* display_connection = nullptr;

  // Render window. This is a pointer to the native window handle, which depends
  // on the platform. e.g. HWND for Windows, Window for X11. If the surface is
  // set to nullptr, the video backend will run in headless mode.
  void* render_window = nullptr;

  // Render surface. Depending on the host platform, this may differ from the window.
  // This is kept seperate as input may require a different handle to rendering, and
  // during video backend startup the surface pointer may change (MoltenVK).
  void* render_surface = nullptr;

  // Scale of the render surface. For hidpi systems, this will be >1.
  float render_surface_scale = 1.0f;

  // Should dolphin render to surface/swapchain?
  bool enable_surface = true;

  // This allows core to use a Qt opengl context without depending on Qt.
  std::function<std::unique_ptr<GLContext>(bool)> gl_context_factory;

  // Query vulkan instance/device extentions required by the WindowSystem
  std::function<std::vector<std::string>()> vk_get_instance_extensions;
  std::function<std::vector<std::string>()> vk_get_device_extensions;

  // Tell window system about the created vulkan instance
  std::function<void(VkInstance)> vk_set_instance;

  // Get a pre-created surface from the window system.
  // Must have called vk_set_instance first
  std::function<VkSurfaceKHR()> vk_get_surface;

  // Tell the window system we are finished with it's surface
  std::function<void()> vk_surface_done;

  // Tell window system about selected device
  std::function<void(VkPhysicalDevice, VkDevice, int, int)> vk_set_device;
};
