#pragma once
#include "GBufferRenderer.h"
class BasePass : public GBufferRenderer
{
private:
	FrameBufferAttachment roughnessMetallic;
	FrameBufferAttachment albedo;
	FrameBufferAttachment emissive;

	std::vector<gltfModel*> models;

public:
	inline void AddModel(gltfModel* model) { models.push_back(model); }

	virtual std::vector<VkAttachmentDescription> GetAttachmentDescriptions() const override;
	virtual std::vector<VkAttachmentReference> GetAttachmentRefs() const override;
	virtual std::vector<VkImageView> GetImageViews() const override;

	virtual void CreateDescriptSetLayout() override;
	virtual void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo) override;
	virtual void CreateGBuffer() override;

	virtual void SetupDescriptSet(VkDescriptorPool pool) override;
	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel);

	virtual void Cleanup() override;

public:
	inline const VkDescriptorImageInfo& GetRoughnessDescriptorImageInfo() const { return roughnessMetallic.descriptor; }
	inline const VkDescriptorImageInfo& GetAlbedoDescriptorImageInfo() const { return albedo.descriptor; }
	inline const VkDescriptorImageInfo& GetEmissiveDescriptorImageInfo() const { return emissive.descriptor; }
};

