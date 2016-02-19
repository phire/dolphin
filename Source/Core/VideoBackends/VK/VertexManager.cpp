// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "VideoBackends/VK/VertexManager.h"

#include "VideoCommon/IndexGenerator.h"
#include "VideoCommon/NativeVertexFormat.h"

namespace VK
{

class VKVertexFormat : public NativeVertexFormat
{
public:
	VKVertexFormat(const PortableVertexDeclaration& vtx_decl)
	{
		this->vtx_decl = vtx_decl;
	}

	void SetupVertexPointers() {}
};

NativeVertexFormat *VertexManager::CreateNativeVertexFormat(const PortableVertexDeclaration& vtx_decl)
{
	return new VKVertexFormat(vtx_decl);
}

void VertexManager::ResetBuffer(u32 stride)
{
	s_pCurBufferPointer = s_pBaseBufferPointer;
	IndexGenerator::Start(indexBuffer.get());
}

}