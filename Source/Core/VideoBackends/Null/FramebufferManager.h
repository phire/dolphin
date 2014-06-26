// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include "VideoBackends/Null/Render.h"

#include "VideoCommon/FramebufferManagerBase.h"

namespace NullVideo {

struct XFBSource : public XFBSourceBase
{
	void CopyEFB(float Gamma) override {}
	void DecodeToTexture(u32 xfbAddr, u32 fbWidth, u32 fbHeight) override {}
	void Draw(const MathUtil::Rectangle<int> &sourcerc,
		const MathUtil::Rectangle<float> &drawrc) const override {}
};

class FramebufferManager : public FramebufferManagerBase
{
public:
	FramebufferManager(int targetWidth, int targetHeight) {
		m_targetWidth = targetWidth;
		m_targetHeight = targetHeight;
	}

private:
	XFBSourceBase* CreateXFBSource(unsigned int target_width, unsigned int target_height) override {
		return new XFBSource();
	}
	void GetTargetSize(unsigned int *width, unsigned int *height, const EFBRectangle& sourceRc) override {
		*width = m_targetWidth;
		*height = m_targetHeight;
	}

	void CopyToRealXFB(u32 xfbAddr, u32 fbWidth, u32 fbHeight, const EFBRectangle& sourceRc,float Gamma) override {}

	int m_targetWidth;
	int m_targetHeight;
};

}  // namespace NullVideo
