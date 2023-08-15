
#include <algorithm>
#include <iostream>
#include <random>

#include "HelloVulkan.h"
#include "ReflectiveShadowMap.h"

std::vector<VkAttachmentDescription> ReflectiveShadowMap::GetAttachmentDescriptions() const
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
	colorAttachment.format = flux.format;

	attachments.push_back(colorAttachment);

	return attachments;
}

std::vector<VkAttachmentReference> ReflectiveShadowMap::GetAttachmentRefs() const
{
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::vector<VkAttachmentReference> colorAttachmentRefs = { colorAttachmentRef, colorAttachmentRef, colorAttachmentRef };
	colorAttachmentRefs[0].attachment = 0;
	colorAttachmentRefs[1].attachment = 1;
	colorAttachmentRefs[2].attachment = 3;

	return colorAttachmentRefs;
}

std::vector<VkImageView> ReflectiveShadowMap::GetImageViews() const
{
	std::vector<VkImageView> imageviews = {
		position.view,
		normal.view,
		depth.view,
		flux.view
	};

	return imageviews;
}

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

void ReflectiveShadowMap::CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo)
{
	vertexShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/rsmgbuffer.vert.spv";
	fragmentShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/rsmgbuffer.frag.spv";

	__super::CreatePipeline(info, creatInfo);
}

void ReflectiveShadowMap::CreateGBuffer()
{
	__super::CreateGBuffer();
	CreateAttachment(&flux, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
}

void ReflectiveShadowMap::CreateUniformBuffer()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformMemory);

	bufferSize = sizeof(RandomUniformBufferObject);
	vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, runiformBuffer, runiformMemory);
}

void ReflectiveShadowMap::UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos)
{
	UniformBufferObject ubo;
	ubo.clipPlane.x = vulkanAPP->GetCamera().getNearClip();
	ubo.clipPlane.y = vulkanAPP->GetCamera().getFarClip();
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
	flux.Cleanup(device);

	vkDestroyBuffer(device, runiformBuffer, nullptr);
	vkFreeMemory(device, runiformMemory, nullptr);

	__super::Cleanup();
}
