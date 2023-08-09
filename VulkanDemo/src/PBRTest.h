#pragma once
#include "SinglePipeline.h"
class PBRTest : public SinglePipeline
{
private:
	struct Material {
		// Parameter block used as push constant block
		struct PushBlock {
			float roughness;
			float metallic;
			float r, g, b;
		} params;
		std::string name;
		Material() {};
		Material(std::string n, glm::vec3 c, float r, float m) : name(n) {
			params.roughness = r;
			params.metallic = m;
			params.r = c.r;
			params.g = c.g;
			params.b = c.b;
		};
	};

	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 viewPos;
		glm::vec4 lightPos[4];
	};

	std::vector<Material> materials;
	int32_t materialIndex = 0;

	std::vector<gltfModel*> models;

public:
	inline void AddModel(gltfModel* model) { models.push_back(model); }

	virtual void Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h);
	virtual void CreateDescriptSetLayout();
	virtual void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo);
	virtual void SetupDescriptSet(VkDescriptorPool pool);
	virtual	void CreateUniformBuffer();
	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel);
	virtual void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos);
	virtual void Cleanup();
};

