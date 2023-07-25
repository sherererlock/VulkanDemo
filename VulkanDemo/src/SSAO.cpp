#include <algorithm>
#include <iostream>
#include <random>

#include "HelloVulkan.h"
#include "SSAO.h"

void SSAO::InitRandomBuffer()
{
}

VkDescriptorBufferInfo SSAO::GetBufferInfo() const
{
	return VkDescriptorBufferInfo();
}

void SSAO::CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo)
{
	vertexShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/ssaogbuffer.vert.spv";
	fragmentShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/ssaogbuffer.frag.spv";

	__super::CreatePipeline(info, creatInfo);
}

void SSAO::CreateDescriptSetLayout()
{
	__super::CreateDescriptSetLayout();
}

void SSAO::SetupDescriptSet(VkDescriptorPool pool)
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
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

void SSAO::CreateGBuffer()
{
	__super::CreateGBuffer();
}

void SSAO::CreateUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformMemory);
}

void SSAO::CreateFrameBuffer()
{
	__super::CreateFrameBuffer();
}

void SSAO::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
	__super::BuildCommandBuffer(commandBuffer, gltfmodel);
}

void SSAO::UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 cameraPos, float zNear, float zFar)
{
	UniformBufferObject ubo;
	ubo.view = view;
	ubo.viewProj = proj * view;
	ubo.clipPlane.x = zNear;
	ubo.clipPlane.y = zFar;

	void* data;
	vkMapMemory(device, uniformMemory, 0, sizeof(UniformBufferObject), 0, &data);
	memcpy(data, &ubo, sizeof(UniformBufferObject));
	vkUnmapMemory(device, uniformMemory);
}

void SSAO::Cleanup()
{
	__super::Cleanup();
}
