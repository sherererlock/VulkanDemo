
#include <array>
#include <stdexcept>
#include <iostream>

#include "HelloVulkan.h"
#include "BasePass.h"

std::vector<VkAttachmentDescription> BasePass::GetAttachmentDescriptions() const
{
	std::vector<VkAttachmentDescription> attachments = __super::GetAttachmentDescriptions();

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 采样数
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // 存储下来
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	colorAttachment.format = albedo.format;

	attachments.push_back(colorAttachment);
	attachments.push_back(colorAttachment);
	attachments.push_back(colorAttachment);
	colorAttachment.format = velocity.format;
	attachments.push_back(colorAttachment);

	return attachments;
}

std::vector<VkAttachmentReference> BasePass::GetAttachmentRefs() const
{
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::vector<VkAttachmentReference> colorAttachmentRefs = { colorAttachmentRef, colorAttachmentRef, colorAttachmentRef, colorAttachmentRef, colorAttachmentRef, colorAttachmentRef };
	colorAttachmentRefs[0].attachment = 0;
	colorAttachmentRefs[1].attachment = 1;
	colorAttachmentRefs[2].attachment = 3;
	colorAttachmentRefs[3].attachment = 4;
	colorAttachmentRefs[4].attachment = 5;
	colorAttachmentRefs[5].attachment = 6;

	return colorAttachmentRefs;
}

std::vector<VkImageView> BasePass::GetImageViews() const
{
	std::vector<VkImageView> imageviews = {
		position.view,
		normal.view,
		depth.view,
		roughnessMetallic.view,
		albedo.view,
		emissive.view,
		velocity.view
	};

	return imageviews;
}

void BasePass::CreateDescriptSetLayout()
{
	// for material
	{
		VkDescriptorSetLayoutBinding binding;

		binding.binding = 0;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.descriptorCount = 1;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		binding.pImmutableSamplers = nullptr; // Optional

		std::array<VkDescriptorSetLayoutBinding, 5> bindings;
		bindings.fill(binding);

		for (int i = 1; i < 5; i++)
			bindings[i].binding = i;

		bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	// for TAA jitter and velocitybuffer
	{

		VkDescriptorSetLayoutBinding binding;
		binding.binding = 0;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding.descriptorCount = 1;
		binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		binding.pImmutableSamplers = nullptr; // Optional

		std::array<VkDescriptorSetLayoutBinding, 2> bindings;
		bindings.fill(binding);
		bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[1].binding = 1;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &velocitySetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

}

void BasePass::CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo)
{
	vertexShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/BasePass.vert.spv";
	fragmentShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/BasePass.frag.spv";

	VkPushConstantRange pushConstantRange;
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(glm::mat4);

	std::vector<VkDescriptorSetLayout> setLayouts = vulkanAPP->GetDescriptorSetLayouts();
	setLayouts.push_back(descriptorSetLayout);
	setLayouts.push_back(velocitySetLayout);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = (uint32_t)setLayouts.size(); // Optional
	pipelineLayoutInfo.pSetLayouts = setLayouts.data(); // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	auto attributeDescriptoins = Vertex::getAttributeDescriptions({ Vertex::VertexComponent::Position, Vertex::VertexComponent::Normal, Vertex::VertexComponent::UV, Vertex::VertexComponent::Tangent });
	auto attributeDescriptionBindings = Vertex::getBindingDescription();

	info.vertexInputInfo.pVertexBindingDescriptions = &attributeDescriptionBindings; // Optional
	info.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptoins.size());
	info.vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptoins.data(); // Optional

	auto shaderStages = vulkanAPP->CreaterShader(vertexShader, fragmentShader);

	creatInfo.layout = pipelineLayout;
	creatInfo.stageCount = (uint32_t)shaderStages.size();
	creatInfo.pStages = shaderStages.data();
	creatInfo.subpass = 0;
	creatInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	creatInfo.basePipelineIndex = -1; // Optional

	info.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	info.rasterizer.depthBiasClamp = VK_FALSE;
	info.rasterizer.depthBiasEnable = VK_FALSE;
	info.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
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

	auto colorAttachmentRefs = GetAttachmentRefs();
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
	for (auto colorAttachmentRef : colorAttachmentRefs)
		colorBlendAttachments.push_back(colorBlendAttachment);

	info.colorBlending.pAttachments = colorBlendAttachments.data();
	info.colorBlending.attachmentCount = (uint32_t)colorBlendAttachments.size();

	info.Apply(creatInfo);

	creatInfo.renderPass = renderPass;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &creatInfo, nullptr, &pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}

void BasePass::CreateGBuffer()
{
	__super::CreateGBuffer();
	CreateAttachment(&roughnessMetallic, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	CreateAttachment(&albedo, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	CreateAttachment(&emissive, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	CreateAttachment(&velocity, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
}

void BasePass::CreateUniformBuffer()
{
	uint32_t bufferSize = (uint32_t)(vulkanAPP->GetHaltonSequence().size() * sizeof(glm::vec4));
	vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, haltonSequenceBuffer, haltonSequenceMemory);

	bufferSize = sizeof(JitterInfo);
	vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, jitterBuffer, jitterMemory);
}

void BasePass::SetupDescriptSet(VkDescriptorPool pool)
{
	std::array<VkWriteDescriptorSet, 5> descriptorWrites = {};

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	for (gltfModel* gltfmodel : models)
	{
		for (int i = 0; i < gltfmodel->materials.size(); i++)
		{
			Material& material = gltfmodel->materials[i];
			if (vkAllocateDescriptorSets(device, &allocInfo, &material.descriptorSet) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to allocate descriptor set!");
			}

			VkDeviceSize bufferSize = sizeof(Material::MaterialData);
			vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, material.materialBuffer, material.materialBufferMemory);
			material.UpdateMaterialBuffer(device);

			uint32_t indices[4] = { material.baseColorTextureIndex, material.normalTextureIndex,  material.roughnessTextureIndex, material.emissiveTextureIndex };
			for (int j = 0; j < 4; j++)
			{
				descriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[j].dstSet = material.descriptorSet;
				descriptorWrites[j].dstBinding = j;
				descriptorWrites[j].dstArrayElement = 0;
				descriptorWrites[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[j].descriptorCount = 1;
				descriptorWrites[j].pBufferInfo = nullptr;
				if (indices[j] != -1)
					descriptorWrites[j].pImageInfo = &gltfmodel->images[indices[j]].texture.descriptor; // Optional
				else
					descriptorWrites[j].pImageInfo = &vulkanAPP->GetEmptyTexture().descriptor; // Optional

				descriptorWrites[j].pTexelBufferView = nullptr; // Optional
			}

			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = material.materialBuffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(Material::MaterialData);

			descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[4].dstSet = material.descriptorSet;
			descriptorWrites[4].dstBinding = 4;
			descriptorWrites[4].dstArrayElement = 0;
			descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[4].descriptorCount = 1;
			descriptorWrites[4].pBufferInfo = &bufferInfo;
			descriptorWrites[4].pImageInfo = nullptr; // Optional
			descriptorWrites[4].pTexelBufferView = nullptr; // Optional

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	{
		allocInfo.pSetLayouts = &velocitySetLayout;
		if (vkAllocateDescriptorSets(device, &allocInfo, &velocitySet) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor set!");
		}

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		uint32_t bufferSize = (uint32_t)(vulkanAPP->GetHaltonSequence().size() * sizeof(glm::vec4));

		VkDescriptorBufferInfo hbufferInfo = {};
		hbufferInfo.buffer = haltonSequenceBuffer;
		hbufferInfo.offset = 0;
		hbufferInfo.range = bufferSize;

		VkDescriptorBufferInfo jbufferInfo = {};
		jbufferInfo.buffer = jitterBuffer;
		jbufferInfo.offset = 0;
		jbufferInfo.range = sizeof(jitterInfo);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = velocitySet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &hbufferInfo;
		descriptorWrites[0].pImageInfo = nullptr; // Optional
		descriptorWrites[0].pTexelBufferView = nullptr; // Optional

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = velocitySet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &jbufferInfo;
		descriptorWrites[1].pImageInfo = nullptr; // Optional
		descriptorWrites[1].pTexelBufferView = nullptr; // Optional

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	{
		jitterInfo.preViewProj = vulkanAPP->GetCamera().matrices.perspective * vulkanAPP->GetCamera().matrices.view;
		jitterInfo.hindex = 0;
		jitterInfo.width = (float)width;
		jitterInfo.height = (float)height;

		const std::vector<glm::vec4>& haltonSequence = vulkanAPP->GetHaltonSequence();

		uint32_t bufferSize = (uint32_t)(haltonSequence.size() * sizeof(glm::vec4));

		void* data;
		vkMapMemory(device, haltonSequenceMemory, 0, bufferSize, 0, &data);
		memcpy(data, haltonSequence.data(), bufferSize);
		vkUnmapMemory(device, haltonSequenceMemory);
	}
}

void BasePass::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = width;
	renderPassInfo.renderArea.extent.height = height;

	std::vector<VkClearValue> clearValues(7);
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[2].depthStencil = { 1.0f, 0 };
	clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[4].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[5].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[6].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

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

	renderPassInfo.framebuffer = framebuffer;
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	auto descriptorSets = vulkanAPP->GetDescriptorSets();
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[0], 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &descriptorSets[1], 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 3, 1, &velocitySet, 0, nullptr);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	gltfmodel.draw(commandBuffer, pipelineLayout, 0, 2);

	vkCmdEndRenderPass(commandBuffer);
}

void BasePass::UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos)
{
	const std::vector<glm::vec4>& haltonSequence = vulkanAPP->GetHaltonSequence();
	jitterInfo.hindex++;
	jitterInfo.hindex %= (uint32_t)haltonSequence.size();

	void* data;
	vkMapMemory(device, jitterMemory, 0, sizeof(jitterInfo), 0, &data);
	memcpy(data, &jitterInfo, sizeof(jitterInfo));
	vkUnmapMemory(device, jitterMemory);

	jitterInfo.preViewProj = proj * view;
}

void BasePass::Cleanup()
{
	roughnessMetallic.Cleanup(device);
	albedo.Cleanup(device);
	emissive.Cleanup(device);
	velocity.Cleanup(device);

	vkDestroyBuffer(device, haltonSequenceBuffer, nullptr);
	vkFreeMemory(device, haltonSequenceMemory, nullptr);

	vkDestroyBuffer(device, jitterBuffer, nullptr);
	vkFreeMemory(device, jitterMemory, nullptr);

	vkDestroyDescriptorSetLayout(device, velocitySetLayout, nullptr);

	__super::Cleanup();
}