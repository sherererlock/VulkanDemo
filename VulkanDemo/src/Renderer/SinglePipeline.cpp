#include <stdexcept>

#include "HelloVulkan.h"
#include "SinglePipeline.h"

void SinglePipeline::Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h)
{
	vulkanAPP = app;
	device = vkdevice;
	width = w;
	height = h;
}

void SinglePipeline::CreateAttachment(FrameBufferAttachment* attachment, VkFormat format, VkImageUsageFlags usage)
{
	attachment->format = format;

	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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
	vulkanAPP->createTextureSampler(attachment->sampler, VK_FILTER_NEAREST, VK_FILTER_NEAREST, 1, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	attachment->UpdateDescriptor();
}

void SinglePipeline::CreatePipeline(PipelineCreateInfo& pipelineCreateInfo, VkGraphicsPipelineCreateInfo& creatInfo)
{
	auto attributeDescriptoins = Vertex::getAttributeDescriptions();
	auto attributeDescriptionBindings = Vertex::getBindingDescription();
	pipelineCreateInfo.vertexInputInfo.pVertexBindingDescriptions = &attributeDescriptionBindings; // Optional
	pipelineCreateInfo.vertexInputInfo.vertexAttributeDescriptionCount = 1;
	pipelineCreateInfo.vertexInputInfo.pVertexAttributeDescriptions = &attributeDescriptoins[0]; // Optional

	creatInfo.pVertexInputState = &pipelineCreateInfo.vertexInputInfo;

	// No blend attachment states (no color attachments used)
	pipelineCreateInfo.colorBlending.attachmentCount = 0;

	// Disable culling, so all faces contribute to SinglePipelines
	pipelineCreateInfo.rasterizer.cullMode = VK_CULL_MODE_NONE;
	pipelineCreateInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

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

	auto shaderStages = vulkanAPP->CreaterShader(vertexShader, fragmentShader);
	creatInfo.stageCount = 2;
	creatInfo.pStages = shaderStages.data();

	//std::array<VkPushConstantRange, 1> pushConstantRanges;
	//pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	//pushConstantRanges[0].offset = 0;
	//pushConstantRanges[0].size = sizeof(primitiveInfo);

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

void SinglePipeline::CreateDescriptSetLayout()
{
	VkDescriptorSetLayoutBinding uniformLayoutBinding = {};

	uniformLayoutBinding.binding = 0;
	uniformLayoutBinding.descriptorCount = 1;
	uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformLayoutBinding.pImmutableSamplers = nullptr;
	uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uniformLayoutBinding;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create SinglePipeline descriptor set layout!");
	}
}

void SinglePipeline::SetupDescriptSet(VkDescriptorPool pool)
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

	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = bufferSize;

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo;
	descriptorWrite.pImageInfo = nullptr; // Optional
	descriptorWrite.pTexelBufferView = nullptr; // Optional

	vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
}

void SinglePipeline::CreateUniformBuffer()
{
	if(bufferSize == 0)
		return;

	vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformMemory);

	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = bufferSize;
}

void SinglePipeline::Cleanup()
{
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkFreeMemory(device, uniformMemory, nullptr);

	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}
