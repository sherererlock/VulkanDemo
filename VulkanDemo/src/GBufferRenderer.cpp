
#include <algorithm>
#include <iostream>
#include <random>

#include "HelloVulkan.h"
#include "GBufferRenderer.h"

void GBufferRenderer::InitRandomBuffer()
{
}

VkDescriptorBufferInfo GBufferRenderer::GetBufferInfo() const
{
	VkDescriptorBufferInfo bufferInfo = {};

	return bufferInfo;
}

void GBufferRenderer::Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h)
{
	width = w;
	height = h;
	device = vkdevice;
	vulkanAPP = app;
}

void GBufferRenderer::CreatePipeline(PipelineCreateInfo& pipelineCreateInfo, VkGraphicsPipelineCreateInfo& creatInfo)
{
	auto attributeDescriptoins = Vertex1::getAttributeDescriptions({ Vertex1::VertexComponent::Position, Vertex1::VertexComponent::Normal, Vertex1::VertexComponent::UV, Vertex1::VertexComponent::Tangent });
	auto attributeDescriptionBindings = Vertex1::getBindingDescription();

	pipelineCreateInfo.vertexInputInfo.pVertexBindingDescriptions = &attributeDescriptionBindings; // Optional
	pipelineCreateInfo.vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptoins.size();
	pipelineCreateInfo.vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptoins.data(); // Optional

	creatInfo.pVertexInputState = &pipelineCreateInfo.vertexInputInfo;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	std::array<VkPipelineColorBlendAttachmentState, 3> colorBlendAttachments = { colorBlendAttachment, colorBlendAttachment, colorBlendAttachment };

	pipelineCreateInfo.colorBlending.pAttachments = colorBlendAttachments.data();
	pipelineCreateInfo.colorBlending.attachmentCount = (uint32_t)colorBlendAttachments.size();

	// Disable culling, so all faces contribute to shadows
	pipelineCreateInfo.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	pipelineCreateInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

	// Enable depth bias
	pipelineCreateInfo.rasterizer.depthBiasEnable = true;

	pipelineCreateInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

	pipelineCreateInfo.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_DEPTH_BIAS
	};
	pipelineCreateInfo.dynamicState.dynamicStateCount = 3;
	pipelineCreateInfo.dynamicState.pDynamicStates = dynamicStates;

	auto shaderStages = vulkanAPP->CreaterShader("D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/rsmgbuffer.vert.spv", "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/rsmgbuffer.frag.spv");
	creatInfo.stageCount = 2;
	creatInfo.pStages = shaderStages.data();

	creatInfo.renderPass = renderPass;

	std::array<VkPushConstantRange, 1> pushConstantRanges;
	pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRanges[0].offset = 0;
	pushConstantRanges[0].size = sizeof(glm::mat4);

	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { descriptorSetLayout, vulkanAPP->GetMaterialDescriptorSetLayout() };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 2; // Optional
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data(); // Optional
	pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size(); // Optional
	pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data(); // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	creatInfo.layout = pipelineLayout;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &creatInfo, nullptr, &pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}

void GBufferRenderer::CreatePass()
{

}

void GBufferRenderer::CreateDescriptSetLayout()
{
}

void GBufferRenderer::SetupDescriptSet(VkDescriptorPool pool)
{
}

void GBufferRenderer::CreateGBuffer()
{
}

void GBufferRenderer::CreateAttachment(FrameBufferAttachment* attachment, VkFormat format, VkImageUsageFlagBits usage)
{
	attachment->format = format;

	VkImageAspectFlags aspectMask = 0;
	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	vulkanAPP->createImage(width, height, format, VK_IMAGE_TILING_OPTIMAL, usage | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachment->image, attachment->mem, 1, VK_SAMPLE_COUNT_1_BIT);
	vulkanAPP->createImageView(attachment->view, attachment->image, format, aspectMask, 1);
	vulkanAPP->createTextureSampler(attachment->sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, 1);

	attachment->UpdateDescriptor();
}

void GBufferRenderer::CreateUniformBuffer()
{
}

void GBufferRenderer::CreateFrameBuffer()
{
}

void GBufferRenderer::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
}

void GBufferRenderer::UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos, float zNear, float zFar)
{

}

void GBufferRenderer::Cleanup()
{
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkFreeMemory(device, uniformMemory, nullptr);

	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroyFramebuffer(device, framebuffer, nullptr);
}
