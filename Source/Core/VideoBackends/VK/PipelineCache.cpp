// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "VideoBackends/VK/CompileGlsl.h"
#include "VideoBackends/VK/PipelineCache.h"

#include "glslang/SPIRV/disassemble.h"
#include "glslang/SPIRV/GlslangToSpv.h"

#include "Common/FileUtil.h"


namespace VK
{

std::unique_ptr<PipelineCache> g_pipeline_cache;

std::string s_glsl_header =
"#version 450\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"#extension GL_ARB_shading_language_420pack : enable\n"

"#define float2 vec2\n"
"#define float3 vec3\n"
"#define float4 vec4\n"
"#define uint2 uvec2\n"
"#define uint3 uvec3\n"
"#define uint4 uvec4\n"
"#define int2 ivec2\n"
"#define int3 ivec3\n"
"#define int4 ivec4\n"

// hlsl to glsl function translation
"#define frac fract\n"
"#define lerp mix\n"

"#define SAMPLER_BINDING(x) layout(set = 0, binding = x)\n\n";

VkShaderModule PipelineCache::CompileShader(std::string shader, VkShaderStageFlagBits type) const
{
	std::vector<unsigned int> spirv = CompileGlslToSpirv(s_glsl_header + shader, type);

	std::string filename = type == VK_SHADER_STAGE_FRAGMENT_BIT ? "fragment.spv" : "vertex.spv";

	//glslang::OutputSpv(spirv, filename.c_str());

	//std::ofstream out(filename);
	//spv::Disassemble(out, spirv);
	//out.close();

	VkShaderModule module;

	VkShaderModuleCreateInfo info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0 };
	info.pCode = spirv.data();
	info.codeSize = spirv.size() * sizeof(unsigned int);
	vkCreateShaderModule(m_device, &info, nullptr, &module);

	return module;
}

VkPipeline PipelineCache::MakePipeline(VertexShaderUid vertex_uid, GeometryShaderUid geo_uid, PixelShaderUid pixel_uid, PrimitiveType type, std::vector<VkVertexInputAttributeDescription> vertAttrDesc, u32 stride)
{
	if (!m_pipelines_to_destroy.empty())
	{
		return m_pipelines_to_destroy[0];
	}
	// Generate GLSL shaders and convert them to SPIR-V
	// TODO: cache these modules too?
	VkShaderModule pixelModule = CompileShader(GeneratePixelShaderCode(DSTALPHA_NONE, API_OPENGL, pixel_uid.GetUidData()).GetBuffer(), VK_SHADER_STAGE_FRAGMENT_BIT);
	VkShaderModule vertexModule = CompileShader(GenerateVertexShaderCode(API_OPENGL, vertex_uid.GetUidData()).GetBuffer(), VK_SHADER_STAGE_VERTEX_BIT);

	VkPipelineShaderStageCreateInfo shaderstage[2];
	shaderstage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderstage[0].pNext = nullptr;
	shaderstage[0].pName = "main";
	shaderstage[0].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderstage[0].module = pixelModule;
	shaderstage[0].pSpecializationInfo = nullptr;
	shaderstage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderstage[1].pNext = nullptr;
	shaderstage[1].pName = "main";
	shaderstage[1].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderstage[1].module = vertexModule;
	shaderstage[1].pSpecializationInfo = nullptr;

	VkVertexInputBindingDescription inputBinding;
	inputBinding.binding = 0;
	inputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	inputBinding.stride = stride;

	VkPipelineVertexInputStateCreateInfo vertexInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0 };
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &inputBinding;
	vertexInput.vertexAttributeDescriptionCount = static_cast<u32>(vertAttrDesc.size());
	vertexInput.pVertexAttributeDescriptions = vertAttrDesc.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0 };
	switch (type)
	{
	case PRIMITIVE_POINTS:
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		inputAssembly.primitiveRestartEnable = false;
		break;
	case PRIMITIVE_LINES:
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		inputAssembly.primitiveRestartEnable = false;
		break;
	case PRIMITIVE_TRIANGLES:
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = false;
	}

	// TODO: HARDCODED
	VkPipelineViewportStateCreateInfo viewportInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, 0 };
	viewportInfo.viewportCount = 1;
	VkViewport viewport;
	viewport.height = 480;
	viewport.width = 640;
	viewport.x = 0;
	viewport.y = 48;
	viewport.maxDepth = 1.0;
	viewport.minDepth = 0.0;
	viewportInfo.pViewports = &viewport;
	viewportInfo.scissorCount = 1;
	VkRect2D scissor;
	scissor.extent = { 640, 528 };
	scissor.offset = { 0, 48 };
	viewportInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, 0 };
	rasterInfo.depthClampEnable = false;
	rasterInfo.rasterizerDiscardEnable = false;
	rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterInfo.cullMode = VK_CULL_MODE_NONE;
	rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterInfo.depthBiasEnable = false;
	rasterInfo.lineWidth = 1.0; // TODO: Use wide lines

	VkPipelineMultisampleStateCreateInfo multisampleInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0 };
	multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleInfo.sampleShadingEnable = false;
	multisampleInfo.minSampleShading = 1;
	multisampleInfo.pSampleMask = nullptr;
	multisampleInfo.alphaToCoverageEnable = false;
	multisampleInfo.alphaToOneEnable = false;

	VkPipelineDepthStencilStateCreateInfo depthInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0 };
	depthInfo.depthTestEnable = false;
	depthInfo.depthWriteEnable = false;
	depthInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	depthInfo.depthBoundsTestEnable = false;
	depthInfo.stencilTestEnable = false;
	depthInfo.back.failOp = VK_STENCIL_OP_KEEP;
	depthInfo.back.passOp = VK_STENCIL_OP_KEEP;
	depthInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthInfo.front = depthInfo.back;

	VkPipelineColorBlendAttachmentState attachment;
	attachment.blendEnable = false;
	attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo blendInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0 };
	blendInfo.logicOpEnable = false;
	blendInfo.attachmentCount = 1;
	blendInfo.pAttachments = &attachment;

	VkPipelineDynamicStateCreateInfo dynamicInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0 };
	dynamicInfo.dynamicStateCount = 0;
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_BLEND_CONSTANTS,
	};
	dynamicInfo.pDynamicStates = dynamicStates;

	VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr };
	pipelineInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderstage;
	pipelineInfo.pVertexInputState = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pViewportState = &viewportInfo;
	pipelineInfo.pRasterizationState = &rasterInfo;
	pipelineInfo.pMultisampleState = &multisampleInfo;
	pipelineInfo.pDepthStencilState = &depthInfo;
	pipelineInfo.pColorBlendState = &blendInfo;
	pipelineInfo.pDynamicState = &dynamicInfo;
	pipelineInfo.layout = m_shadergen_pipeline_layout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0;

	VkPipeline pipeline;
	VkResult result = vkCreateGraphicsPipelines(m_device, nullptr, 1, &pipelineInfo, nullptr, &pipeline);

	_assert_msg_(VIDEO, result == VK_SUCCESS, "Error %i while creating Graphics Pipeline.", result);

	vkDestroyShaderModule(m_device, pixelModule, nullptr);
	vkDestroyShaderModule(m_device, vertexModule, nullptr);
	m_pipelines_to_destroy.push_back(pipeline);

	return pipeline;
}

VkPipeline PipelineCache::GetPipeline(VertexShaderUid vertex_uid, GeometryShaderUid geo_uid, PixelShaderUid pixel_uid, PrimitiveType type, std::vector<VkVertexInputAttributeDescription> vertAttrDesc, u32 stride)
{
	return MakePipeline(vertex_uid, geo_uid, pixel_uid, type, vertAttrDesc, stride); // TODO: actually cache things
}

PipelineCache::PipelineCache(VkDevice device) : m_device(device)
{
	// Generic descriptor set and layout that all shadergen shaders use.
	VkDescriptorSetLayoutCreateInfo descSetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0 };
	descSetInfo.bindingCount = 2;
	VkDescriptorSetLayoutBinding binding[3];
	binding[0].binding = 1;
	binding[0].descriptorCount = 1;
	binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	binding[0].pImmutableSamplers = nullptr;
	binding[1].binding = 2;
	binding[1].descriptorCount = 1;
	binding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	binding[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	binding[1].pImmutableSamplers = nullptr;
	binding[2].binding = 0;
	binding[2].descriptorCount = 8;
	binding[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	binding[2].pImmutableSamplers = nullptr;

	descSetInfo.pBindings = binding;

	VkResult result = vkCreateDescriptorSetLayout(m_device, &descSetInfo, nullptr, &m_shadergen_descriptor_set);
	_assert_msg_(VIDEO, result == VK_SUCCESS, "Error %i while creating Descriptor Set Layout", result);

	VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0 };
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &m_shadergen_descriptor_set;
	layoutInfo.pushConstantRangeCount = 0;
	result = vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_shadergen_pipeline_layout);
	_assert_msg_(VIDEO, result == VK_SUCCESS, "Error %i while creating Pipeline Layout", result);

	// Render Pass

	VkAttachmentDescription rp_attachment;
	rp_attachment.flags = 0;
	rp_attachment.format = VK_FORMAT_B8G8R8A8_UNORM;
	rp_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	rp_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	rp_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	rp_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	rp_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	rp_attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	rp_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachment = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpass = { 0 };
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachment;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = nullptr;
	subpass.preserveAttachmentCount = 0;

	VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0 };
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &rp_attachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 0;

	vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
}

PipelineCache::~PipelineCache()
{
	for (VkPipeline pipeline : m_pipelines_to_destroy)
		vkDestroyPipeline(m_device, pipeline, nullptr);

	vkDestroyRenderPass(m_device, m_renderPass, nullptr);
	vkDestroyPipelineLayout(m_device, m_shadergen_pipeline_layout, nullptr);
	vkDestroyDescriptorSetLayout(m_device, m_shadergen_descriptor_set, nullptr);
}

}
