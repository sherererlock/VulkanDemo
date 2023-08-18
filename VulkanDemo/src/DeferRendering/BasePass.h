#pragma once
#include "GBufferRenderer.h"
class BasePass : public GBufferRenderer
{
private:

	struct JitterInfo{
		glm::mat4 preViewProj;
		glm::mat4 model;
		glm::uint hindex;
		float width;
		float height;
	};

	FrameBufferAttachment roughnessMetallic;
	FrameBufferAttachment albedo;
	FrameBufferAttachment emissive;
	FrameBufferAttachment velocity;
	std::vector<gltfModel*> models;

	VkDescriptorSetLayout velocitySetLayout;
	VkDescriptorSet velocitySet;

	VkBuffer haltonSequenceBuffer;
	VkDeviceMemory haltonSequenceMemory;

	VkBuffer jitterBuffer;
	VkDeviceMemory jitterMemory;

	JitterInfo jitterInfo;

public:
	inline uint32_t GetIndex() const { return jitterInfo.hindex; }
	inline void AddModel(gltfModel* model) { models.push_back(model); }

	virtual std::vector<VkAttachmentDescription> GetAttachmentDescriptions() const override;
	virtual std::vector<VkAttachmentReference> GetAttachmentRefs() const override;
	virtual std::vector<VkImageView> GetImageViews() const override;

	virtual void CreateDescriptSetLayout() override;
	virtual void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo) override;
	virtual void CreateGBuffer() override;

	virtual void CreateUniformBuffer() override;

	virtual void SetupDescriptSet(VkDescriptorPool pool) override;
	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel);
	virtual void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos);

	virtual void Cleanup() override;

public:
	inline const VkImage& GetDepthImage() const { return depth.image; }
	inline const VkDescriptorImageInfo& GetRoughnessDescriptorImageInfo() const { return roughnessMetallic.descriptor; }
	inline const VkDescriptorImageInfo& GetAlbedoDescriptorImageInfo() const { return albedo.descriptor; }
	inline const VkDescriptorImageInfo& GetEmissiveDescriptorImageInfo() const { return emissive.descriptor; }
	inline const VkDescriptorImageInfo& GetVelocityDescriptorImageInfo() const { return velocity.descriptor; }
};

