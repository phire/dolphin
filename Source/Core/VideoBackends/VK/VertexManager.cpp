// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <cstring>

#include "Common/VK/VkLoader.h"

#include "VideoBackends/VK/PipelineCache.h"
#include "VideoBackends/VK/Renderer.h" // Only for g_quque, which is a hack
#include "VideoBackends/VK/VertexManager.h"

#include "VideoCommon/GeometryShaderGen.h"
#include "VideoCommon/IndexGenerator.h"
#include "VideoCommon/NativeVertexFormat.h"
#include "VideoCommon/PixelShaderManager.h"
#include "VideoCommon/PixelShaderGen.h"
#include "VideoCommon/Statistics.h"
#include "VideoCommon/VertexLoaderManager.h"
#include "VideoCommon/VertexShaderManager.h"
#include "VideoCommon/VertexShaderGen.h"


namespace VK
{

class VKVertexFormat : public NativeVertexFormat
{
public:
	VKVertexFormat(const PortableVertexDeclaration& vtx_decl)
	{
		this->vtx_decl = vtx_decl;
	}

	// This array is very close to the DX11 array,
	// just find/replace: "DXGI" with "VK", "UNKNOWN" with "UNDEFINED" and "FLOAT with "SFLOAT"
	static const constexpr VkFormat vk_format_lookup[5 * 4 * 2] =
	{
		// float formats
		VK_FORMAT_R8_UNORM, VK_FORMAT_R8_SNORM, VK_FORMAT_R16_UNORM, VK_FORMAT_R16_UNORM, VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8_SNORM, VK_FORMAT_R16G16_UNORM, VK_FORMAT_R16G16_UNORM, VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R32G32B32A32_SFLOAT,

		// integer formats
		VK_FORMAT_R8_UINT, VK_FORMAT_R8_SINT, VK_FORMAT_R16_UINT, VK_FORMAT_R16_SINT, VK_FORMAT_UNDEFINED,
		VK_FORMAT_R8G8_UINT, VK_FORMAT_R8G8_SINT, VK_FORMAT_R16G16_UINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED,
		VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_SINT, VK_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_UNDEFINED,
	};


	VkFormat VarToVkFormat(VarType t, int size, bool integer)
	{
		VkFormat retval = vk_format_lookup[static_cast<int>(t) + 5 * (size - 1) + 5 * 4 * static_cast<int>(integer)];
		if (retval == VK_FORMAT_UNDEFINED)
		{
			PanicAlert("VarToVkFormat: Invalid type/size combo %i , %i, %i", static_cast<int>(t), size, static_cast<int>(integer));
		}
		return retval;
	}

	void AddAttributeFormat(std::vector<VkVertexInputAttributeDescription>& bindings, const AttributeFormat& format, unsigned int location)
	{
		if (!format.enable)
			return;

		VkVertexInputAttributeDescription desc = {};

		desc.location = location;
		desc.binding = 0; // We only have the one vertex binding
		desc.format = VarToVkFormat(format.type, format.components, format.integer);
		desc.offset = format.offset;

		bindings.push_back(desc);
	}

	std::vector<VkVertexInputAttributeDescription> GetAttributeDescription()
	{
		std::vector<VkVertexInputAttributeDescription> bindings;

		AddAttributeFormat(bindings, vtx_decl.position, 0);
		AddAttributeFormat(bindings, vtx_decl.posmtx, 1);
		for (int i = 0; i < 3; i++)
			AddAttributeFormat(bindings, vtx_decl.normals[i], 2 + i);
		for (int i = 0; i < 2; i++)
			AddAttributeFormat(bindings, vtx_decl.colors[i], 5 + i);
		for (int i = 0; i < 8; i++)
			AddAttributeFormat(bindings, vtx_decl.texcoords[i], 7 + i);

		return bindings;
	}

	void SetupVertexPointers() {}
};

NativeVertexFormat *VertexManager::CreateNativeVertexFormat(const PortableVertexDeclaration& vtx_decl)
{
	return new VKVertexFormat(vtx_decl);
}

VertexManager::VertexManager(VkDevice device) : m_device(device)
{

	vertexBuffer = MemoryAllocator::CreateStreamBuffer(MAX_VBUFFER_SIZE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	indexBuffer = MemoryAllocator::CreateStreamBuffer(MAX_IBUFFER_SIZE, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	uniformBuffer = MemoryAllocator::CreateStreamBuffer(UBO_LENGTH, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	cmdBuffer = VK_NULL_HANDLE;

	VkDescriptorPoolSize sizes[2];
	sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	sizes[0].descriptorCount = 20;
	sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sizes[1].descriptorCount = 80;

	VkDescriptorPoolCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr };
	info.flags = 0;
	info.maxSets = 10;
	info.poolSizeCount = 2;
	info.pPoolSizes = sizes;
	vkCreateDescriptorPool(m_device, &info, nullptr, &m_descpriptor_pool);

	VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr };
	allocateInfo.descriptorPool = m_descpriptor_pool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &g_pipeline_cache->m_shadergen_descriptor_set;

	VkResult result = vkAllocateDescriptorSets(m_device, &allocateInfo, &m_descriptor_set);

	VkDescriptorBufferInfo VertBufferInfo;
	VertBufferInfo.buffer = uniformBuffer->m_buffer;
	VertBufferInfo.offset = 0;
	VertBufferInfo.range = sizeof(VertexShaderConstants);

	VkDescriptorBufferInfo PixelBufferInfo;
	PixelBufferInfo.buffer = uniformBuffer->m_buffer;
	PixelBufferInfo.offset = 0;
	PixelBufferInfo.range = sizeof(PixelShaderConstants);

	VkWriteDescriptorSet writes[3];
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].pNext = nullptr;
	writes[0].dstSet = m_descriptor_set;
	writes[0].dstBinding = 2;
	writes[0].dstArrayElement = 0;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writes[0].pBufferInfo = &VertBufferInfo;
	writes[0].pTexelBufferView = nullptr;
	writes[0].pImageInfo = nullptr;
	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].pNext = nullptr;
	writes[1].dstSet = m_descriptor_set;
	writes[1].dstBinding = 1;
	writes[1].dstArrayElement = 0;
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writes[1].pBufferInfo = &PixelBufferInfo;
	writes[1].pTexelBufferView = nullptr;
	writes[1].pImageInfo = nullptr;
	vkUpdateDescriptorSets(m_device, 2, writes, 0, nullptr);

	_assert_msg_(VIDEO, result == VK_SUCCESS, "Creation of Descriptor sets failed");

	// Pre-create 16 fences, we can create more if needed
	//VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr };
	//fenceInfo.flags = 0;
	//for (int i = 0; i < numFences; i++)
	//{
	//	VkFence fence;
	//	vkCreateFence(s_device, &fenceInfo, nullptr, &fence);
	//	m_spare_fences.push_back(fence);
	//}
}

VertexManager::~VertexManager()
{
	vkDestroyDescriptorPool(m_device, m_descpriptor_pool, nullptr);
}

void VertexManager::ResetBuffer(u32 stride, u32 count)
{
	std::pair<u8*, u32> vert = vertexBuffer->Reserve(stride * count, stride);
	std::pair<u8*, u32> index = indexBuffer->Reserve(MAXIBUFFERSIZE * sizeof(u16), sizeof(u16)); // TODO: make index generator work with minimum sizes.

	while (vert.first == nullptr || index.first == nullptr)
	{
		// TODO: advance queue

		// TODO: check fences

		vert = vertexBuffer->Reserve(stride * count, stride);
		index = indexBuffer->Reserve(MAXIBUFFERSIZE * sizeof(u16), sizeof(u16));
	}

	s_pCurBufferPointer = s_pBaseBufferPointer = vert.first;
	s_pEndBufferPointer = vert.first + vert.second;

	IndexGenerator::Start(reinterpret_cast<u16*>(index.first));


	// TODO: Combine current state stuff into a single "Current State UID"
	// TODO: And maybe combine all 4 into one master UID?
	VKVertexFormat* nativeVertexFmt = (VKVertexFormat*)VertexLoaderManager::GetCurrentVertexFormat();
	currentPipeline = g_pipeline_cache->GetPipeline(GetVertexShaderUid(), GetGeometryShaderUid(current_primitive_type), GetPixelShaderUid(DSTALPHA_DUAL_SOURCE_BLEND),
	                                                current_primitive_type, nativeVertexFmt->GetAttributeDescription(), nativeVertexFmt->GetVertexStride());
}

void VertexManager::PrepareDrawBuffers(u32 stride)
{
	u32 vertex_data_size = IndexGenerator::GetNumVerts() * stride;
	u32 index_data_size = IndexGenerator::GetIndexLen() * sizeof(u16);

	vertexBuffer->Commit(vertex_data_size);
	indexBuffer->Commit(index_data_size);

	ADDSTAT(stats.thisFrame.bytesVertexStreamed, vertex_data_size);
	ADDSTAT(stats.thisFrame.bytesIndexStreamed, index_data_size);
}

void VertexManager::SignalFence(VkFence fence)
{
	vertexBuffer->SignalFence(fence);
	indexBuffer->SignalFence(fence);
}

void VertexManager::SetFence(VkFence fence)
{
	vertexBuffer->SetFence(fence);
	indexBuffer->SetFence(fence);
}

//void VertexManager::CheckFences(u64 timeout)
//{
	//while(vkWaitForFences(s_device, 1, &s_active_fences.front(), VK_TRUE, timeout) == VK_SUCCESS)
//}

void VertexManager::vFlush(bool dstAlpha)
{
	VKVertexFormat *nativeVertexFmt = (VKVertexFormat*)VertexLoaderManager::GetCurrentVertexFormat();
	u32 stride = nativeVertexFmt->GetVertexStride();

	u32 vertexOffset = vertexBuffer->getOffset();
	u32 indexOffset = indexBuffer->getOffset();

	PrepareDrawBuffers(stride);

	// TODO: the only thing that actually has to be done in vFlush is the CmdDrawIndexed

	// TODO: only upload uniform buffers if they have changed
	std::pair<u8*, u32> vert = uniformBuffer->Reserve(sizeof(VertexShaderConstants), uniformBuffer->m_alignment);
	_assert_(vert.first != nullptr);
	u32 VertexUniformOffset = uniformBuffer->getOffset();
	std::memcpy(vert.first, &VertexShaderManager::constants, sizeof(VertexShaderConstants));
	uniformBuffer->Commit(sizeof(VertexShaderConstants));

	std::pair<u8*, u32> pixel = uniformBuffer->Reserve(sizeof(PixelShaderConstants), uniformBuffer->m_alignment);
	_assert_(pixel.first != nullptr);
	u32 PixelUniformOffset = uniformBuffer->getOffset();
	std::memcpy(pixel.first, &PixelShaderManager::constants, sizeof(PixelShaderConstants));
	uniformBuffer->Commit(sizeof(PixelShaderConstants));

	// TODO: pipelines and descriptor sets don't need to change every draw call.
	u32 dynamicUniformOffsets[2] = { PixelUniformOffset, VertexUniformOffset };
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline_cache->m_shadergen_pipeline_layout, 0, 1, &m_descriptor_set, 2, dynamicUniformOffsets);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline);
	vkCmdDrawIndexed(cmdBuffer, IndexGenerator::GetNumVerts(), 1, indexOffset / sizeof(u16), vertexOffset / stride, 0);
}

void VertexManager::StartCommandBuffer()
{
	vkCmdBindIndexBuffer(cmdBuffer, indexBuffer->m_buffer, 0, VK_INDEX_TYPE_UINT16);
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer->m_buffer, offsets);
}

}