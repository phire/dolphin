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
#include "VideoBackends/Null/ProgramShaderCache.h"
#include "VideoBackends/Null/Render.h"
#include "VideoBackends/Null/StreamBuffer.h"
#include "VideoBackends/Null/TextureCache.h"
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

static StreamBuffer *s_vertexBuffer;
static StreamBuffer *s_indexBuffer;
static size_t s_baseVertex;
static size_t s_index_offset;

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
	s_vertexBuffer = StreamBuffer::Create(GL_ARRAY_BUFFER, MAX_VBUFFER_SIZE);
	m_vertex_buffers = s_vertexBuffer->m_buffer;

	s_indexBuffer = StreamBuffer::Create(GL_ELEMENT_ARRAY_BUFFER, MAX_IBUFFER_SIZE);
	m_index_buffers = s_indexBuffer->m_buffer;

	m_last_vao = 0;
}

void VertexManager::DestroyDeviceObjects()
{
	GL_REPORT_ERRORD();
	glBindBuffer(GL_ARRAY_BUFFER, 0 );
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0 );
	GL_REPORT_ERROR();

	delete s_vertexBuffer;
	delete s_indexBuffer;
	GL_REPORT_ERROR();
}

void VertexManager::PrepareDrawBuffers(u32 stride)
{
	u32 vertex_data_size = IndexGenerator::GetNumVerts() * stride;
	u32 index_data_size = IndexGenerator::GetIndexLen() * sizeof(u16);

	s_vertexBuffer->Unmap(vertex_data_size);
	s_indexBuffer->Unmap(index_data_size);

	ADDSTAT(stats.thisFrame.bytesVertexStreamed, vertex_data_size);
	ADDSTAT(stats.thisFrame.bytesIndexStreamed, index_data_size);
}

void VertexManager::ResetBuffer(u32 stride)
{
	auto buffer = s_vertexBuffer->Map(MAXVBUFFERSIZE, stride);
	s_pCurBufferPointer = s_pBaseBufferPointer = buffer.first;
	s_pEndBufferPointer = buffer.first + MAXVBUFFERSIZE;
	s_baseVertex = buffer.second / stride;

	buffer = s_indexBuffer->Map(MAXIBUFFERSIZE * sizeof(u16));
	IndexGenerator::Start((u16*)buffer.first);
	s_index_offset = buffer.second;
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
