#pragma once
#include "SinglePipeline.h"

class BasePass;

class LightingPass : public SinglePipeline
{
private:
	struct UniformBufferObject
	{
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 depthVP;
		glm::vec4 viewPos;
		glm::vec4 lightPos[4];
		glm::vec4 shadowParams = glm::vec4(1.0);
	};

	BasePass* basePass;

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

	inline const FrameBufferAttachment* GetAttachment() const { return &color; }
};

