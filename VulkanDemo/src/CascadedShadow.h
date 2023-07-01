#pragma once

#include <glm/glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "Shadow.h"

class HelloVulkan;
class gltfModel;
struct PipelineCreateInfo;

struct CasadedInfo
{
	VkFramebuffer frameBuffer;
	VkImageView shadowMapImageView;

	float splitDepth;
	glm::mat4 depthVP;

	void Cleanup(VkDevice device);
};

class CascadedShadow : Shadow
{
private:
	struct ShadowUniformBufferObject {
		glm::mat4 depthVP[CASCADED_COUNT];
		uint32_t cascadedIndex;
	};

	std::array<CasadedInfo, CASCADED_COUNT> casadedInfos;

public:

	CascadedShadow();
	void GetCascadedInfo(glm::mat4* mat, float* splitDepth);
	void UpdateCascaded(glm::mat4 view, glm::mat4 proj, glm::vec4 lightpos);

	virtual void CreateFrameBuffer() override;
	virtual void CreateUniformBuffer() override;
	virtual void CreateShadowMap() override;
	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel) override;

	virtual void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos) override;

	virtual void Cleanup() override;
};

