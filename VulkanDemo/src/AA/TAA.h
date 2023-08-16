#pragma once
#include "SinglePipeline.h"


class TAA : public SinglePipeline
{
private:
	struct UniformBufferObject
	{
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 lastView;
		glm::mat4 lastProj;
		float alpha = 0.1f;
	};

	FrameBufferAttachment historyBuffer;
	UniformBufferObject ubo;

	VkRenderPass renderPass;
	VkFramebuffer frameBuffer;

	FrameBufferAttachment color;

public:
	virtual void Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h);

	virtual void CreatePass();
	virtual void CreateFrameBuffer();
	virtual void CreateGBuffer();

	virtual void CreateDescriptSetLayout();
	virtual void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo);
	virtual void SetupDescriptSet(VkDescriptorPool pool);

	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel);
	void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::mat4 depthVP, glm::vec4 viewPos);

	virtual void Cleanup();

	void SaveHistoryBuffer(VkCommandBuffer commandBuffer, VkImage image);

	inline const VkDescriptorImageInfo& GetColorDescriptorImageInfo() const { return color.descriptor; }
};
