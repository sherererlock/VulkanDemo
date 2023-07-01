#pragma once

#include "Shadow.h"


class CommonShadow : public Shadow
{
private:

	struct ShadowUniformBufferObject {
		glm::mat4 depthVP;
	};

	VkFramebuffer frameBuffer;

public:
	CommonShadow();
	virtual	void CreateUniformBuffer() override;
	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel) override;
	virtual void CreateFrameBuffer() override;
	virtual void Cleanup() override;

	virtual void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos) override;
};

