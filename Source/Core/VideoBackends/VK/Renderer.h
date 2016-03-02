// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <string>

#include "Common/VK/VkLoader.h"

#include "VideoCommon/RenderBase.h"

namespace VK
{

// FIXME: UGLY HACK
extern VkQueue g_queue;
extern VkCommandBuffer cmdBuffer; // Currently recording command Buffer

class Renderer : public ::Renderer
{
public:
	Renderer(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	~Renderer();

	static void Init();
	static void Shutdown();

	void RenderText(const std::string& text, int left, int top, u32 color) override { };

	u32 AccessEFB(EFBAccessType type, u32 x, u32 y, u32 poke_data) override { return 0; };
	void PokeEFB(EFBAccessType type, const EfbPokeData* points, size_t num_points) override { };

	u16 BBoxRead(int index) override { return 0; };
	void BBoxWrite(int index, u16 value) override { };

	TargetRectangle ConvertEFBRectangle(const EFBRectangle& rc) override { return TargetRectangle(); };

	void StartFrame();
	void SwapImpl(u32 xfbAddr, u32 fbWidth, u32 fbStride, u32 fbHeight, const EFBRectangle& rc, float Gamma) override;

	void ClearScreen(const EFBRectangle& rc, bool colorEnable, bool alphaEnable, bool zEnable, u32 color, u32 z) override { };

	void ReinterpretPixelData(unsigned int convtype) override { };

	int GetMaxTextureSize() override { return 1; };

private:
	void CreateDevice(VkPhysicalDevice physicalDevice);
	void CreateSwapchain();

	void TestTriangle();

	VkPhysicalDevice m_physical_device;
	VkDevice m_device;
	VkCommandPool m_commandPool;
	VkSurfaceKHR m_surface;
	VkSwapchainKHR m_swapchain;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> swapchainFramebuffers;
	u32 currentImage;
	VkExtent2D m_surface_extent;
};

}
