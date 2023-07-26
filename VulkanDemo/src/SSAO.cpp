#include <algorithm>
#include <iostream>
#include <random>
#include <random>

#include "HelloVulkan.h"
#include "SSAO.h"

#define SSAO_NOISE_DIM 4

float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

void SSAO::InitRandomBuffer()
{
	std::default_random_engine rndEngine( (unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

	for (uint32_t i = 0; i < SSAO_KERNEL_SIZE; ++i)
	{
		glm::vec3 sample(rndDist(rndEngine) * 2.0 - 1.0, rndDist(rndEngine) * 2.0 - 1.0, rndDist(rndEngine));
		sample = glm::normalize(sample);
		sample *= rndDist(rndEngine);
		float scale = float(i) / float(SSAO_KERNEL_SIZE);
		scale = lerp(0.1f, 1.0f, scale * scale);
		ubo.samples[i] = glm::vec4(sample * scale, 0.0f);
	}

	ubo.proj = vulkanAPP->GetCamera().matrices.perspective;

	void* data;
	vkMapMemory(device, ssaoPipeline.uniformMemory, 0, sizeof(SSAOUBO), 0, &data);
	memcpy(data, &ubo, sizeof(SSAOUBO));
	vkUnmapMemory(device, ssaoPipeline.uniformMemory);

	std::vector<glm::vec4> ssaoNoise(SSAO_NOISE_DIM * SSAO_NOISE_DIM);
	for (uint32_t i = 0; i < static_cast<uint32_t>(ssaoNoise.size()); i++)
	{
		ssaoNoise[i] = glm::vec4(rndDist(rndEngine) * 2.0f - 1.0f, rndDist(rndEngine) * 2.0f - 1.0f, 0.0f, 0.0f);
	}

	noiseMap.fromBuffer(vulkanAPP, ssaoNoise.data(), ssaoNoise.size() * sizeof(glm::vec4), VK_FORMAT_R32G32B32A32_SFLOAT, SSAO_NOISE_DIM, SSAO_NOISE_DIM);
}

VkDescriptorBufferInfo SSAO::GetBufferInfo() const
{
	return VkDescriptorBufferInfo();
}

void SSAO::CreatePipeline(PipelineCreateInfo& pipelineCreateInfo, VkGraphicsPipelineCreateInfo& creatInfo)
{
	vertexShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/ssaogbuffer.vert.spv";
	fragmentShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/ssaogbuffer.frag.spv";

	__super::CreatePipeline(pipelineCreateInfo, creatInfo);

	vertexShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/quad.vert.spv";
	fragmentShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/ssao.frag.spv";

	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
	pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	creatInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;

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

	pipelineCreateInfo.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineCreateInfo.rasterizer.cullMode = VK_CULL_MODE_NONE;
	pipelineCreateInfo.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	pipelineCreateInfo.dynamicState.dynamicStateCount = 2;
	pipelineCreateInfo.dynamicState.pDynamicStates = dynamicStates;

	auto shaderStages = vulkanAPP->CreaterShader(vertexShader, fragmentShader);

	struct SpecializationData
	{
		uint32_t kernelSize = SSAO_KERNEL_SIZE;
		float radius = SSAO_RADIUS;
	}specializationData;

	std::array<VkSpecializationMapEntry, 2> entries;

	entries[0].constantID = 0;
	entries[0].offset = offsetof(SpecializationData, kernelSize);
	entries[0].size = sizeof(SpecializationData::kernelSize);

	entries[1].constantID = 1;
	entries[1].offset = offsetof(SpecializationData, radius);
	entries[1].size = sizeof(SpecializationData::radius);

	VkSpecializationInfo specializationInfo{};
	specializationInfo.mapEntryCount = 2;
	specializationInfo.pMapEntries = entries.data();
	specializationInfo.dataSize = sizeof(specializationData);
	specializationInfo.pData = &specializationData;

	shaderStages[1].pSpecializationInfo = &specializationInfo;

	creatInfo.stageCount = (uint32_t)shaderStages.size();
	creatInfo.pStages = shaderStages.data();

	creatInfo.renderPass = ssaoFrameBuffer.renderPass;
	
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &ssaoPipeline.descriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &ssaoPipeline.pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	creatInfo.layout = ssaoPipeline.pipelineLayout;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &creatInfo, nullptr, &ssaoPipeline.pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}

void SSAO::CreatePass()
{
	__super::CreatePass();

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = ssaoFrameBuffer.color.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 采样数
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // 存储下来
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachmentRef.attachment = 0;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // 图像渲染的子流程
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef; // fragment shader使用 location = 0 outcolor,输出
	subpass.pDepthStencilAttachment = nullptr;

	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;

	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;   //  图形渲染管线的末尾阶段
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT; // A subpass 中需要完成的操作

	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; //  颜色附着输出阶段，将像素写入帧缓冲的颜色附着
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

	renderPassInfo.dependencyCount = (uint32_t)dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &ssaoFrameBuffer.renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

void SSAO::CreateDescriptSetLayout()
{
	__super::CreateDescriptSetLayout();

	VkDescriptorSetLayoutBinding layoutBinding;
	layoutBinding.binding = 0;
	layoutBinding.descriptorCount = 1;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.pImmutableSamplers = nullptr;
	layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 4> layoutBindings = { layoutBinding, layoutBinding, layoutBinding, layoutBinding };
	for (int i = 1; i < 4; i++)
	{
		layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindings[i].binding = i;
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = (uint32_t) layoutBindings.size();
	layoutInfo.pBindings = layoutBindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &ssaoPipeline.descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shadow descriptor set layout!");
	}
}

void SSAO::SetupDescriptSet(VkDescriptorPool pool)
{
	InitRandomBuffer();

	// gbuffer
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

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

	// ssao
	allocInfo.pSetLayouts = &ssaoPipeline.descriptorSetLayout;

	if (vkAllocateDescriptorSets(device, &allocInfo, &ssaoPipeline.descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	bufferInfo.buffer = ssaoPipeline.uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(SSAOUBO);

	descriptorWrite.dstSet = ssaoPipeline.descriptorSet;
	std::array<VkWriteDescriptorSet, 4> descriptorWrites = { descriptorWrite, descriptorWrite, descriptorWrite, descriptorWrite };
	for (int i = 1; i < 4; i++)
	{
		descriptorWrites[i].dstBinding = i;
		descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[i].pBufferInfo = nullptr;
	}

	descriptorWrites[1].pImageInfo = &position.descriptor; // Optional
	descriptorWrites[2].pImageInfo = &normal.descriptor; // Optional
	descriptorWrites[3].pImageInfo = &noiseMap.descriptor; // Optional

	vkUpdateDescriptorSets(device, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void SSAO::CreateGBuffer()
{
	__super::CreateGBuffer();

	CreateAttachment(&ssaoFrameBuffer.color, VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
}

void SSAO::CreateUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformMemory);

	bufferSize = sizeof(SSAOUBO);
	vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ssaoPipeline.uniformBuffer, ssaoPipeline.uniformMemory);
}

void SSAO::CreateFrameBuffer()
{
	__super::CreateFrameBuffer();

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = ssaoFrameBuffer.renderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &ssaoFrameBuffer.color.view;
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &ssaoFrameBuffer.frameBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create framebuffer!");
	}
}

void SSAO::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
	// gbuffer
	__super::BuildCommandBuffer(commandBuffer, gltfmodel);

	BuildSSAOCommandBuffer(commandBuffer);
}

void SSAO::BuildSSAOCommandBuffer(VkCommandBuffer commandBuffer)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = ssaoFrameBuffer.renderPass;
	renderPassInfo.framebuffer = ssaoFrameBuffer.frameBuffer;

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = width;
	renderPassInfo.renderArea.extent.height = height;

	std::vector<VkClearValue> clearValues(2);
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[0].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = 2;
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
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoPipeline.pipelineLayout, 0, 1, &ssaoPipeline.descriptorSet, 0, nullptr);

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoPipeline.pipeline);
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}

void SSAO::UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 cameraPos, float zNear, float zFar)
{
	UniformBufferObject ubo;
	ubo.view = view;
	ubo.proj = proj;
	ubo.clipPlane.x = zNear;
	ubo.clipPlane.y = zFar;

	void* data;
	vkMapMemory(device, uniformMemory, 0, sizeof(UniformBufferObject), 0, &data);
	memcpy(data, &ubo, sizeof(UniformBufferObject));
	vkUnmapMemory(device, uniformMemory);
}

void SSAO::Cleanup()
{
	noiseMap.destroy();
	ssaoPipeline.Cleanup(device);
	ssaoFrameBuffer.Cleanup(device);

	__super::Cleanup();
}
