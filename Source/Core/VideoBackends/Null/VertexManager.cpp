// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <fstream>
#include <string>
#include <vector>

#include "Common/FileUtil.h"
#include "Common/MemoryUtil.h"
#include "Common/StringUtil.h"

#include "VideoBackends/Null/main.h"
#include "VideoBackends/Null/Render.h"
#include "VideoBackends/Null/VertexManager.h"

#include "VideoCommon/BPMemory.h"
#include "VideoCommon/DriverDetails.h"
#include "VideoCommon/Fifo.h"
#include "VideoCommon/ImageWrite.h"
#include "VideoCommon/IndexGenerator.h"
#include "VideoCommon/PixelShaderManager.h"
#include "VideoCommon/Statistics.h"
#include "VideoCommon/VertexLoader.h"
#include "VideoCommon/VertexShaderGen.h"
#include "VideoCommon/VertexShaderManager.h"
#include "VideoCommon/VideoConfig.h"

// internal state for loading vertices
extern NativeVertexFormat *g_nativeVertexFmt;

namespace NullVideo
{
//This are the initially requested size for the buffers expressed in bytes
const u32 MAX_IBUFFER_SIZE =  2*1024*1024;
const u32 MAX_VBUFFER_SIZE = 32*1024*1024;

static u8 *s_vertexBuffer;
static u8 *s_indexBuffer;

VertexManager::VertexManager()
{
	CreateDeviceObjects();
}

VertexManager::~VertexManager()
{
	DestroyDeviceObjects();
}

void VertexManager::CreateDeviceObjects()
{
	// Junk buffers for Videocommon to render vertices into.
	// Instead of sending them directly to the videocard, we keep overwriting
	// the same buffers, discarding the data.
	s_vertexBuffer = new u8[MAX_VBUFFER_SIZE];
	s_indexBuffer = new u8[MAX_IBUFFER_SIZE];
}

void VertexManager::DestroyDeviceObjects()
{
	delete[] s_vertexBuffer;
	delete[] s_indexBuffer;
}

void VertexManager::PrepareDrawBuffers(u32 stride)
{
	u32 vertex_data_size = IndexGenerator::GetNumVerts() * stride;
	u32 index_data_size = IndexGenerator::GetIndexLen() * sizeof(u16);

	ADDSTAT(stats.thisFrame.bytesVertexStreamed, vertex_data_size);
	ADDSTAT(stats.thisFrame.bytesIndexStreamed, index_data_size);
}

void VertexManager::ResetBuffer(u32 stride)
{
	s_pCurBufferPointer = s_pBaseBufferPointer = s_vertexBuffer;
	s_pEndBufferPointer = s_vertexBuffer + MAXVBUFFERSIZE;

	IndexGenerator::Start((u16*)s_indexBuffer);
}

void VertexManager::Draw(u32 stride)
{
	INCSTAT(stats.thisFrame.numIndexedDrawCalls);
}

void VertexManager::vFlush(bool useDstAlpha)
{
	GLVertexFormat *nativeVertexFmt = (GLVertexFormat*)g_nativeVertexFmt;
	u32 stride  = nativeVertexFmt->GetVertexStride();

	PrepareDrawBuffers(stride);

	Draw(stride);
}


}  // namespace
