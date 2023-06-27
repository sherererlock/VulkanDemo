#pragma once

#include <glm/glm/glm.hpp>
#include <vulkan/vulkan.h>

struct ShadowUniformBufferObject {
	glm::mat4 depthVP;
};

class HelloVulkan;
class gltfModel;
struct PipelineCreateInfo;

#define DEPTH_FORMAT VK_FORMAT_D16_UNORM
//#define DEPTH_FORMAT VK_FORMAT_D32_SFLOAT

//const VkFormat format = VK_FORMAT_D32_SFLOAT;


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
	VkDescriptorImageInfo descriptor;

	VkDevice device;
	uint32_t width, height;
	HelloVulkan* vulkanAPP;

	// Depth bias (and slope) are used to avoid shadowing artifacts
	// Constant depth bias factor (always applied)
	float depthBiasConstant = 1.25f;
	// Slope depth bias factor, applied depending on polygon's slope
	float depthBiasSlope = 1.75f;

public:

	inline VkDescriptorImageInfo GetDescriptorImageInfo() const {
		return descriptor;
	}

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

