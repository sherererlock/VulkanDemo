
#include <algorithm>
#include <iostream>

#include "HelloVulkan.h"
#include "SSRGBufferRenderer.h"
#include "GenHierarchicalDepth.h"

void GenHierarchicalDepth::CreateRenderpass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 采样数
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // 存储下来

    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0; // 引用的附件在数组中的索引
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // 图像渲染的子流程

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef; // fragment shader使用 location = 0 outcolor,输出
    subpass.pDepthStencilAttachment = nullptr;
    subpass.pResolveAttachments = nullptr;

	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;

	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;   //  A subpass layout发生转换的阶段
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT; // A subpass 中需要完成的操作

	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; //  B subpass 等待执行的阶段
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // b subpass等待执行操作

	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;;

	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void GenHierarchicalDepth::CreateFrameBuffer()
{
	framebuffers.resize(attachments.size());
	for (int i = 0; i < attachments.size(); i++)
	{
		FrameBufferAttachment& attachment = attachments[i];

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &attachment.view;
		framebufferInfo.width = attachment.width;
		framebufferInfo.height = attachment.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void GenHierarchicalDepth::CreateMipMap()
{
	const uint32_t numMips = static_cast<uint32_t>(std::max((floor(log2(width))) + 1, floor(log2( height))) + 1);
	uint32_t currentWidth = width;
	uint32_t currentHeight = height;

	for (uint32_t i = 0; i < numMips; i ++)
	{
		FrameBufferAttachment color;
		CreateAttachment(&color, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, currentWidth, currentHeight);
		attachments.push_back(color);

		if (currentWidth == 1 && currentHeight == 1)
			break;

		currentWidth = (uint32_t)std::floor(currentWidth / 2);
		currentHeight = (uint32_t)std::floor(currentHeight / 2);

		currentWidth = std::max((uint32_t)1, currentWidth);
		currentHeight = std::max((uint32_t)1, currentHeight);
	}

	uint32_t mip = (uint32_t)attachments.size();
	vulkanAPP->createImage(width, height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, hierarchicalDepth.image, hierarchicalDepth.deviceMemory, mip, VK_SAMPLE_COUNT_1_BIT);
	vulkanAPP->createImageView(hierarchicalDepth.view, hierarchicalDepth.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, mip);
	vulkanAPP->createTextureSampler(hierarchicalDepth.sampler, VK_FILTER_NEAREST, VK_FILTER_NEAREST, mip, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	hierarchicalDepth.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	hierarchicalDepth.updateDescriptor();
	hierarchicalDepth.device = device;
}

void GenHierarchicalDepth::CreateAttachment(FrameBufferAttachment* attachment, VkFormat format, VkImageUsageFlagBits usage, uint32_t width, uint32_t height)
{
	attachment->format = format;
	attachment->width = width;
	attachment->height = height;

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

	vulkanAPP->createImage(width, height, format, VK_IMAGE_TILING_OPTIMAL, usage| VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachment->image, attachment->mem, 1, VK_SAMPLE_COUNT_1_BIT);
	vulkanAPP->createImageView(attachment->view, attachment->image, format, aspectMask, 1);
	vulkanAPP->createTextureSampler(attachment->sampler, VK_FILTER_NEAREST, VK_FILTER_NEAREST, 1, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	attachment->UpdateDescriptor();
}

void GenHierarchicalDepth::CopyDepth(VkCommandBuffer commandBuffer)
{
	uint32_t mip = (uint32_t)attachments.size();
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = mip;
	subresourceRange.layerCount = 1;

	vulkanAPP->transitionImageLayout(commandBuffer, hierarchicalDepth.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

	VkImageCopy copyRegion = {};

	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.srcSubresource.baseArrayLayer = 0;
	copyRegion.srcSubresource.mipLevel = 0;
	copyRegion.srcSubresource.layerCount = 1;
	copyRegion.srcOffset = { 0, 0, 0 };

	subresourceRange.levelCount = 1;

	uint32_t currentWidth = width;
	uint32_t currentHeight = height;
	for (int i = 0; i < attachments.size(); i++)
	{
		vulkanAPP->transitionImageLayout(commandBuffer, attachments[i].image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);

		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.mipLevel = 0;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.srcOffset = { 0, 0, 0 };

		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.baseArrayLayer = 0;
		copyRegion.dstSubresource.mipLevel = i;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.dstOffset = { 0, 0, 0 };

		copyRegion.extent.width = currentWidth;
		copyRegion.extent.height = currentHeight;
		copyRegion.extent.depth = 1;

		vkCmdCopyImage(
			commandBuffer,
			attachments[i].image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			hierarchicalDepth.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&copyRegion);

		vulkanAPP->transitionImageLayout(commandBuffer, attachments[i].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subresourceRange);

		currentWidth = static_cast<uint32_t>(std::floor(currentWidth / 2));
		currentHeight = static_cast<uint32_t>(std::floor(currentHeight / 2));
		currentWidth = std::max((uint32_t)1, currentWidth);
		currentHeight = std::max((uint32_t)1, currentHeight);
	}

	subresourceRange.levelCount = mip;
	vulkanAPP->transitionImageLayout(commandBuffer, hierarchicalDepth.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
}

void GenHierarchicalDepth::CreateDescriptSetLayout()
{
	VkDescriptorSetLayoutBinding uniformLayoutBinding = {};

	uniformLayoutBinding.binding = 0;
	uniformLayoutBinding.descriptorCount = 1;
	uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	uniformLayoutBinding.pImmutableSamplers = nullptr;
	uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uniformLayoutBinding;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create SinglePipeline descriptor set layout!");
	}
}

void GenHierarchicalDepth::CreatePipeline(PipelineCreateInfo& pipelineCreateInfo, VkGraphicsPipelineCreateInfo& creatInfo)
{
	vertexShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/quad.vert.spv";
	fragmentShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/genDepthMinMip.frag.spv";

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = { };
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	creatInfo.pVertexInputState = &vertexInputCreateInfo;

	pipelineCreateInfo.colorBlending.attachmentCount = 0;
	pipelineCreateInfo.rasterizer.cullMode = VK_CULL_MODE_NONE;
	pipelineCreateInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	pipelineCreateInfo.rasterizer.depthBiasEnable = false;

	pipelineCreateInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

	pipelineCreateInfo.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

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

	VkPushConstantRange pushConstantRange;
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(int);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	creatInfo.layout = pipelineLayout;
	creatInfo.renderPass = renderPass;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &creatInfo, nullptr, &pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}

void GenHierarchicalDepth::SetupDescriptSet(VkDescriptorPool pool)
{
	std::array<VkDescriptorSetLayout, 12> descriptorSetLayouts;
	descriptorSetLayouts.fill(descriptorSetLayout);

	descriptorSets.resize(attachments.size());

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = (uint32_t)attachments.size();
	allocInfo.pSetLayouts = descriptorSetLayouts.data();
	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate SinglePipeline descriptor set!");
	}

	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = bufferSize;

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = nullptr;
	descriptorWrite.pImageInfo = nullptr; // Optional
	descriptorWrite.pTexelBufferView = nullptr; // Optional

	SSRGBufferRenderer* gbuffer = vulkanAPP->GetSSRGBuffer();
	descriptorWrite.dstSet = descriptorSets[0];
	descriptorWrite.pImageInfo = &gbuffer->GetDepthDescriptorImageInfo();

	vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

	for (int i = 1; i < descriptorSets.size(); i++)
	{
		descriptorWrite.dstSet = descriptorSets[i];
		descriptorWrite.pImageInfo = &attachments[i - 1].descriptor;

		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
	}
}

void GenHierarchicalDepth::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;

	VkClearValue clearValue;
	clearValue.color = {  0.0f, 0.0f, 0.0f, 1.0f };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearValue;

	uint32_t currentWidth = width;
	uint32_t currentHeight = height;

	renderPassInfo.renderArea.offset = { 0, 0 };

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent.width = currentWidth;
	scissor.extent.height = currentHeight;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	for (int i = 0; i < framebuffers.size(); i++)
	{
		renderPassInfo.renderArea.extent.width = currentWidth;
		renderPassInfo.renderArea.extent.height = currentHeight;
		viewport.width = (float)currentWidth;
		viewport.height = (float)currentHeight;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int), &i);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

		renderPassInfo.framebuffer = framebuffers[i];
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(commandBuffer);

		currentWidth = (uint32_t)std::floor(currentWidth / 2);
		currentHeight = (uint32_t)std::floor(currentHeight / 2);
		currentWidth = std::max((uint32_t)1, currentWidth);
		currentHeight = std::max((uint32_t)1, currentHeight);
	}

	CopyDepth(commandBuffer);
}

void GenHierarchicalDepth::Cleanup()
{
	__super::Cleanup();

	vkDestroyRenderPass(device, renderPass, nullptr);
	for(VkFramebuffer framebuffer : framebuffers)
		vkDestroyFramebuffer(device, framebuffer, nullptr);

	for (int i = 0; i < attachments.size(); i++)
		attachments[i].Cleanup(device);

	hierarchicalDepth.destroy();
}
