
#include <algorithm>
#include <iostream>
#include <random>

#include "HelloVulkan.h"
#include "GBufferRenderer.h"

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

std::vector<VkAttachmentDescription> GBufferRenderer::GetAttachmentDescriptions() const
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
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	std::vector<VkAttachmentDescription> colorAttachments = { colorAttachment, colorAttachment, depthAttachment };
	colorAttachments[0].format = position.format;
	colorAttachments[1].format = normal.format;
	colorAttachments[2].format = depth.format;

	return colorAttachments;
}

std::vector<VkAttachmentReference> GBufferRenderer::GetAttachmentRefs() const
{
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::vector<VkAttachmentReference> colorAttachmentRefs = { colorAttachmentRef, colorAttachmentRef };
	colorAttachmentRefs[0].attachment = 0;
	colorAttachmentRefs[1].attachment = 1;

	return colorAttachmentRefs;
}

std::vector<VkImageView> GBufferRenderer::GetImageViews() const
{
	std::vector<VkImageView> imageviews = {
		position.view,
		normal.view,
		depth.view
	};

	return imageviews;
}

void GBufferRenderer::CreatePipeline(PipelineCreateInfo& pipelineCreateInfo, VkGraphicsPipelineCreateInfo& creatInfo)
{
	auto attributeDescriptoins = Vertex::getAttributeDescriptions({ Vertex::VertexComponent::Position, Vertex::VertexComponent::Normal, Vertex::VertexComponent::UV, Vertex::VertexComponent::Tangent });
	auto attributeDescriptionBindings = Vertex::getBindingDescription();

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

	auto colorAttachmentRefs = GetAttachmentRefs();
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
	for (auto colorAttachmentRef : colorAttachmentRefs)
		colorBlendAttachments.push_back(colorBlendAttachment);

	pipelineCreateInfo.colorBlending.pAttachments = colorBlendAttachments.data();
	pipelineCreateInfo.colorBlending.attachmentCount = (uint32_t)colorBlendAttachments.size();

	// Disable culling, so all faces contribute to shadows
	pipelineCreateInfo.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	pipelineCreateInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

	// Enable depth bias
	pipelineCreateInfo.rasterizer.depthBiasEnable = VK_FALSE;
	pipelineCreateInfo.rasterizer.depthBiasClamp = VK_FALSE;

	pipelineCreateInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

	pipelineCreateInfo.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	pipelineCreateInfo.dynamicState.dynamicStateCount = 2;
	pipelineCreateInfo.dynamicState.pDynamicStates = dynamicStates;

	auto shaderStages = vulkanAPP->CreaterShader(vertexShader, fragmentShader);
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
	std::vector<VkAttachmentDescription> colorAttachments = GetAttachmentDescriptions();

	std::vector<VkAttachmentReference> colorAttachmentRefs = GetAttachmentRefs();

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 2;
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

void GBufferRenderer::CreateDescriptSetLayout()
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

void GBufferRenderer::SetupDescriptSet(VkDescriptorPool pool)
{

}

void GBufferRenderer::CreateGBuffer()
{
	CreateAttachment(&position, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	CreateAttachment(&normal, VK_FORMAT_R8G8B8A8_SNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	CreateAttachment(&depth, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	depth.descriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
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
	vulkanAPP->createTextureSampler(attachment->sampler, VK_FILTER_NEAREST, VK_FILTER_NEAREST, 1, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	attachment->UpdateDescriptor();
}

void GBufferRenderer::CreateFrameBuffer()
{
	std::vector<VkImageView> attachments = GetImageViews();

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

void GBufferRenderer::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = width;
	renderPassInfo.renderArea.extent.height = height;

	std::vector<VkClearValue> clearValues(4);
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[2].depthStencil = { 1.0f, 0 };
	clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

	renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)width;
	viewport.height = (float)height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent.width = width;
	scissor.extent.height = height;

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

	renderPassInfo.framebuffer = framebuffer;
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	gltfmodel.draw(commandBuffer, pipelineLayout, 0, 1);
	vkCmdEndRenderPass(commandBuffer);
}

void GBufferRenderer::UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos, float zNear, float zFar)
{

}

void GBufferRenderer::Cleanup()
{
	position.Cleanup(device);
	normal.Cleanup(device);
	depth.Cleanup(device);

	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkFreeMemory(device, uniformMemory, nullptr);

	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroyFramebuffer(device, framebuffer, nullptr);
}
