// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "Common/Assert.h"
#include "Common/Common.h"
#include "Common/Logging/Log.h"
#include "Common/MsgHandler.h"

#include "VideoBackends/VK/Renderer.h"
#include "VideoBackends/VK/MemoryAllocator.h"
#include "VideoBackends/VK/PipelineCache.h"
#include "VideoBackends/VK/VertexManager.h"

namespace VK
{

VkQueue g_queue;
VkCommandBuffer cmdBuffer;

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

static void DestroySwapchain(VkDevice device, VkSwapchainKHR oldSwapchain, std::vector<VkImageView> imageViews, std::vector<VkFramebuffer> framebuffers)
{
	for (VkFramebuffer fb : framebuffers)
		vkDestroyFramebuffer(device, fb, nullptr);

	for (VkImageView view : imageViews)
		vkDestroyImageView(device, view, nullptr);

	vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
}

// Create a swapchain that matches the current window dimensions.
void Renderer::CreateSwapchain()
{
	// Running without a surface is a completely valid thing you might want to do
	if (m_surface == VK_NULL_HANDLE)
		return;

	// Check that the device actually supports this surface.
	VkBool32 supported;
	vkGetPhysicalDeviceSurfaceSupportKHR(m_physical_device, 0, m_surface, &supported);
	_assert_msg_(VIDEO, supported, "Vulkan: Surface not supported by this device");

	// Dolphin almost never renders at the correct resolution, so we always render
	// into an off-screen buffer and just scale/post-process into the final buffer.
	// This allows us to enable a few optimizations for the swapchain.
	// We don't enable depth or stencil buffers and enable clipping so the implementation
	// is free skip the rendering of any pixels that won't be shown on the screen.

	// This has a side effect that reading back pixels from the swapchain image is now undefined.

	VkSwapchainKHR oldSwapchain;

	// Get capabilities of the surface, mostly for the window dimensions
	VkSurfaceCapabilitiesKHR surfaceCapablities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &surfaceCapablities);
	_assert_msg_(VIDEO, surfaceCapablities.minImageCount <= 3 && surfaceCapablities.maxImageCount >= 3,
				 "Vulkan implementation is fussy about number of surface images. Wants between %i and %i",
				 surfaceCapablities.minImageCount, surfaceCapablities.maxImageCount);
	_assert_(surfaceCapablities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT); // zero chance of this happening

	// Get supported image formats
	u32 numFormats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &numFormats, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(numFormats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &numFormats, formats.data());

	// Get the list of present modes.
	u32 numModes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &numModes, nullptr);
	std::vector<VkPresentModeKHR> presentModes(numModes);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &numModes, presentModes.data());

	// Find the best presentation mode we have (preferably non-vsynced)
	VkPresentModeKHR bestPresetMode = VK_PRESENT_MODE_FIFO_KHR; // vsync, all vulkan implementations are required to support this.
	for (auto mode : presentModes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			bestPresetMode = mode; // Doesn't vsync or tear, more latency than tearing
		else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR && bestPresetMode != VK_PRESENT_MODE_MAILBOX_KHR)
			bestPresetMode = mode; // Standard tearing mode.
		else if (mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR && bestPresetMode == VK_PRESENT_MODE_FIFO_KHR)
			bestPresetMode = mode; // Normally vsyncs, but tears if a frame is late.
	}

	VkSwapchainCreateInfoKHR info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, nullptr, 0 };
	info.surface = m_surface;
	info.minImageCount = 3; // Tripple buffering!
	// I'm not sure what the best way to go about selecting a color format is. Lets just pick the first one.
	info.imageFormat = formats[0].format;
	info.imageColorSpace = formats[0].colorSpace;
	info.imageExtent = m_surface_extent = surfaceCapablities.currentExtent; // Match the screen size.
	info.imageArrayLayers = 1; // No stereoscopy.
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Only attach a color buffer
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.preTransform = surfaceCapablities.currentTransform; // use current transform
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Ignore alpha
	info.presentMode = bestPresetMode;
	info.clipped = true; // Allow implementation to rendering of skip hidden pixels
	info.oldSwapchain = oldSwapchain = m_swapchain;

	VkResult ret = vkCreateSwapchainKHR(m_device, &info, nullptr, &m_swapchain);

	_assert_msg_(VIDEO, ret == VK_SUCCESS, "Error %i while creating Swapchain", ret);

	// Destroy the old swapchain, swapchain image views and framebuffers
	if (oldSwapchain != VK_NULL_HANDLE)
		DestroySwapchain(m_device, oldSwapchain, swapchainImageViews, swapchainFramebuffers); // Todo, some kind of fence?

	// Create a command buffer so we can convert the images
	VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr };
	allocateInfo.commandBufferCount = 1;
	allocateInfo.commandPool = m_commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkAllocateCommandBuffers(m_device, &allocateInfo, &cmdBuffer);

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuffer, &beginInfo);

	// Get the VkImages from the swapchain
	u32 count = 0;
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, nullptr);
	swapchainImages.resize(count);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, swapchainImages.data());

	// Convert the VkImages to the format we want and pre-create framebuffers
	VkImageMemoryBarrier convertBarrier = {};
	convertBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	convertBarrier.pNext = NULL;
	convertBarrier.srcAccessMask = 0;
	convertBarrier.dstAccessMask = 0;
	convertBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	convertBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Convert to PRESENT_SRC_KHR first, so we can simplify StartFrame() loigc
	convertBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	convertBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	convertBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0 };
	viewInfo.format = formats[0].format;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.components = {
		VK_COMPONENT_SWIZZLE_R,
		VK_COMPONENT_SWIZZLE_G,
		VK_COMPONENT_SWIZZLE_B,
		VK_COMPONENT_SWIZZLE_A
	};
	viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0 };
	framebufferInfo.renderPass = g_pipeline_cache->m_renderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.width = m_surface_extent.width;
	framebufferInfo.height = m_surface_extent.height;
	framebufferInfo.layers = 1;

	for (VkImage image : swapchainImages) // For each image
	{
		VkImageView view;
		VkFramebuffer framebuffer;

		// Use the image barrier to convert the image format
		convertBarrier.image = image;
		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &convertBarrier);

		// Create a view
		viewInfo.image = image;
		vkCreateImageView(m_device, &viewInfo, nullptr, &view);
		swapchainImageViews.push_back(view); // Stored simply so we can destroy it later.

		// Create an framebuffer
		framebufferInfo.pAttachments = &view;
		vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &framebuffer);
		swapchainFramebuffers.push_back(framebuffer);
	}

	vkEndCommandBuffer(cmdBuffer);
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.waitSemaphoreCount = 0;
	vkQueueSubmit(g_queue, 1, &submitInfo, nullptr);

	vkQueueWaitIdle(g_queue);

	vkResetCommandBuffer(cmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
}

void Renderer::StartFrame()
{
	u32 nextImage;
	VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, nullptr, nullptr, &nextImage);

	_assert_msg_(VIDEO, result == VK_SUCCESS, "vkAcquireNextImageKHR failed with error %i", result);

	currentImage = nextImage;

	VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr };
	allocateInfo.commandBufferCount = 1;
	allocateInfo.commandPool = m_commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkAllocateCommandBuffers(m_device, &allocateInfo, &cmdBuffer);

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuffer, &beginInfo);

	VkImageMemoryBarrier postPresentBarrier = {};
	postPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	postPresentBarrier.pNext = NULL;
	postPresentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	postPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	postPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	postPresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	postPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	postPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	postPresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	postPresentBarrier.image = swapchainImages[currentImage];

	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &postPresentBarrier);

	VkClearValue clearColor;
	clearColor.color = { 0.0f, 0.0f, 0.0f, 0.0f };
	VkRenderPassBeginInfo passInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr };
	passInfo.renderArea.extent = m_surface_extent;
	passInfo.renderArea.offset.x = 0;
	passInfo.renderArea.offset.y = 0;
	passInfo.clearValueCount = 1;
	passInfo.pClearValues = &clearColor;
	passInfo.renderPass = g_pipeline_cache->m_renderPass;
	passInfo.framebuffer = swapchainFramebuffers[currentImage];
	vkCmdBeginRenderPass(cmdBuffer, &passInfo, VK_SUBPASS_CONTENTS_INLINE);

	((VK::VertexManager*) (g_vertex_manager.get()))->StartCommandBuffer();
}

void Renderer::SwapImpl(u32 xfbAddr, u32 fbWidth, u32 fbStride, u32 fbHeight, const EFBRectangle& rc, float Gamma)
{
	// End the command buffer and submit it.
	vkCmdEndRenderPass(cmdBuffer);

	VkImageMemoryBarrier prePresentBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr };
	prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	prePresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	prePresentBarrier.image = swapchainImages[currentImage];

	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &prePresentBarrier);

	_assert_(!vkEndCommandBuffer(cmdBuffer));

	VkSemaphore swapSemaphore;
	VkSemaphoreCreateInfo info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
	vkCreateSemaphore(m_device, &info, nullptr, &swapSemaphore);

	//VkFence itsAfence;
	//VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr };
	//fenceInfo.flags = 0;
	//vkCreateFence(m_device, &fenceInfo, nullptr, &itsAfence);

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &swapSemaphore;
	submitInfo.waitSemaphoreCount = 0;
	_assert_(!vkQueueSubmit(g_queue, 1, &submitInfo, nullptr));

	// TODO: reuse command buffers
	_assert_(!vkQueueWaitIdle(g_queue));
	vkResetCommandBuffer(cmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

	//vkWaitForFences(m_device, 1, &itsAfence, true, UINT64_MAX);

	// Present the frame to the screen.
	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &swapSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.pImageIndices = &currentImage;
	VkResult result = vkQueuePresentKHR(g_queue, &presentInfo);

	_assert_msg_(VIDEO, result == VK_SUCCESS, "vkQueuePresent failed with error %i", result);

	vkDestroySemaphore(m_device, swapSemaphore, nullptr);

	Renderer::StartFrame();

}


Renderer::Renderer(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) : m_physical_device(physicalDevice), m_surface(surface)
{
	m_swapchain = VK_NULL_HANDLE;
	CreateDevice(physicalDevice);
	MemoryAllocator::Initilize(physicalDevice, m_device);
	g_pipeline_cache = std::make_unique<PipelineCache>(m_device);
	g_vertex_manager = std::make_unique<VertexManager>(m_device);

	vkGetDeviceQueue(m_device, 0, 0, &g_queue); // FIXME: Hardcoded to the first queue

	VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr };
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = 0;
	vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);

	CreateSwapchain();

	StartFrame(); // We need to start the first frame explicitly
}

Renderer::~Renderer()
{
	vkQueueWaitIdle(g_queue);
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
	g_pipeline_cache.reset();
	g_vertex_manager.reset();
	MemoryAllocator::Shutdown();
	if (m_swapchain != VK_NULL_HANDLE)
		DestroySwapchain(m_device, m_swapchain, swapchainImageViews, swapchainFramebuffers);
	vkDestroyDevice(m_device, nullptr);
}

}