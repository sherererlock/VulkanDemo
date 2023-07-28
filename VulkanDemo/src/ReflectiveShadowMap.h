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

	VkBuffer runiformBuffer;
	VkDeviceMemory runiformMemory;

	FrameBufferAttachment flux;

public:

	virtual std::vector<VkAttachmentDescription> GetAttachmentDescriptions() const override;
	virtual std::vector<VkAttachmentReference> GetAttachmentRefs() const override;
	virtual std::vector<VkImageView> GetImageViews() const override;

	void InitRandomBuffer();
	VkDescriptorBufferInfo GetBufferInfo() const override;

	void SetupDescriptSet(VkDescriptorPool pool) override;

	virtual void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo) override;
	void CreateGBuffer() override;
	void CreateUniformBuffer() override;
	void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos, float zNear, float zFar) override;

	void Cleanup() override;

public:

	inline const VkDescriptorImageInfo& GetFluxDescriptorImageInfo() const { return flux.descriptor; }

};


