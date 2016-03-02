// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <vector>

#include "Common/VK/VkLoader.h"

#include "VideoCommon/PixelShaderGen.h"
#include "VideoCommon/VertexShaderGen.h"
#include "VideoCommon/GeometryShaderGen.h"

namespace VK
{

class PipelineCache
{
public:
	PipelineCache(VkDevice device);
	~PipelineCache();

	VkPipeline GetPipeline(VertexShaderUid vertex_uid, GeometryShaderUid geo_uid, PixelShaderUid pixel_uid, PrimitiveType type, std::vector<VkVertexInputAttributeDescription> vertAttrDesc, u32 stride);

	VkDescriptorSetLayout m_shadergen_descriptor_set;
	VkPipelineLayout m_shadergen_pipeline_layout;
	VkRenderPass m_renderPass;
private:
	VkShaderModule CompileShader(std::string shader, VkShaderStageFlagBits type) const;
	VkPipeline MakePipeline(VertexShaderUid vertex_uid, GeometryShaderUid geo_uid, PixelShaderUid pixel_uid, PrimitiveType type, std::vector<VkVertexInputAttributeDescription> vertAttrDesc, u32 stride);

	std::vector<VkPipeline> m_pipelines_to_destroy;
	VkDevice m_device;
};

extern std::unique_ptr<PipelineCache> g_pipeline_cache;

}