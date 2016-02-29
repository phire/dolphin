// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include "Common/VK/VkLoader.h"

namespace VK
{

class MappedBuffer
{
public:
	MappedBuffer(VkBuffer buffer) : m_buffer(buffer) {}

	// Get a pointer to a aligned sub-buffer which is at least size bytes big
	// Might return a null pointer
	virtual std::pair<u8 *, u32> Reserve(u32 size, u32 alignment) = 0;

	// Mark size bytes as ready for the GPU
	virtual void Commit(u32 size) = 0;

	// Get the offset of the current sub-buffer
	virtual u32 getOffset() const = 0;

	// Returns true if this buffer would like SetFence to be called
	virtual bool WantFence() const = 0;

	// Provides a fence that will be signaled when all currently committed
	// bytes have been used by the GPU.
	virtual void SetFence(VkFence fence) = 0;

	// Frees parts of the buffer currently fenced off by fence.
	// fence should be a VkFence that was previously provided to SetFence
	// fences need to be signaled in the same order they were set.
	virtual void SignalFence(VkFence fence) = 0;

	VkBuffer m_buffer;
};

namespace MemoryAllocator
{
void Initilize(VkPhysicalDevice physicalDevice, VkDevice device);
void Shutdown();

std::unique_ptr<MappedBuffer> CreateStreamBuffer(u32 size, VkBufferUsageFlags bufferType);

}
}