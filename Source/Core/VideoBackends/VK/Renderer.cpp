// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <vulkan/vulkan.h>

#include "Common/Common.h"
#include "Common/Logging/Log.h"
#include "Common/MsgHandler.h"

#include "VideoBackends/VK/Renderer.h"

namespace VK
{

// Create a VkDevice for us to use
bool Renderer::CreateDevice(VkPhysicalDevice physicalDevice)
{
	// TODO
	return false;
}

Renderer::Renderer(VkPhysicalDevice physicalDevice) : m_physical_device(physicalDevice)
{
	if (!CreateDevice(physicalDevice))
		PanicAlert("Couldn't create a Vulkan device");
}

}