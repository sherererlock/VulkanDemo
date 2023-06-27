#include "HelloVulkan.h"
#include "Debug.h"
#include <stdexcept>

void Debug::CreateDebugPipeline(PipelineCreateInfo& pipelineCreateInfo, VkGraphicsPipelineCreateInfo& creatInfo)
{
	// empty vertex input for quad
	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
	pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	creatInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;

    // Disable culling, so all faces contribute to shadows
    pipelineCreateInfo.rasterizer.cullMode = VK_CULL_MODE_NONE;

	auto shaderStages = vulkanAPP->CreaterShader("D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/quad.vert.spv", "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/quad.frag.spv");
    creatInfo.stageCount = shaderStages.size();
    creatInfo.pStages = shaderStages.data();

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

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &creatInfo, nullptr, &debugPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}

void Debug::CreateDescriptSetLayout()
{
    std::array<VkDescriptorSetLayoutBinding, 2> uniformLayoutBindings = {};
	uniformLayoutBindings[0].binding = 0;
	uniformLayoutBindings[0].descriptorCount = 1;
	uniformLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformLayoutBindings[0].pImmutableSamplers = nullptr;
	uniformLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	uniformLayoutBindings[1].binding = 1;
	uniformLayoutBindings[1].descriptorCount = 1;
	uniformLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	uniformLayoutBindings[1].pImmutableSamplers = nullptr;
	uniformLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = uniformLayoutBindings.size();
	layoutInfo.pBindings = uniformLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shadow descriptor set layout!");
	}
}

void Debug::SetupDescriptSet(VkDescriptorPool pool, const VkDescriptorImageInfo& imageInfo)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate shadow descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(DebugUniformBufferObject);

	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;
	descriptorWrites[0].pImageInfo = nullptr; // Optional
	descriptorWrites[0].pTexelBufferView = nullptr; // Optional

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = nullptr;
	descriptorWrites[1].pImageInfo = &imageInfo; // Optional
	descriptorWrites[1].pTexelBufferView = nullptr; // Optional

	vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void Debug::CreateUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(DebugUniformBufferObject);
	vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformMemory);

	DebugUniformBufferObject ubo;
	ubo.zNear = zNear;
	ubo.zFar = zFar;

	void* data;
	vkMapMemory(device, uniformMemory, 0, sizeof(DebugUniformBufferObject), 0, &data);
	memcpy(data, &ubo, sizeof(DebugUniformBufferObject));
	vkUnmapMemory(device, uniformMemory);
}

void Debug::BuildCommandBuffer(VkCommandBuffer commandBuffer)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debugPipeline);
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void Debug::Cleanup()
{
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkFreeMemory(device, uniformMemory, nullptr);

	vkDestroyPipeline(device, debugPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}
