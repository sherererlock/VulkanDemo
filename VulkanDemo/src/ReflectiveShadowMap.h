#pragma once

#include <glm/glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "GBufferRenderer.h"

class ReflectiveShadowMap : public GBufferRenderer
{
	struct UniformBufferObject
	{
		glm::vec4 clipPlane;
		glm::vec4 lightPos;
		glm::vec4 lightColor;
		glm::mat4 depthVP;
	};

	const static int sampleCount = 64;
	float radius = 500.0f;
	struct RandomUniformBufferObject
	{
		glm::vec4 xi[sampleCount];
	} rubo;

private:
	VkDescriptorSetLayout descriptorSetLayoutMa;

	VkBuffer runiformBuffer;
	VkDeviceMemory runiformMemory;

	FrameBufferAttachment position;
	FrameBufferAttachment normal;
	FrameBufferAttachment flux;
	FrameBufferAttachment depth;

	float depthBiasConstant = 1.25f;
	// Slope depth bias factor, applied depending on polygon's slope
	float depthBiasSlope = 1.75f;

public:

	virtual void InitRandomBuffer() override;
	VkDescriptorBufferInfo GetBufferInfo() const override;

	void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo) override;
	void CreatePass() override;
	void CreateDescriptSetLayout() override;
	void SetupDescriptSet(VkDescriptorPool pool) override;

	void CreateGBuffer() override;
	void CreateUniformBuffer() override;
	void CreateFrameBuffer() override;
	void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel) override;
	void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos, float zNear, float zFar) override;

	void Cleanup() override;

public:
	inline const VkDescriptorImageInfo& GetDepthDescriptorImageInfo() const { return depth.descriptor; }
	inline const VkDescriptorImageInfo& GetPositionDescriptorImageInfo() const { return position.descriptor; }
	inline const VkDescriptorImageInfo& GetNormalDescriptorImageInfo() const { return normal.descriptor; }
	inline const VkDescriptorImageInfo& GetFluxDescriptorImageInfo() const { return flux.descriptor; }

};


