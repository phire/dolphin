// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2 or Later
// Refer to the license.txt file included.

#include "VideoBackends/CrossPhire/VertexManager.h"

#include "VideoCommon/IndexGenerator.h"

namespace CrossPhire
{

class VertexFormat : public NativeVertexFormat
{
public:

	virtual void Initialize(const PortableVertexDeclaration &vtx_decl)
	{
		this->vtx_decl = vtx_decl;
	}
	virtual void SetupVertexPointers() {}
};

NativeVertexFormat *VertexManager::CreateNativeVertexFormat() 
{
	return new VertexFormat;
}

void VertexManager::ResetBuffer(u32 stride)
{
	s_pCurBufferPointer = s_pBaseBufferPointer;
	IndexGenerator::Start(indexBuffer.get());
}

}