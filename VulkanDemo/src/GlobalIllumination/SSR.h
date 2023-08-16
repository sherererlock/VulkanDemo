#pragma once
#include "SinglePipeline.h"

class SSRGBufferRenderer;
class GenHierarchicalDepth;
class SSR : public SinglePipeline
{
private:
	struct UniformBufferObject
	{
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 depthVP;
		glm::vec4 viewPos;
	};

	SSRGBufferRenderer* gbuffer;
	GenHierarchicalDepth* hierarchicaldepth;

public:
	virtual void Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h);

	virtual void CreateDescriptSetLayout();
	virtual void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo);
	virtual void SetupDescriptSet(VkDescriptorPool pool);

	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel);
	void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::mat4 depthVP, glm::vec4 viewPos);

};

