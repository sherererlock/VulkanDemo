#pragma once
#include "GBufferRenderer.h"

class SSAO : public GBufferRenderer
{
private:
	struct UniformBufferObject
	{
		glm::mat4 view;
		glm::mat4 viewProj;
		glm::vec4 clipPlane;
	};

private:

	float depthBiasConstant = 1.25f;
	// Slope depth bias factor, applied depending on polygon's slope
	float depthBiasSlope = 1.75f;

public:
	virtual void InitRandomBuffer() override;
	VkDescriptorBufferInfo GetBufferInfo() const override;
	void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo) override;
	void CreateDescriptSetLayout() override;
	void SetupDescriptSet(VkDescriptorPool pool) override;
	void CreateGBuffer() override;
	void CreateUniformBuffer() override;
	void CreateFrameBuffer() override;
	void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel) override;
	void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 cameraPos, float zNear, float zFar) override;
	void Cleanup() override;

public:
	inline const VkDescriptorImageInfo& GetDepthDescriptorImageInfo() const { return depth.descriptor; }
	inline const VkDescriptorImageInfo& GetPositionDescriptorImageInfo() const { return position.descriptor; }
	inline const VkDescriptorImageInfo& GetNormalDescriptorImageInfo() const { return normal.descriptor; }
};

