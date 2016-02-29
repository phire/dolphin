// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <memory>
#include <vector>
#include <deque>

#include "Common/Assert.h"
#include "Common/CommonTypes.h"
#include "Common/MathUtil.h"
#include "Common/Logging/Log.h"


#include "VideoBackends/VK/MemoryAllocator.h"


namespace VK
{

static VkDevice s_device;
static u32 s_device_local_memory_type;
static u32 s_streaming_memory_type;
static bool s_streaming_memory_is_coherent;

void MemoryAllocator::Initilize(VkPhysicalDevice physicalDevice, VkDevice device)
{
	s_device = device;
	VkPhysicalDeviceMemoryProperties properties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);

	// Memory types are ordered in optimal order so we can just take the first type with the properties we need

	// Find a device local memory heap/type
	for (u32 i = 0; i < properties.memoryTypeCount; i++)
	{
		if (properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		{
			s_device_local_memory_type = i;
			WARN_LOG(VIDEO, "Found %iMB of device memory at type index %i",
			         properties.memoryHeaps[properties.memoryTypes[i].heapIndex].size / 1024, i);
			break;
		}
	}

	// Find a memory heap/type we can stream vertices and uniforms to (might be the same heap as above)
	for (u32 i = 0; i < properties.memoryTypeCount; i++)
	{
		if (properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			s_streaming_memory_type = i;
			s_streaming_memory_is_coherent = !!(properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			WARN_LOG(VIDEO, "Found %iMB of %s streaming memory at type index %i",
			         properties.memoryHeaps[properties.memoryTypes[i].heapIndex].size / 1024,
					 s_streaming_memory_is_coherent ? "coherent" : "non-coherent", i);
			break;
		}
	}
}

void MemoryAllocator::Shutdown()
{
	// Todo, make sure all allocations have been freed.
}

static VkDeviceMemory Allocate(VkDeviceSize size, u32 type)
{
	VkDeviceMemory allocation;
	VkMemoryAllocateInfo info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr };
	info.allocationSize = size;
	info.memoryTypeIndex = type;
	VkResult res = vkAllocateMemory(s_device, &info, nullptr, &allocation);
	_assert_msg_(VIDEO, res == VK_SUCCESS, "Error %i while allocating memory", res); // TODO: try and recover?
	return allocation;
}

class StreamBuffer : MappedBuffer
{
public:
	StreamBuffer(VkBuffer buffer, u32 size, VkDeviceMemory memory, void *pointer) : MappedBuffer(buffer)
	{
		m_size = size;
		m_memory = memory;
		m_base_pointer = static_cast<u8*>(pointer);
		m_current_offset = m_fenced_bottom = 0;
	}
	~StreamBuffer()
	{
		vkUnmapMemory(s_device, m_memory);
		vkDestroyBuffer(s_device, m_buffer, nullptr);
	}

	// Returns a pointer/size to part of the buffer with at least size bytes
	// If we can't reserve enough bytes we will return (null, 0)
	std::pair<u8 *, u32> Reserve(u32 size, u32 alignment) override
	{
		m_current_offset = ROUND_UP(m_current_offset, alignment);

		// Looks like we can't reserve enough bytes, the caller should call WaitForFence()
		if (AvaliableBytes() < size)
			return std::make_pair(nullptr, 0);

		return std::make_pair(m_base_pointer + m_current_offset, AvaliableBytes());
	}

	// Mark size bytes as ready for the GPU
	// returns true if we think a fence would be a good idea.
	void Commit(u32 size) override
	{
		m_current_offset += size;
	}

	bool WantFence() const override
	{
		return UnfencedBytes() > m_size / numFences || (SpaceSpaceAtBottom() > AvaliableBytes() && UnfencedBytes() > 0);
	}

	void SetFence(VkFence fence) override
	{
		m_active_fences.push_back(std::make_pair(fence, m_current_offset));
		if (SpaceSpaceAtBottom() > AvaliableBytes())
			m_current_offset = 0;
	}

	void SignalFence(VkFence fence) override
	{
		_assert_msg_(VIDEO, fence == m_active_fences.front().first, "Fences not cleared in the correct order");
		m_fenced_bottom = m_active_fences.front().second;
		m_active_fences.pop_front();
		if (SpaceSpaceAtBottom() > AvaliableBytes() && UnfencedBytes() == 0)
			m_current_offset = 0;
	}

	u32 getOffset() const
	{
		return m_current_offset;
	}

private:
	u32 AvaliableBytes() const
	{
		if (m_fenced_bottom >= m_current_offset)
			return m_size - m_current_offset;
		else
			return m_fenced_bottom - m_current_offset;
	}

	u32 UnfencedBytes() const
	{
		if (m_active_fences.empty())
		{
			if (m_fenced_bottom <= m_current_offset)
				return m_current_offset - m_fenced_bottom;
			else
				return m_current_offset;
		}
		else
		{
			return m_current_offset - m_active_fences.back().second;
		}
	}

	u32 SpaceSpaceAtBottom() const
	{
		if (m_fenced_bottom <= m_current_offset)
			return m_fenced_bottom;
		return 0;
	}

	static const int numFences = 16; // I have no idea how many fences would be optimal
	VkDeviceMemory m_memory;
	u8 *m_base_pointer; // pointer to the mapping
	u32 m_size;
	u32 m_current_offset;
	std::deque<std::pair<VkFence, u32>> m_active_fences; // Fences we are waiting to hear back about
	u32 m_fenced_bottom ; // Lowest address currently covered by a fence
};

std::unique_ptr<MappedBuffer> MemoryAllocator::CreateStreamBuffer(u32 size, VkBufferUsageFlags bufferType)
{
	// TODO: implement code that works when streaming memory isn't coherent
	_assert_(s_streaming_memory_is_coherent);

	VkBuffer buffer;
	VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr };
	info.flags = 0;
	info.usage = bufferType;
	info.size = size;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateBuffer(s_device, &info, nullptr, &buffer);

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(s_device, buffer, &memRequirements);

	_assert_msg_(VIDEO, memRequirements.memoryTypeBits & 1 << s_streaming_memory_type,
	             "Memory type %i not suitable for %x, 0x%x", s_streaming_memory_type, bufferType, memRequirements.memoryTypeBits);
	VkDeviceMemory memory = Allocate(memRequirements.size, s_streaming_memory_type);

	vkBindBufferMemory(s_device, buffer, memory, 0);

	void *pointer;
	vkMapMemory(s_device, memory, 0, size, 0, &pointer);

	return std::unique_ptr<MappedBuffer>(new StreamBuffer(buffer, size, memory, pointer));
}



void Free(VkDeviceMemory allocation)
{
	vkFreeMemory(s_device, allocation, nullptr);
}

}