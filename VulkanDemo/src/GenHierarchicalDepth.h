#pragma once
#include<vector>
#include "SinglePipeline.h"
class GenHierarchicalDepth : public SinglePipeline
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

		uint32_t width, height;

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

	struct MipData
	{
		glm::ivec2 uLastMipSize;
		int currentMipLv;
	};

	std::vector<FrameBufferAttachment> attachments;
	std::vector<VkFramebuffer> framebuffers;
	std::vector<VkDescriptorSet> descriptorSets;
	VkRenderPass renderPass;

public:

	void CreateRenderpass();
	void CreateFrameBuffer();
	void UpdateBuffer(uint32_t lv, uint32_t width, uint32_t height);
	void CreateMipMap();
	void CreateAttachment(FrameBufferAttachment* attachment, VkFormat format, VkImageUsageFlagBits usage, uint32_t width, uint32_t height);

	virtual void Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h);

	virtual void CreateDescriptSetLayout();
	virtual void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo);
	virtual void SetupDescriptSet(VkDescriptorPool pool);
	
	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel);

	virtual void Cleanup();
};

