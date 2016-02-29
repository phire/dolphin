// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "VideoBackends/VK/PipelineCache.h"
#include "VideoBackends/VK/VertexManager.h"

#include "VideoCommon/GeometryShaderGen.h"
#include "VideoCommon/IndexGenerator.h"
#include "VideoCommon/NativeVertexFormat.h"
#include "VideoCommon/PixelShaderGen.h"
#include "VideoCommon/Statistics.h"
#include "VideoCommon/VertexLoaderManager.h"
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
		//for (int i = 0; i < 2; i++)
		//	AddAttributeFormat(bindings, vtx_decl.colors[i], 5 + i);
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

VertexManager::VertexManager()
{

	vertexBuffer = MemoryAllocator::CreateStreamBuffer(MAX_VBUFFER_SIZE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	indexBuffer = MemoryAllocator::CreateStreamBuffer(MAX_IBUFFER_SIZE, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	cmdBuffer = VK_NULL_HANDLE;

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
	g_pipeline_cache->GetPipeline(GetVertexShaderUid(), GetGeometryShaderUid(current_primitive_type), GetPixelShaderUid(DSTALPHA_DUAL_SOURCE_BLEND),
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

	vkCmdDrawIndexed(cmdBuffer, IndexGenerator::GetNumVerts(), 1, indexOffset, vertexOffset, 0);
}

void VertexManager::StartCommandBuffer()
{
	vkCmdBindIndexBuffer(cmdBuffer, indexBuffer->m_buffer, 0, VK_INDEX_TYPE_UINT16);
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer->m_buffer, offsets);
}

}