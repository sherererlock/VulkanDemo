#pragma once

#include <glm/glm/glm.hpp>
#include <vulkan/vulkan.h>


struct ShadowUniformBufferObject {
	glm::mat4 LightMVP;
};

class HelloVulkan;
struct PipelineCreateInfo;

class Shadow
{
private:
	VkPipeline shadowPipeline;
	VkRenderPass shadowPass;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;

	VkFramebuffer frameBuffer;

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformMemory;

	VkImage shadowMapImage;
	VkImageView shadowMapImageView;
	VkDeviceMemory shadowMapMemory;
	VkSampler shadowMapSampler;

	VkDevice device;
	uint32_t width, height;
	HelloVulkan* vulkanAPP;

public:
	Shadow(VkDevice vkdevice, uint32_t w, uint32_t h) : device(vkdevice), width(w), height(h) {}

	void CreateShadowPipeline(PipelineCreateInfo&  pipelineCreateInfo);
	void CreateShadowPass();
	void CreateDescriptSetLayout();
	void SetupDescriptSet(VkDescriptorPool pool);
	void CreateFrameBuffer();
	void CreateUniformBuffer();
	void CreateShadowMap();
	void CreateShader();

	void UpateLightMVP(glm::mat4 translation);
};

