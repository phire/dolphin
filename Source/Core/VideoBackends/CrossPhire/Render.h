// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2 or Later
// Refer to the license.txt file included.

#pragma once

#include "VideoCommon/RenderBase.h"

namespace CrossPhire
{

	class Renderer : public ::Renderer
	{
	public:
		Renderer();
		~Renderer();

		static void Init() {}
		static void Shutdown() {}

		void SetColorMask() override {}
		void SetBlendMode(bool forceUpdate) override {}
		void SetScissorRect(const EFBRectangle& rc) override {}
		void SetGenerationMode() override {}
		void SetDepthMode() override {}
		void SetLogicOpMode() override {}
		void SetDitherMode() override {}
		void SetSamplerState(int stage, int texindex, bool custom_tex) override {}
		void SetInterlacingMode() override {}
		void SetViewport() override {}

		void ApplyState(bool bUseDstAlpha) override {}
		void RestoreState() override {}

		void RenderText(const std::string& text, int left, int top, u32 color) override {}
		void ShowEfbCopyRegions() {}
		void FlipImageData(u8 *data, int w, int h, int pixel_width = 3) {}

		u32 AccessEFB(EFBAccessType type, u32 x, u32 y, u32 poke_data) override { return 0; }

		u16 BBoxRead(int index) override { return 0; }
		void BBoxWrite(int index, u16 value) override {}

		void ResetAPIState() override {}
		void RestoreAPIState() override {}

		TargetRectangle ConvertEFBRectangle(const EFBRectangle& rc) override
		{ 
			TargetRectangle rect;
			return rect;
		}

		void SwapImpl(u32 xfbAddr, u32 fbWidth, u32 fbStride, u32 fbHeight, const EFBRectangle& rc, float Gamma) override {}

		void ClearScreen(const EFBRectangle& rc, bool colorEnable, bool alphaEnable, bool zEnable, u32 color, u32 z) override {}

		void ReinterpretPixelData(unsigned int convtype) override {}

		bool SaveScreenshot(const std::string &filename, const TargetRectangle &rc) override { return false;  }

		int GetMaxTextureSize() override { return 0; }
	};

}