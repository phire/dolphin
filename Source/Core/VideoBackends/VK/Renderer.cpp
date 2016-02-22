// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "Common/Assert.h"
#include "Common/Common.h"
#include "Common/Logging/Log.h"
#include "Common/MsgHandler.h"

#include "VideoBackends/VK/Renderer.h"

namespace VK
{

// Create a VkDevice for us to use
void Renderer::CreateDevice(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceFeatures features;
	memset(&features, 0, sizeof(VkPhysicalDeviceFeatures)); // Disable all features

	// Enable the features that we need. If a device doesn't support a feature, creation will fail.
	features.geometryShader = true;
	features.dualSrcBlend = true;
	features.logicOp = true;

	// Baby steps: Enable 1 queue from the first family
	// TODO: AMD is probably going to require more advanced queue creation logic.
	float priorities[1] = { 1.0f };
	VkDeviceQueueCreateInfo queues = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0 };
	queues.queueCount = 1;
	queues.queueFamilyIndex = 0;
	queues.pQueuePriorities = priorities;

	VkDeviceCreateInfo info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, nullptr, 0 };
	info.pEnabledFeatures = &features;
	info.queueCreateInfoCount = 1;
	info.pQueueCreateInfos = &queues;
	info.enabledExtensionCount = 1;
	const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	info.ppEnabledExtensionNames = extensions;
	info.enabledLayerCount = 1;
	const char* layers[] = { "VK_LAYER_LUNARG_standard_validation" }; // Debug layers
	info.ppEnabledLayerNames = layers;

	VkResult ret = vkCreateDevice(physicalDevice, &info, nullptr, &m_device);
	_assert_msg_(Video, ret == VK_SUCCESS, "Couldn't create a Vulkan device. Error code %i", ret);

	VulkanLoadDeviceFunctions(m_device);
}

// Create a swapchain that matches the current window dimensions.
void Renderer::CreateSwapchain()
{
	// Running without a surface is a completely valid thing you might want to do
	if (m_surface == VK_NULL_HANDLE)
		return;

	// Dolphin almost never renders at the correct resolution, so we always render
	// into an off-screen buffer and just scale/post-process into the final buffer.
	// This allows us to enable a few optimizations for the swapchain.
	// We don't enable depth or stencil buffers and enable clipping so the implementation
	// is free skip the rendering of any pixels that won't be shown on the screen.

	// This has a side effect that reading back pixels from the swapchain image is now undefined.

	VkSwapchainKHR oldSwapchain;

	VkSurfaceCapabilitiesKHR surfaceCapablities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &surfaceCapablities);

	VkSwapchainCreateInfoKHR info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, nullptr, 0 };
	info.surface = m_surface;
	info.minImageCount = 3; // Tripple buffering!
	info.imageFormat = VK_FORMAT_R8G8B8A8_UINT;
	info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	info.imageExtent = surfaceCapablities.currentExtent; // Match the screen size.
	info.imageArrayLayers = 1; // No stereoscopy.
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Only attach a color buffer
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.preTransform = VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR; // Use OS rotation
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Ignore alpha
	info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR; // TODO: this forces VSYNC on
	info.clipped = true; // Allow implementation to rendering of skip hidden pixels
	info.oldSwapchain = oldSwapchain = m_swapchain;

	VkResult ret = vkCreateSwapchainKHR(m_device, &info, nullptr, &m_swapchain);

	_assert_msg_(VIDEO, ret == VK_SUCCESS, "Error %i while creating Swapchain", ret);

	// Destroy the old swapchain
	if (oldSwapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);
}

Renderer::Renderer(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) : m_physical_device(physicalDevice), m_surface(surface)
{
	m_swapchain = VK_NULL_HANDLE;
	CreateDevice(physicalDevice);
	CreateSwapchain();
	vkGetDeviceQueue(m_device, 0, 0, &m_queue); // FIXME: Hardcoded to the first queue

	// Temporary code to prove that we can display something on the screen (clear the buffer to 'sky blue')

	VkImage swapchainImages[3];
	u32 count = 3;
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, swapchainImages);

	u32 nextImage;
	vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, nullptr, nullptr, &nextImage);

	VkCommandPool commandPool;
	VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr };
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = 0;
	vkCreateCommandPool(m_device, &poolInfo, nullptr, &commandPool);

	VkCommandBuffer cmdBuf;
	VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr };
	allocateInfo.commandBufferCount = 1;
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkAllocateCommandBuffers(m_device, &allocateInfo, &cmdBuf);

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuf, &beginInfo);

	VkClearColorValue clearColor = {1.0f, 0.2f, 0.0f, 1.0f};
	VkImageSubresourceRange range;
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.layerCount = 1;
	range.baseArrayLayer = 0;
	range.levelCount = 1;
	vkCmdClearColorImage(cmdBuf, swapchainImages[nextImage], VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &range);

	vkEndCommandBuffer(cmdBuf);
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.waitSemaphoreCount = 0;
	vkQueueSubmit(m_queue, 1, &submitInfo, nullptr);

	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr };
	presentInfo.pWaitSemaphores = 0;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.pImageIndices = &nextImage;
	vkQueuePresentKHR(m_queue, &presentInfo);
}

Renderer::~Renderer()
{
	if (m_swapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	vkDestroyDevice(m_device, nullptr);
}

}