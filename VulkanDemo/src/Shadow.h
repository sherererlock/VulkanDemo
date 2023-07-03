#pragma once

#include <glm/glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "macros.h"

class HelloVulkan;
class gltfModel;
struct PipelineCreateInfo;

#define DEPTH_FORMAT VK_FORMAT_D16_UNORM
//#define DEPTH_FORMAT VK_FORMAT_D32_SFLOAT

class Shadow
{
protected:
	VkPipeline shadowPipeline;
	VkRenderPass shadowPass;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkPipelineLayout pipelineLayout;

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformMemory;

	VkImage shadowMapImage;
	VkDeviceMemory shadowMapMemory;
	VkImageView shadowMapImageView;
	VkSampler shadowMapSampler;
	VkDescriptorImageInfo descriptor;

	VkDevice device;
	uint32_t width, height;
	HelloVulkan* vulkanAPP;

	// Depth bias (and slope) are used to avoid shadowing artifacts
	// Constant depth bias factor (always applied)
	float depthBiasConstant = 1.25f;
	// Slope depth bias factor, applied depending on polygon's slope
	float depthBiasSlope = 1.75f;

	VkImageViewType viewType;
	int layerCount;
	VkDeviceSize size;

	struct primitiveInfo {
		glm::mat4 model;
		int cascadedIndx;
	};


public:

	inline VkDescriptorImageInfo GetDescriptorImageInfo() const {
		return descriptor;
	}
	
	void Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h);

	void CreateShadowPipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo);
	void CreateShadowPass();
	void CreateDescriptSetLayout();
	void SetupDescriptSet(VkDescriptorPool pool);

	virtual void CreateShadowMap();
	virtual	void CreateUniformBuffer() = 0;
	virtual void CreateFrameBuffer() = 0;
	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel) = 0;
	virtual void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos) = 0;

	virtual void Cleanup();
};

