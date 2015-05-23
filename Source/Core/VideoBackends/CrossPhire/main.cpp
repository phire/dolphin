// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2 or Later
// Refer to the license.txt file included.


#include "Core/ConfigManager.h"
#include "Core/Host.h"

#include "VideoBackends/CrossPhire/Render.h"
#include "VideoBackends/CrossPhire/TextureCache.h"
#include "VideoBackends/CrossPhire/VertexManager.h"
#include "VideoBackends/CrossPhire/VideoBackend.h"

// TODO: Extract gl headers from OGL backend
#include "VideoBackends/OGL/GLUtil.h"
#include "VideoBackends/OGL/GLInterfaceBase.h"

#include "VideoCommon/BPStructs.h"
#include "VideoCommon/CommandProcessor.h"
#include "VideoCommon/Fifo.h"
#include "VideoCommon/IndexGenerator.h"
#include "VideoCommon/MainBase.h"
#include "VideoCommon/OpcodeDecoding.h"
#include "VideoCommon/RenderBase.h"
#include "VideoCommon/PixelEngine.h"
#include "VideoCommon/TextureCacheBase.h"
#include "VideoCommon/VertexLoaderManager.h"
#include "VideoCommon/VertexManagerBase.h"
#include "VideoCommon/VideoConfig.h"

namespace CrossPhire
{

unsigned int VideoBackend::PeekMessages()
{
	if (GLInterface != NULL)
		return GLInterface->PeekMessages();
	return false;
}

std::string VideoBackend::GetName() const
{
	return "CrossPhire";
}

std::string VideoBackend::GetDisplayName() const
{
	return "CrossPhire Software Renderer";
}

void VideoBackend::ShowConfig(void *hParent)
{
	Host_ShowVideoConfig(hParent, GetDisplayName(), "gfx_crossphire");
}

bool VideoBackend::Initialize(void *window_handle)
{
	InitializeShared();

	g_Config.Load(File::GetUserPath(D_CONFIG_IDX) + "gfx_crossphire.ini");
	g_Config.GameIniLoad();
	g_Config.UpdateProjectionHack();
	g_Config.VerifyValidity();
	UpdateActiveConfig();

	InitInterface();
	GLInterface->SetMode(GLInterfaceMode::MODE_DETECT);
	if (!GLInterface->Create(window_handle))
		return false;

	s_BackendInitialized = true;
	return true;
}

void VideoBackend::Video_Prepare()
{
	GLInterface->MakeCurrent();

	g_renderer = new Renderer;

	CommandProcessor::Init();
	PixelEngine::Init();

	BPInit();
	g_vertex_manager = new VertexManager;
	g_perf_query = new PerfQueryBase;
	Fifo_Init(); // must be done before OpcodeDecoder_Init()
	OpcodeDecoder_Init();
	IndexGenerator::Init();
	g_texture_cache = new TextureCache();
	Renderer::Init();
	VertexLoaderManager::Init();

	// Notify the core that the video backend is ready
	Host_Message(WM_USER_CREATE);

}

void VideoBackend::Shutdown()
{
	s_BackendInitialized = false;
}

void VideoBackend::Video_Cleanup()
{
}

}