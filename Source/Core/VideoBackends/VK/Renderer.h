// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <vulkan/vulkan.h>

#include "VideoCommon/RenderBase.h"

namespace VK
{

class Renderer : public ::Renderer
{
public:
	Renderer(VkPhysicalDevice physicalDevice);

	static void Init();
	static void Shutdown();

	void RenderText(const std::string& text, int left, int top, u32 color) override { };

	u32 AccessEFB(EFBAccessType type, u32 x, u32 y, u32 poke_data) override { return 0; };
	void PokeEFB(EFBAccessType type, const EfbPokeData* points, size_t num_points) override { };

	u16 BBoxRead(int index) override { return 0; };
	void BBoxWrite(int index, u16 value) override { };

	TargetRectangle ConvertEFBRectangle(const EFBRectangle& rc) override { return TargetRectangle(); };

	void SwapImpl(u32 xfbAddr, u32 fbWidth, u32 fbStride, u32 fbHeight, const EFBRectangle& rc, float Gamma) override { };

	void ClearScreen(const EFBRectangle& rc, bool colorEnable, bool alphaEnable, bool zEnable, u32 color, u32 z) override { };

	void ReinterpretPixelData(unsigned int convtype) override { };

	int GetMaxTextureSize() override { return 1; };

private:
	VkPhysicalDevice m_physical_device;
	VkDevice m_device;
	bool CreateDevice(VkPhysicalDevice physicalDevice);
};

}
