// Copyright 2016 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VideoBackends/Vulkan/VulkanHeaders.h"

// Currently, exclusive fullscreen is only supported on Windows.
#if defined(WIN32)
#define SUPPORTS_VULKAN_EXCLUSIVE_FULLSCREEN 1
#endif

// We abuse the preprocessor here to only need to specify function names once.
#define VULKAN_MODULE_ENTRY_POINT(name, required) extern PFN_##name name;
#define VULKAN_INSTANCE_ENTRY_POINT(name, required) extern PFN_##name name;
#define VULKAN_DEVICE_ENTRY_POINT(name, required) extern PFN_##name name;
#include "VideoBackends/Vulkan/VulkanEntryPoints.inl"
#undef VULKAN_DEVICE_ENTRY_POINT
#undef VULKAN_INSTANCE_ENTRY_POINT
#undef VULKAN_MODULE_ENTRY_POINT

#include "Common/Logging/Log.h"

namespace Vulkan
{
bool LoadVulkanLibrary();
bool LoadVulkanInstanceFunctions(VkInstance instance);
bool LoadVulkanDeviceFunctions(VkDevice device);
void UnloadVulkanLibrary();

const char* VkResultToString(VkResult res);
void LogVulkanResult(Common::Log::LogLevel level, const char* func_name, VkResult res,
                     const char* msg, ...);

#define LOG_VULKAN_ERROR(res, ...)                                                                 \
  LogVulkanResult(Common::Log::LogLevel::LERROR, __func__, res, __VA_ARGS__)

}  // namespace Vulkan
