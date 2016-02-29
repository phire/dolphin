// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include "Common/VK/VkLoader.h"

#include "VideoCommon/PixelShaderGen.h"
#include "VideoCommon/VertexShaderGen.h"
#include "VideoCommon/GeometryShaderGen.h"

namespace VK
{

class PipelineCache
{
public:
	PipelineCache(VkDevice device) : m_device(device) {};

	VkPipeline GetPipeline(VertexShaderUid vertex_uid, GeometryShaderUid geo_uid, PixelShaderUid pixel_uid, PrimitiveType type, std::vector<VkVertexInputAttributeDescription> vertAttrDesc, u32 stride);

private:
	VkShaderModule CompileShader(std::string shader, VkShaderStageFlagBits type) const;
	VkPipeline MakePipeline(VertexShaderUid vertex_uid, GeometryShaderUid geo_uid, PixelShaderUid pixel_uid, PrimitiveType type, std::vector<VkVertexInputAttributeDescription> vertAttrDesc, u32 stride) const;

	VkDevice m_device;
};

extern std::unique_ptr<PipelineCache> g_pipeline_cache;

}