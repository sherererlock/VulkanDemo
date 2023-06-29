#pragma once

#include <glm/glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "macros.h"

struct ShadowUniformBufferObject {
	glm::mat4 depthVP[CASCADED_COUNT];
	uint32_t cascadedIndex;
};

class HelloVulkan;
class gltfModel;
struct PipelineCreateInfo;

#define DEPTH_FORMAT VK_FORMAT_D16_UNORM
//#define DEPTH_FORMAT VK_FORMAT_D32_SFLOAT

//const VkFormat format = VK_FORMAT_D32_SFLOAT;

struct CasadedInfo
{
	VkFramebuffer frameBuffer;
	VkImageView shadowMapImageView;

	float splitDepth;
	glm::mat4 depthVP;

	void Cleanup(VkDevice device);
};

class Shadow
{
private:

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

	std::array<CasadedInfo, CASCADED_COUNT> casadedInfos;

public:

	inline VkDescriptorImageInfo GetDescriptorImageInfo() const {
		return descriptor;
	}

	void GetCascadedInfo(glm::mat4* mat, float* splitDepth);

	void Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h);

	void CreateShadowPipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo);

	void CreateShadowPass();
	void CreateDescriptSetLayout();
	void SetupDescriptSet(VkDescriptorPool pool);
	void CreateFrameBuffer();
	void CreateUniformBuffer();
	void CreateShadowMap();
	void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel);
	void UpdateCascaded(glm::mat4 depthvp);
	void UpdateCascaded(glm::mat4 view, glm::mat4 proj, glm::vec4 lightpos);
	void UpateLightMVP(int i = 0);

	void Cleanup();
};

