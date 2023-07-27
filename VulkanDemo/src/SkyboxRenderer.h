#pragma once
#include "SinglePipeline.h"

class SkyboxRenderer : public SinglePipeline
{
private:
	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 projection;
	};

	gltfModel skyboxModel;
	TextureCubeMap cubeMap;

public:
	inline const TextureCubeMap& GetCubemap() const { return cubeMap; }
	inline const gltfModel& GetSkybox() const { return skyboxModel; }
	void LoadSkyBox();

	virtual void Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h) override;
	virtual void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo) override;
	virtual void CreateDescriptSetLayout() override;
	virtual void SetupDescriptSet(VkDescriptorPool pool) override;
	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel) override;
	virtual void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos) override;

	virtual void Cleanup() override;
};

