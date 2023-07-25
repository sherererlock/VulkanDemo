#pragma once

#include <glm/glm/glm.hpp>
#include <vulkan/vulkan.h>

class HelloVulkan;
class gltfModel;
struct PipelineCreateInfo;

class GBufferRenderer
{
protected:
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

protected:
	VkFramebuffer framebuffer;
	VkPipeline pipeline;
	VkRenderPass renderPass;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkPipelineLayout pipelineLayout;

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformMemory;

	VkDevice device;
	uint32_t width, height;
	HelloVulkan* vulkanAPP;

public:
	void Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h);
	virtual void InitRandomBuffer();
	virtual VkDescriptorBufferInfo GetBufferInfo() const;

	virtual void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo);
	virtual void CreatePass();
	virtual void CreateDescriptSetLayout();
	virtual void SetupDescriptSet(VkDescriptorPool pool);

	virtual void CreateGBuffer();
	
	virtual void CreateUniformBuffer();
	virtual void CreateFrameBuffer();
	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel);
	virtual void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos, float zNear, float zFar);

	virtual void Cleanup();

	void CreateAttachment(FrameBufferAttachment* attachment, VkFormat format, VkImageUsageFlagBits usage);
};


