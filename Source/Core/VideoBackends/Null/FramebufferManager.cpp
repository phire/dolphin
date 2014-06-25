// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "Core/HW/Memmap.h"

#include "VideoBackends/Null/FramebufferManager.h"
#include "VideoBackends/Null/Render.h"
#include "VideoBackends/Null/TextureConverter.h"

#include "VideoCommon/DriverDetails.h"
#include "VideoCommon/OnScreenDisplay.h"
#include "VideoCommon/VertexShaderGen.h"

namespace NullVideo
{

int FramebufferManager::m_targetWidth;
int FramebufferManager::m_targetHeight;


FramebufferManager::FramebufferManager(int targetWidth, int targetHeight)
{
	m_targetWidth = targetWidth;
	m_targetHeight = targetHeight;
}


void FramebufferManager::CopyToRealXFB(u32 xfbAddr, u32 fbWidth, u32 fbHeight, const EFBRectangle& sourceRc,float Gamma)
{
	// Do nothing
}


XFBSourceBase* FramebufferManager::CreateXFBSource(unsigned int target_width, unsigned int target_height)
{
	return new XFBSource();
}

void FramebufferManager::GetTargetSize(unsigned int *width, unsigned int *height, const EFBRectangle& sourceRc)
{
	*width = m_targetWidth;
	*height = m_targetHeight;
}

}  // namespace NullVideo
