// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <memory>

#include "VideoBackends/VK/MemoryAllocator.h"

#include "VideoCommon/VertexManagerBase.h"


namespace VK
{

class VertexManager : public VertexManagerBase
{
public:
	VertexManager();
protected:
	NativeVertexFormat* CreateNativeVertexFormat(const PortableVertexDeclaration& vtx_decl) override;

	virtual void ResetBuffer(u32 stride, u32 count) override;

private:
	virtual void vFlush(bool useDstAlpha) override;
	void PrepareDrawBuffers(u32 stride);

	void StartCommandBuffer();

	void SignalFence(VkFence fence);
	void SetFence(VkFence fence);

	//This are the initially requested size for the buffers expressed in bytes
	const u32 MAX_IBUFFER_SIZE = 2 * 1024 * 1024;
	const u32 MAX_VBUFFER_SIZE = 32 * 1024 * 1024;

	std::unique_ptr<MappedBuffer> vertexBuffer;
	std::unique_ptr<MappedBuffer> indexBuffer;

	VkCommandBuffer cmdBuffer; // Currently recording command Buffer
	VkPipeline currentPipeline;
};

}