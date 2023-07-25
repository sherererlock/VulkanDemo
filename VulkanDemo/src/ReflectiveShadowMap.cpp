
#include <algorithm>
#include <iostream>
#include <random>

#include "HelloVulkan.h"
#include "ReflectiveShadowMap.h"

void ReflectiveShadowMap::InitRandomBuffer()
{
	std::default_random_engine e;
	std::uniform_real_distribution<float> u(0, 1);

	for (int i = 0; i < sampleCount; i++)
	{
		rubo.xi[i].x = u(e);
		rubo.xi[i].y = u(e);
		rubo.xi[i].z = radius / width;
	}

	void* data;
	vkMapMemory(device, runiformMemory, 0, sizeof(RandomUniformBufferObject), 0, &data);
	memcpy(data, &rubo, sizeof(RandomUniformBufferObject));
	vkUnmapMemory(device, runiformMemory);
}

VkDescriptorBufferInfo ReflectiveShadowMap::GetBufferInfo() const
{
	VkDescriptorBufferInfo bufferInfo = {};

	bufferInfo.buffer = runiformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(RandomUniformBufferObject);

	return bufferInfo;
}

void ReflectiveShadowMap::CreatePipeline(PipelineCreateInfo& pipelineCreateInfo, VkGraphicsPipelineCreateInfo& creatInfo)
{
	__super::CreatePipeline(pipelineCreateInfo, creatInfo);
	
}

void ReflectiveShadowMap::CreatePass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 采样数
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // 存储下来
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = VK_FORMAT_D32_SFLOAT;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	std::array<VkAttachmentDescription, 4> colorAttachments = { colorAttachment, colorAttachment, colorAttachment, depthAttachment };
	colorAttachments[0].format = position.format;
	colorAttachments[1].format = normal.format;
	colorAttachments[2].format = flux.format;
	colorAttachments[3].format = depth.format;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array<VkAttachmentReference, 3> colorAttachmentRefs = { colorAttachmentRef, colorAttachmentRef, colorAttachmentRef };
	colorAttachmentRefs[0].attachment = 0;
	colorAttachmentRefs[1].attachment = 1;
	colorAttachmentRefs[2].attachment = 2;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 3;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // 图像渲染的子流程
	subpass.colorAttachmentCount = (uint32_t)colorAttachmentRefs.size();
	subpass.pColorAttachments = colorAttachmentRefs.data(); // fragment shader使用 location = 0 outcolor,输出
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;

	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;   //  A subpass layout发生转换的阶段
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT; // A subpass 中需要完成的操作

	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; //  B subpass 等待执行的阶段
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // b subpass等待执行操作

	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = (uint32_t)colorAttachments.size();
	renderPassInfo.pAttachments = colorAttachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	renderPassInfo.dependencyCount = (uint32_t)dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

void ReflectiveShadowMap::CreateDescriptSetLayout()
{
	VkDescriptorSetLayoutBinding layoutBinding;
	layoutBinding.binding = 0;
	layoutBinding.descriptorCount = 1;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.pImmutableSamplers = nullptr;
	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow descriptor set layout!");
    }
}

void ReflectiveShadowMap::SetupDescriptSet(VkDescriptorPool pool)
{
    VkDescriptorSetLayout layouts[] = {descriptorSetLayout};
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;

    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to allocate descriptor set!");
    }

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

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

void ReflectiveShadowMap::CreateGBuffer()
{
	CreateAttachment(&position, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	CreateAttachment(&normal, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	CreateAttachment(&flux, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	CreateAttachment(&depth, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	depth.descriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
}

void ReflectiveShadowMap::CreateUniformBuffer()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformMemory);

	bufferSize = sizeof(RandomUniformBufferObject);
	vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, runiformBuffer, runiformMemory);
}

void ReflectiveShadowMap::CreateFrameBuffer()
{
	std::array<VkImageView, 4> attachments = {
		position.view,
		normal.view,
		flux.view,
		depth.view
	};

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create framebuffer!");
	}
}

void ReflectiveShadowMap::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent.width = width;
    renderPassInfo.renderArea.extent.height = height;

	std::vector<VkClearValue> clearValues(4);
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[3].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) width;
    viewport.height = (float) height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent.width = width;
    scissor.extent.height = height;

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdSetDepthBias(
		commandBuffer,
		depthBiasConstant,
		0.0f,
		depthBiasSlope);

    renderPassInfo.framebuffer = framebuffer;
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	gltfmodel.draw(commandBuffer, pipelineLayout, 0, 1);
	vkCmdEndRenderPass(commandBuffer);
}

void ReflectiveShadowMap::UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos, float zNear, float zFar)
{
	UniformBufferObject ubo;
	ubo.clipPlane.x = zNear;
	ubo.clipPlane.y = zFar;
	ubo.lightPos = lightPos;
	ubo.lightColor = glm::vec4(1.0f);
	ubo.depthVP = proj * view;

	void* data;
	vkMapMemory(device, uniformMemory, 0, sizeof(UniformBufferObject), 0, &data);
	memcpy(data, &ubo, sizeof(UniformBufferObject));
	vkUnmapMemory(device, uniformMemory);
}

void ReflectiveShadowMap::Cleanup()
{
	position.Cleanup(device);
	normal.Cleanup(device);
	flux.Cleanup(device);
	depth.Cleanup(device);

	vkDestroyBuffer(device, runiformBuffer, nullptr);
	vkFreeMemory(device, runiformMemory, nullptr);

	__super::Cleanup();
}
