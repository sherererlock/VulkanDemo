#include <stdexcept>
#include <vulkan/vulkan.h>

#include "PreProcess.h"
#include "HelloVulkan.h"
#include "Texture.h"

#define M_PI       3.14159265358979323846   // pi

void PreProcess::generateIrradianceCube(HelloVulkan* vulkan, const TextureCubeMap& cubeMap, TextureCubeMap& irradianceMap)
{
	VkDevice device = vulkan->GetDevice();

	const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
	const uint32_t dim = 64;
	const uint32_t numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;

	vulkan->createImage(dim, dim, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, irradianceMap.image, irradianceMap.deviceMemory, numMips, VK_SAMPLE_COUNT_1_BIT, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
	vulkan->createImageView(irradianceMap.view, irradianceMap.image, format, VK_IMAGE_ASPECT_COLOR_BIT, numMips, VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, 6);
	vulkan->createTextureSampler(irradianceMap.sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, numMips, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	irradianceMap.updateDescriptor();

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 采样数
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // 存储下来

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;   //  A subpass layout发生转换的阶段
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT; // A subpass 中需要完成的操作

	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; //  B subpass 等待执行的阶段
	dependencies[0].dstAccessMask =  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // b subpass等待执行操作

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

	renderPassInfo.dependencyCount = (uint32_t)dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();

	VkRenderPass renderPass;
	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}

	struct offScreen
	{
		VkImage image;
		VkImageView imageView;
		VkDeviceMemory buffer;
		VkFramebuffer frameBuffer;
	};

	offScreen offscreen;
	{
		vulkan->createImage(dim, dim, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			offscreen.image, offscreen.buffer, 1, VK_SAMPLE_COUNT_1_BIT, 1);

		vulkan->transitionImageLayout(offscreen.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
		vulkan->createImageView(offscreen.imageView, offscreen.image, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &offscreen.imageView;
		framebufferInfo.width = dim;
		framebufferInfo.height = dim;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &offscreen.frameBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}

	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.binding = 0;
	layoutBinding.descriptorCount = 1;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBinding.pImmutableSamplers = nullptr;
	layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &layoutBinding;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    poolInfo.maxSets = 1;

	VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = nullptr;
	descriptorWrite.pImageInfo = &cubeMap.descriptor; // Optional
	descriptorWrite.pTexelBufferView = nullptr; // Optional

	vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

	struct PushConstBlock
	{
		glm::mat4 mvp;
		// Sampling deltas
		float deltaPhi = (2.0f * float(M_PI)) / 180.0f;
		float deltaTheta = (0.5f * float(M_PI)) / 64.0f;
	} pushConstBlock;

	VkPushConstantRange pushConstantRange;
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(pushConstBlock);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

	VkPipelineLayout pipelineLayout;
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	PipelineCreateInfo info = vulkan->CreatePipelineCreateInfo();

	info.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    info.depthStencil.depthTestEnable = VK_FALSE;
    info.depthStencil.depthWriteEnable = VK_FALSE;
    info.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    std::string vertexFileName = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/filtercube.vert.spv";
    std::string fragmentFileName = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/irradiancecube.frag.spv";

    auto shaderStages = vulkan->CreaterShader(vertexFileName, fragmentFileName);
    
	auto attributeDescriptoins = Vertex1::getAttributeDescriptions({Vertex1::VertexComponent::Position});
    auto attributeDescriptionBindings = Vertex1::getBindingDescription();

    info.vertexInputInfo.pVertexBindingDescriptions = &attributeDescriptionBindings; // Optional
    info.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptoins.size());
    info.vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptoins.data(); // Optional

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    info.dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    info.dynamicState.dynamicStateCount = 2;
    info.dynamicState.pDynamicStates = dynamicStates;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    info.colorBlending.pAttachments = &colorBlendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    pipelineInfo.stageCount = (uint32_t)shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();

    pipelineInfo.pVertexInputState = &info.vertexInputInfo; // bindings and attribute
    pipelineInfo.pInputAssemblyState = &info.inputAssembly; // topology
    pipelineInfo.pViewportState = &info.viewportState; 
    pipelineInfo.pRasterizationState = &info.rasterizer;
    pipelineInfo.pMultisampleState = &info.multisampling;
    pipelineInfo.pDepthStencilState = &info.depthStencil; // Optional
    pipelineInfo.pColorBlendState = &info.colorBlending;
    pipelineInfo.pDynamicState = &info.dynamicState; // Optional

    pipelineInfo.layout = pipelineLayout; // uniform变量

    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0; // 索引

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

	VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

	VkClearValue clearValue = {};
    clearValue.color = {0.0f, 0.0f, 0.0f, 0.0f};

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = offscreen.frameBuffer;

    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent.width = dim;
	renderPassBeginInfo.renderArea.extent.height = dim;

    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;

	std::vector<glm::mat4> matrices = {
		// POSITIVE_X
		glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_X
		glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// POSITIVE_Y
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_Y
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// POSITIVE_Z
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_Z
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	};

	VkCommandBuffer cmdBuffer = vulkan->beginSingleTimeCommands();

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = numMips;
	subresourceRange.layerCount = 6;

	vulkan->transitionImageLayout(irradianceMap.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, cmdBuffer);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
	scissor.extent.width = dim;
	scissor.extent.height = dim;

	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;

	for (uint32_t m = 0; m < numMips; m ++)
	{
		for (int f = 0; f < 6; f ++)
		{
			float w = static_cast<float>(dim * std::pow(0.5f, m));
			viewport.width = w;
			viewport.height = w;

			pushConstBlock.mvp = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 512.0f) * matrices[f];

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

			vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			vulkan->GetSkybox().draw(cmdBuffer, pipelineLayout, 2);

			vkCmdEndRenderPass(cmdBuffer);

			vulkan->transitionImageLayout(offscreen.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, cmdBuffer);

			// Copy region for transfer from framebuffer to cube face
			VkImageCopy copyRegion = {};

			copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.srcSubresource.baseArrayLayer = 0;
			copyRegion.srcSubresource.mipLevel = 0;
			copyRegion.srcSubresource.layerCount = 1;
			copyRegion.srcOffset = { 0, 0, 0 };

			copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.dstSubresource.baseArrayLayer = f;
			copyRegion.dstSubresource.mipLevel = m;
			copyRegion.dstSubresource.layerCount = 1;
			copyRegion.dstOffset = { 0, 0, 0 };

			copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
			copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
			copyRegion.extent.depth = 1;

			vkCmdCopyImage(
				cmdBuffer,
				offscreen.image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				irradianceMap.image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&copyRegion);

			// Transform framebuffer color attachment back
			vulkan->transitionImageLayout( offscreen.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subresourceRange, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, cmdBuffer);
		}
	}

	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = numMips;
	subresourceRange.layerCount = 6;
	vulkan->transitionImageLayout(irradianceMap.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, cmdBuffer);

	vulkan->endSingleTimeCommands(cmdBuffer);

    vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device, shaderStages[1].module, nullptr);

	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroyFramebuffer(device, offscreen.frameBuffer, nullptr);
	vkFreeMemory(device, offscreen.buffer, nullptr);
	vkDestroyImageView(device, offscreen.imageView, nullptr);
	vkDestroyImage(device, offscreen.image, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}


