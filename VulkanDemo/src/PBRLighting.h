#pragma once
#include<vector>

#include "SinglePipeline.h"

class PBRLighting : public SinglePipeline
{
private:
	std::vector<gltfModel*> models;

public:
	inline VkDescriptorSetLayout GetDescriptorSetLayout() { return descriptorSetLayout; }
	inline void AddModel(gltfModel* model) { models.push_back(model); }
	void SetShaderFile(std::string vertex, std::string fragment)
	{
		vertexShader = vertex;
		fragmentShader = fragment;
	}

	virtual void CreateDescriptSetLayout() override;
	virtual void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo) override;
	virtual void SetupDescriptSet(VkDescriptorPool pool) override;

	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel) override;
};

