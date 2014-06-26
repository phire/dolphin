// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "Common/Atomic.h"
#include "Common/CommonPaths.h"
#include "Common/FileUtil.h"
#include "Common/StringUtil.h"
#include "Common/Thread.h"
#include "Common/Timer.h"

#include "Core/ConfigManager.h"
#include "Core/Core.h"
#include "Core/Movie.h"

#include "VideoBackends/Null/FramebufferManager.h"
#include "VideoBackends/Null/Render.h"

#include "VideoCommon/BPFunctions.h"
#include "VideoCommon/BPStructs.h"
#include "VideoCommon/DriverDetails.h"
#include "VideoCommon/Fifo.h"
#include "VideoCommon/FPSCounter.h"
#include "VideoCommon/ImageWrite.h"
#include "VideoCommon/OnScreenDisplay.h"
#include "VideoCommon/PixelEngine.h"
#include "VideoCommon/Statistics.h"
#include "VideoCommon/VertexLoader.h"
#include "VideoCommon/VertexLoaderManager.h"
#include "VideoCommon/VertexShaderGen.h"
#include "VideoCommon/VertexShaderManager.h"
#include "VideoCommon/VideoConfig.h"

#if defined(HAVE_WX) && HAVE_WX
#include "DolphinWX/WxUtils.h"
#endif

#ifdef _WIN32
#include <mmsystem.h>
#endif

extern int OSDInternalW, OSDInternalH;

namespace NullVideo
{
VideoConfig g_ogl_config;

// Declarations and definitions
// ----------------------------
static int s_fps = 0;

#if defined(HAVE_WX) && HAVE_WX
static std::thread scrshotThread;
#endif

// Init functions
Renderer::Renderer()
{
	OSDInternalW = 0;
	OSDInternalH = 0;

	s_fps=0;
	InitFPSCounter();

	g_Config.bRunning = true;

	UpdateActiveConfig();
}

Renderer::~Renderer()
{

#if defined(HAVE_WX) && HAVE_WX
	if (scrshotThread.joinable())
		scrshotThread.join();
#endif
}

void Renderer::Shutdown()
{
	delete g_framebuffer_manager;

	g_Config.bRunning = false;
	UpdateActiveConfig();
}

void Renderer::Init()
{
	// Initialize the FramebufferManager
	g_framebuffer_manager = new FramebufferManager(s_target_width, s_target_height);

}

// Create On-Screen-Messages
void Renderer::DrawDebugInfo()
{

}

void Renderer::RenderText(const std::string& text, int left, int top, u32 color)
{

}

TargetRectangle Renderer::ConvertEFBRectangle(const EFBRectangle& rc)
{
	TargetRectangle result;
	result.left   = EFBToScaledX(rc.left);
	result.top    = EFBToScaledY(EFB_HEIGHT - rc.top);
	result.right  = EFBToScaledX(rc.right);
	result.bottom = EFBToScaledY(EFB_HEIGHT - rc.bottom);
	return result;
}

// No video, no access to EFB.
// TODO: Set the option so Skip EFB Acesss is always enabled.
u32 Renderer::AccessEFB(EFBAccessType type, u32 x, u32 y, u32 poke_data)
{
	return 0;
}

void Renderer::ClearScreen(const EFBRectangle& rc, bool colorEnable, bool alphaEnable, bool zEnable, u32 color, u32 z)
{

}

void Renderer::ReinterpretPixelData(unsigned int convtype)
{

}

// This function has the final picture. We adjust the aspect ratio here.
void Renderer::SwapImpl(u32 xfbAddr, u32 fbWidth, u32 fbHeight,const EFBRectangle& rc,float Gamma)
{
	if (XFBWrited)
		s_fps = UpdateFPSCounter();
}

// ALWAYS call RestoreAPIState for each ResetAPIState call you're doing
void Renderer::ResetAPIState()
{

}

void Renderer::RestoreAPIState()
{

}

bool Renderer::SaveScreenshot(const std::string &filename, const TargetRectangle &back_rc)
{
	return false; // TODO: This function is pure virtual in RenderBase, but is only called from inside this file.
}

}
