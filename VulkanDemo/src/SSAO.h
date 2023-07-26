#pragma once
#include "GBufferRenderer.h"

const int SSAO_KERNEL_SIZE = 64;
const float	SSAO_RADIUS = 1.5;

class SSAO : public GBufferRenderer
{
private:
	struct UniformBufferObject
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 clipPlane;
	};

	struct SSAOUBO
	{
		glm::mat4 proj;
		glm::vec4 samples[SSAO_KERNEL_SIZE];
	};

	struct PipelineObject
	{
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet;
		VkBuffer uniformBuffer;
		VkDeviceMemory uniformMemory;

		void Cleanup(VkDevice device)
		{
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

			vkDestroyBuffer(device, uniformBuffer, nullptr);
			vkFreeMemory(device, uniformMemory, nullptr);

			vkDestroyPipeline(device, pipeline, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		}
	};

	struct FrameBuffer
	{
		VkRenderPass renderPass;
		VkFramebuffer frameBuffer;
		int width, height;

		void Cleanup(VkDevice device)
		{
			vkDestroyFramebuffer(device, frameBuffer, nullptr);
			vkDestroyRenderPass(device, renderPass, nullptr);
		}
	};

	struct SSAOFrameBuffer : public FrameBuffer
	{
		FrameBufferAttachment color;
		void Cleanup(VkDevice device)
		{
			color.Cleanup(device);
			__super::Cleanup(device);
		}
	};

private:

	float depthBiasConstant = 1.25f;
	// Slope depth bias factor, applied depending on polygon's slope
	float depthBiasSlope = 1.75f;

	PipelineObject ssaoPipeline;
	SSAOFrameBuffer ssaoFrameBuffer;
	SSAOUBO ubo;
	Texture2D noiseMap;

private:
	void BuildSSAOCommandBuffer(VkCommandBuffer commandBuffer);

public:
	virtual void InitRandomBuffer() override;
	VkDescriptorBufferInfo GetBufferInfo() const override;
	void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo) override;
	virtual void CreatePass()override;
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

