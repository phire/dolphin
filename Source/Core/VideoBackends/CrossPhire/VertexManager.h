// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2 or Later
// Refer to the license.txt file included.

#pragma once

#include <memory>

#include "VideoCommon/VertexManagerBase.h"

namespace CrossPhire
{

class VertexManager : public ::VertexManager
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
	virtual ::NativeVertexFormat* CreateNativeVertexFormat() override;

	virtual void ResetBuffer(u32 stride) override;

private:
	virtual void vFlush(bool useDstAlpha) override { }

	std::unique_ptr<u8[]> vertexBuffer;
	std::unique_ptr<u16[]> indexBuffer;
};

}