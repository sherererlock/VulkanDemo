#include <stdexcept>

#include "HelloVulkan.h"
#include "SSRGbufferRenderer.h"
#include "Shadow.h"
#include "SSR.h"

void SSR::Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h)
{
	__super::Init(app, vkdevice, w, h);
	vertexShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/quad.vert.spv";
	fragmentShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/ssr.frag.spv";
	bufferSize = sizeof(UniformBufferObject);

	gbuffer = app->GetSSRGBuffer();
}

void SSR::CreateDescriptSetLayout()
{
	VkDescriptorSetLayoutBinding uniformLayoutBinding = {};

	uniformLayoutBinding.binding = 0;
	uniformLayoutBinding.descriptorCount = 1;
	uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	uniformLayoutBinding.pImmutableSamplers = nullptr;
	uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 6> uniformLayoutBindings;
	for (int i = 0; i < 6; i++)
	{
		uniformLayoutBindings[i] = uniformLayoutBinding;
		uniformLayoutBindings[i].binding = i;
	}

	uniformLayoutBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 6;
	layoutInfo.pBindings = uniformLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create SinglePipeline descriptor set layout!");
	}
}

void SSR::CreatePipeline(PipelineCreateInfo& pipelineCreateInfo, VkGraphicsPipelineCreateInfo& creatInfo)
{
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = { };
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	creatInfo.pVertexInputState = &vertexInputCreateInfo;

	pipelineCreateInfo.colorBlending.attachmentCount = 0;
	pipelineCreateInfo.rasterizer.cullMode = VK_CULL_MODE_NONE;
	pipelineCreateInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	pipelineCreateInfo.rasterizer.depthBiasEnable = false;

	pipelineCreateInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

	pipelineCreateInfo.multisampling.rasterizationSamples = vulkanAPP->GetSampleCountFlag();

	auto shaderStages = vulkanAPP->CreaterShader(vertexShader, fragmentShader);
	creatInfo.stageCount = 2;
	creatInfo.pStages = shaderStages.data();

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	pipelineCreateInfo.colorBlending.pAttachments = &colorBlendAttachment;
	pipelineCreateInfo.colorBlending.attachmentCount = 1;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	pipelineCreateInfo.dynamicState.dynamicStateCount = 2;
	pipelineCreateInfo.dynamicState.pDynamicStates = dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

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

void SSR::SetupDescriptSet(VkDescriptorPool pool)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate SinglePipeline descriptor set!");
	}

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = nullptr;
	descriptorWrite.pImageInfo = nullptr; // Optional
	descriptorWrite.pTexelBufferView = nullptr; // Optional

	std::array<VkWriteDescriptorSet, 6> descriptorWrites;
	descriptorWrites.fill(descriptorWrite);
	std::array<VkDescriptorImageInfo, 5> imageInfos =
	{
		gbuffer->GetPositionDescriptorImageInfo(),
		gbuffer->GetNormalDescriptorImageInfo(),
		gbuffer->GetDepthDescriptorImageInfo(),
		gbuffer->GetColorDescriptorImageInfo(),
		vulkanAPP->GetShadow()->GetDescriptorImageInfo()
	};

	for (int i = 0; i < 5; i++)
	{
		descriptorWrites[i].dstBinding = i;
		descriptorWrites[i].pImageInfo = &imageInfos[i];
	}

	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[5].pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(device, 6, descriptorWrites.data(), 0, nullptr);
}

void SSR::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdDraw(commandBuffer, 3, 1, 0,0);
}

void SSR::UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::mat4 depthVP, glm::vec4 viewPos)
{
	UniformBufferObject ubo;
	ubo.vp = proj * view;
	ubo.viewPos = viewPos;
	ubo.depthVP = depthVP;

	Trans_Data_To_GPU
}

