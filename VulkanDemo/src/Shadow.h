#pragma once

#include <glm/glm/glm.hpp>
#include <vulkan/vulkan.h>


struct ShadowUniformBufferObject {
	glm::mat4 depthVP;
};

class HelloVulkan;
class gltfModel;
struct PipelineCreateInfo;

class Shadow
{
private:
	VkPipeline shadowPipeline;
	VkRenderPass shadowPass;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkPipelineLayout pipelineLayout;

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

	void Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h);
	void CreateShadowPipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo);
	void CreateShadowPass();
	void CreateDescriptSetLayout();
	void SetupDescriptSet(VkDescriptorPool pool);
	void CreateFrameBuffer();
	void CreateUniformBuffer();
	void CreateShadowMap();

	void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel);

	void UpateLightMVP(glm::mat4 translation);

	void Cleanup();
};

