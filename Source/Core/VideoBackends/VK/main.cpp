// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "Common/Common.h"

#include "Core/ConfigManager.h"
#include "Core/Host.h"

#include "VideoBackends/VK/Renderer.h"
#include "VideoBackends/VK/TextureCache.h"
#include "VideoBackends/VK/VertexManager.h"
#include "VideoBackends/VK/VideoBackend.h"

#include "VideoCommon/BPStructs.h"
#include "VideoCommon/CommandProcessor.h"
#include "VideoCommon/Fifo.h"
#include "VideoCommon/GeometryShaderManager.h"
#include "VideoCommon/IndexGenerator.h"
#include "VideoCommon/OnScreenDisplay.h"
#include "VideoCommon/OpcodeDecoding.h"
#include "VideoCommon/PixelEngine.h"
#include "VideoCommon/PixelShaderManager.h"
#include "VideoCommon/VertexLoaderManager.h"
#include "VideoCommon/VertexShaderManager.h"
#include "VideoCommon/VideoConfig.h"


namespace VK {

// Draw messages on top of the screen
unsigned int VideoBackend::PeekMessages()
{
	return 0; // TODO
}

std::string VideoBackend::GetName() const
{
	return "VK";
}

std::string VideoBackend::GetDisplayName() const
{
	return "Vulkan";
}

static void InitBackendInfo()
{
	g_Config.backend_info.APIType = API_OPENGL; // We want OpenGL's shaders
	g_Config.backend_info.bSupportsExclusiveFullscreen = false;
	g_Config.backend_info.bSupportsOversizedViewports = true; // ?
	g_Config.backend_info.bSupportsGeometryShaders = false; // not yet
	g_Config.backend_info.bSupports3DVision = false;
	g_Config.backend_info.bSupportsPostProcessing = false; // TODO
	g_Config.backend_info.bSupportsSSAA = false; // not yet

	g_Config.backend_info.Adapters.clear();

	// aamodes - 1 is to stay consistent with D3D (means no AA)
	g_Config.backend_info.AAModes = { 1 };
}

void VideoBackend::ShowConfig(void* parent_handle)
{
	if (!m_initialized)
		InitBackendInfo();

	Host_ShowVideoConfig(parent_handle, GetDisplayName(), "gfx_vulkan");
}

bool VideoBackend::Initialize(void* window_handle)
{
	InitializeShared("gfx_vulkan.ini");
	InitBackendInfo();

	// InitInterface();

	// Do our OSD callbacks
	OSD::DoCallbacks(OSD::CallbackType::Initialization);

	m_initialized = true;

	return true;
}

// This is called after Initialize() from the Core
// Run from the graphics thread
void VideoBackend::Video_Prepare()
{

	g_renderer = std::make_unique<Renderer>();

	CommandProcessor::Init();
	PixelEngine::Init();

	BPInit();
	g_vertex_manager = std::make_unique<VertexManager>();
	//g_perf_query = GetPerfQuery();
	Fifo::Init(); // must be done before OpcodeDecoder::Init()
	OpcodeDecoder::Init();
	IndexGenerator::Init();
	VertexShaderManager::Init();
	PixelShaderManager::Init();
	GeometryShaderManager::Init();
	//ProgramShaderCache::Init();
	g_texture_cache = std::make_unique<TextureCache>();
	//g_sampler_cache = std::make_unique<SamplerCache>();
	//Renderer::Init();
	VertexLoaderManager::Init();
	//TextureConverter::Init();
	//BoundingBox::Init();

	// Notify the core that the video backend is ready
	Host_Message(WM_USER_CREATE);
}

void VideoBackend::Shutdown()
{
	m_initialized = false;

	// Do our OSD callbacks
	OSD::DoCallbacks(OSD::CallbackType::Shutdown);
}

void VideoBackend::Video_Cleanup()
{
	if (!g_renderer)
		return;

	Fifo::Shutdown();

	// The following calls are NOT Thread Safe
	// And need to be called from the video thread
	//Renderer::Shutdown();
	//BoundingBox::Shutdown();
	//TextureConverter::Shutdown();
	VertexLoaderManager::Shutdown();
	//g_sampler_cache.reset();
	g_texture_cache.reset();
	//ProgramShaderCache::Shutdown();
	VertexShaderManager::Shutdown();
	PixelShaderManager::Shutdown();
	GeometryShaderManager::Shutdown();

	g_perf_query.reset();
	g_vertex_manager.reset();

	OpcodeDecoder::Shutdown();
	g_renderer.reset();
}

}
