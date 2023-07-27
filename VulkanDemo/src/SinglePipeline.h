#pragma once
#include <string>

#include <glm/glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "macros.h"

class HelloVulkan;
class gltfModel;
struct PipelineCreateInfo;

class SinglePipeline
{
protected:
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformMemory;
	VkDescriptorBufferInfo bufferInfo;

	VkDevice device;
	uint32_t width, height;
	HelloVulkan* vulkanAPP;

	std::string vertexShader;
	std::string fragmentShader;

	VkDeviceSize bufferSize;
public:

	virtual void Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h);

	virtual void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo);
	virtual void CreateDescriptSetLayout();
	virtual void SetupDescriptSet(VkDescriptorPool pool);

	virtual	void CreateUniformBuffer();
	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel) = 0;
	virtual void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos) {};

	virtual void Cleanup();
};

