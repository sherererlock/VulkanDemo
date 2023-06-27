#pragma once

#include <glm/glm/glm.hpp>
#include <vulkan/vulkan.h>

struct DebugUniformBufferObject {
	float zNear;
	float zFar;
};


class HelloVulkan;
class gltfModel;
struct PipelineCreateInfo;


class Debug
{
private:
	VkPipeline debugPipeline;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkPipelineLayout pipelineLayout;

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformMemory;

	VkDevice device;
	HelloVulkan* vulkanAPP;

	float zNear;
	float zFar;

public:
	inline void Init(VkDevice vkdevice, HelloVulkan* app, float nearPlane, float farPlane) {
		device = vkdevice;
		vulkanAPP = app;
		zNear = nearPlane;
		zFar = farPlane;
	}

	void CreateDebugPipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo);
	void CreateDescriptSetLayout();
	void SetupDescriptSet(VkDescriptorPool pool, const VkDescriptorImageInfo& imageInfo);
	void CreateUniformBuffer();
	void BuildCommandBuffer(VkCommandBuffer commandBuffer);

	void Cleanup();
};