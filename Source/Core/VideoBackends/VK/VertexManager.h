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
	VertexManager(VkDevice device);
	~VertexManager();
	void StartCommandBuffer();

protected:
	NativeVertexFormat* CreateNativeVertexFormat(const PortableVertexDeclaration& vtx_decl) override;

	virtual void ResetBuffer(u32 stride, u32 count) override;


private:
	virtual void vFlush(bool useDstAlpha) override;
	void PrepareDrawBuffers(u32 stride);



	void SignalFence(VkFence fence);
	void SetFence(VkFence fence);

	//This are the initially requested size for the buffers expressed in bytes
	static const u32 MAX_IBUFFER_SIZE = 2 * 1024 * 1024;
	static const u32 MAX_VBUFFER_SIZE = 32 * 1024 * 1024;
	static const u32 UBO_LENGTH = 32 * 1024 * 1024;

	std::unique_ptr<MappedBuffer> vertexBuffer;
	std::unique_ptr<MappedBuffer> indexBuffer;
	std::unique_ptr<MappedBuffer> uniformBuffer;

	// TODO: some/all of these things should be moved somewhere else
	VkDevice m_device;
	VkDescriptorPool m_descpriptor_pool;
	VkDescriptorSet m_descriptor_set;
	VkPipeline currentPipeline;
};

}