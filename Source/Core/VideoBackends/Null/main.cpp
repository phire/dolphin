// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.



// Null Backend Documentation
/*

  The goal of the Null video backend is to allow benchmarking of the cpu parts
  of dolphin without being effected by either the video card or the video driver.
*/

#include <algorithm>
#include <cstdarg>

#include "Common/Atomic.h"
#include "Common/CommonPaths.h"
#include "Common/LogManager.h"
#include "Common/Thread.h"

#include "Core/ConfigManager.h"
#include "Core/Core.h"
#include "Core/Host.h"

#include "VideoBackends/Null/FramebufferManager.h"
#include "VideoBackends/Null/Render.h"
#include "VideoBackends/Null/TextureCache.h"
#include "VideoBackends/Null/VertexManager.h"
#include "VideoBackends/Null/VideoBackend.h"

#include "VideoCommon/BPStructs.h"
#include "VideoCommon/CommandProcessor.h"
#include "VideoCommon/Fifo.h"
#include "VideoCommon/ImageWrite.h"
#include "VideoCommon/IndexGenerator.h"
#include "VideoCommon/LookUpTables.h"
#include "VideoCommon/MainBase.h"
#include "VideoCommon/OnScreenDisplay.h"
#include "VideoCommon/OpcodeDecoding.h"
#include "VideoCommon/PixelEngine.h"
#include "VideoCommon/PixelShaderManager.h"
#include "VideoCommon/VertexLoader.h"
#include "VideoCommon/VertexLoaderManager.h"
#include "VideoCommon/VertexShaderManager.h"
#include "VideoCommon/VideoConfig.h"
#include "VideoCommon/VideoState.h"

#ifdef _WIN32
#include "Common/IniFile.h"
#endif

#if defined(HAVE_WX) && HAVE_WX
#include "DolphinWX/VideoConfigDiag.h"
#include "DolphinWX/Debugger/DebuggerPanel.h"
#endif // HAVE_WX

namespace NullVideo
{

unsigned int VideoBackend::PeekMessages()
{
	return 0;
}

// Show the current FPS
void VideoBackend::UpdateFPSDisplay(const std::string& text)
{
	printf("%s | %s | %s\n", scm_rev_str, GetDisplayName().c_str(), text.c_str());
}

std::string VideoBackend::GetName() const
{
	return "Null";
}

std::string VideoBackend::GetDisplayName() const
{
	return "Null Video";

}

void InitBackendInfo()
{
	g_Config.backend_info.APIType = API_NONE;
	g_Config.backend_info.bUseRGBATextures = true;
	g_Config.backend_info.bUseMinimalMipCount = false;
	g_Config.backend_info.bSupports3DVision = false;
	g_Config.backend_info.bSupportsDualSourceBlend = true;
	g_Config.backend_info.bSupportsEarlyZ = true;
	g_Config.backend_info.bSupportsOversizedViewports = true;

	g_Config.backend_info.Adapters.clear();

	// aamodes
	const char* caamodes[] = {_trans("None")};
	g_Config.backend_info.AAModes.assign(caamodes, caamodes + sizeof(caamodes)/sizeof(*caamodes));
}

void VideoBackend::ShowConfig(void *_hParent)
{
#if defined(HAVE_WX) && HAVE_WX
	InitBackendInfo();
	VideoConfigDiag diag((wxWindow*)_hParent, "OpenGL", "gfx_opengl");
	diag.ShowModal();
#endif
}

bool VideoBackend::Initialize(void *&window_handle)
{
	InitializeShared();
	InitBackendInfo();

	frameCount = 0;

	g_Config.Load(File::GetUserPath(D_CONFIG_IDX) + "gfx_opengl.ini");
	g_Config.GameIniLoad();
	g_Config.UpdateProjectionHack();
	g_Config.VerifyValidity();
	UpdateActiveConfig();

	// Do our OSD callbacks
	OSD::DoCallbacks(OSD::OSD_INIT);

	s_BackendInitialized = true;

	return true;
}

// This is called after Initialize() from the Core
// Run from the graphics thread
void VideoBackend::Video_Prepare()
{
	g_renderer = new Renderer;

	s_efbAccessRequested = false;
	s_FifoShuttingDown = false;
	s_swapRequested = false;

	CommandProcessor::Init();
	PixelEngine::Init();

	BPInit();
	g_vertex_manager = new VertexManager;
	g_perf_query = new PerfQueryBase;
	Fifo_Init(); // must be done before OpcodeDecoder_Init()
	OpcodeDecoder_Init();
	IndexGenerator::Init();
	VertexShaderManager::Init();
	PixelShaderManager::Init();
	g_texture_cache = new TextureCache();
	Renderer::Init();
	VertexLoaderManager::Init();

	// Notify the core that the video backend is ready
	Host_Message(WM_USER_CREATE);
}

void VideoBackend::Shutdown()
{
	s_BackendInitialized = false;

	// Do our OSD callbacks
	OSD::DoCallbacks(OSD::OSD_SHUTDOWN);
}

void VideoBackend::Video_Cleanup() {

	if (g_renderer)
	{
		s_efbAccessRequested = false;
		s_FifoShuttingDown = false;
		s_swapRequested = false;
		Fifo_Shutdown();

		// The following calls are NOT Thread Safe
		// And need to be called from the video thread
		Renderer::Shutdown();
		VertexLoaderManager::Shutdown();
		delete g_texture_cache;
		g_texture_cache = nullptr;
		PixelShaderManager::Shutdown();
		delete g_perf_query;
		g_perf_query = nullptr;
		delete g_vertex_manager;
		g_vertex_manager = nullptr;
		OpcodeDecoder_Shutdown();
		delete g_renderer;
		g_renderer = nullptr;
	}
}

}
