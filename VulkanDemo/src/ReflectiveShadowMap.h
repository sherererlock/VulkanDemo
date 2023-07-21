#pragma once

#include <glm/glm/glm.hpp>
#include <vulkan/vulkan.h>

class HelloVulkan;
class gltfModel;
struct PipelineCreateInfo;

class ReflectiveShadowMap
{
private:
	struct FrameBufferAttachment
	{
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
		VkFormat format;
		VkSampler sampler;
		VkDescriptorImageInfo descriptor;

		void Cleanup(VkDevice device)
		{
			vkDestroyImage(device, image, nullptr);
			vkDestroyImageView(device, view, nullptr);
			vkFreeMemory(device, mem, nullptr);
			vkDestroySampler(device, sampler, nullptr);
		}

		void UpdateDescriptor()
		{
			descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			descriptor.imageView = view;
			descriptor.sampler = sampler;
		}
	};

	struct UniformBufferObject
	{
		glm::vec4 clipPlane;
		glm::vec4 lightPos;
		glm::vec4 lightColor;
		glm::mat4 depthVP;
	};


private:
	VkFramebuffer framebuffer;
	VkPipeline pipeline;
	VkRenderPass renderPass;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout descriptorSetLayoutMa;

	VkDescriptorSet descriptorSet;
	VkPipelineLayout pipelineLayout;

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformMemory;

	VkDevice device;
	uint32_t width, height;
	HelloVulkan* vulkanAPP;

	FrameBufferAttachment position;
	FrameBufferAttachment normal;
	FrameBufferAttachment flux;
	FrameBufferAttachment depth;

	float depthBiasConstant = 1.25f;
	// Slope depth bias factor, applied depending on polygon's slope
	float depthBiasSlope = 1.75f;
public:

	void Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h);

	void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo);
	void CreatePass();
	void CreateDescriptSetLayout();
	void SetupDescriptSet(VkDescriptorPool pool);

	void CreateGBuffer();
	void CreateAttachment(FrameBufferAttachment* attachment, VkFormat format, VkImageUsageFlagBits usage);
	void CreateUniformBuffer();
	void CreateFrameBuffer();
	void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel);
	void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos, float zNear, float zFar);

	void Cleanup();

public:

	inline VkDescriptorImageInfo GetDepthDescriptorImageInfo() const { return depth.descriptor; }
};


