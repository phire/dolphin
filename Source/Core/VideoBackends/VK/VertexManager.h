// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <memory>

#include "VideoCommon/VertexManagerBase.h"

namespace VK
{

// TODO: This is a dummy vertex manager that just plops the vertices into a buffer in main memory.

class VertexManager : public VertexManagerBase
{
public:
	VertexManager()
	{
		vertexBuffer.reset(new u8[MAXVBUFFERSIZE]);

		s_pBaseBufferPointer = vertexBuffer.get();
		s_pEndBufferPointer = s_pBaseBufferPointer + MAXVBUFFERSIZE;

		indexBuffer.reset(new u16[MAXIBUFFERSIZE]);
	}
protected:
	NativeVertexFormat* CreateNativeVertexFormat(const PortableVertexDeclaration& vtx_decl) override;

	virtual void ResetBuffer(u32 stride) override;

private:
	virtual void vFlush(bool useDstAlpha) override { }

	std::unique_ptr<u8[]> vertexBuffer;
	std::unique_ptr<u16[]> indexBuffer;
};

}