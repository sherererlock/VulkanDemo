#include <stdexcept>

#include "HelloVulkan.h"
#include "CascadedShadow.h"

CascadedShadow::CascadedShadow()
{
    viewType = VK_IMAGE_VIEW_TYPE_2D;
    layerCount = 1;
    size = sizeof(ShadowUniformBufferObject);
}

void CascadedShadow::GetCascadedInfo(glm::mat4* mat, float* splitDepth)
{
    for (int i = 0; i < casadedInfos.size(); i++)
    {
        glm::mat4 depthvp = casadedInfos[i].depthVP;
        mat[i] = depthvp;
        splitDepth[i] = casadedInfos[i].splitDepth;
    }
}

void CascadedShadow::CreateFrameBuffer()
{
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = shadowPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = 1;

    for (int i = 0; i < CASCADED_COUNT; i++)
    {
        framebufferInfo.pAttachments = &casadedInfos[i].shadowMapImageView;
		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &casadedInfos[i].frameBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shadow map framebuffer!");
		}
    }
}

void CascadedShadow::CreateUniformBuffer()
{
    VkDeviceSize bufferSize = sizeof(ShadowUniformBufferObject);
    vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformMemory);
}

void CascadedShadow::CreateShadowMap()
{
	viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    layerCount = CASCADED_COUNT;

    Shadow::CreateShadowMap();

    VkImageViewCreateInfo depthStencilView = {};
	depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthStencilView.viewType = viewType;
	depthStencilView.format = DEPTH_FORMAT;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.layerCount = CASCADED_COUNT;
	depthStencilView.image = shadowMapImage;
    depthStencilView.subresourceRange.baseArrayLayer = 0;

    for (int i = 0; i < CASCADED_COUNT; i++)
    {
        depthStencilView.subresourceRange.layerCount = 1;
        depthStencilView.subresourceRange.baseArrayLayer = i;
		vkCreateImageView(device, &depthStencilView, nullptr, &casadedInfos[i].shadowMapImageView);
    }
}

void CascadedShadow::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = shadowPass;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent.width = width;
    renderPassInfo.renderArea.extent.height = height;

    VkClearValue clearValue;
    clearValue.depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) width;
    viewport.height = (float) height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent.width = width;
    scissor.extent.height = height;

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdSetDepthBias(
        commandBuffer,
		depthBiasConstant,
		0.0f,
		depthBiasSlope);

    for (int i = 0; i < CASCADED_COUNT; i++)
    {
        renderPassInfo.framebuffer = casadedInfos[i].frameBuffer;
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

        gltfmodel.cascadedIndex = i;
        //gltfmodel.draw(commandBuffer, pipelineLayout, 1);
        gltfmodel.drawWithOffset(commandBuffer, pipelineLayout, 1);

		vkCmdEndRenderPass(commandBuffer);
    }
}

void CascadedShadow::UpdateCascaded(glm::mat4 view, glm::mat4 proj, glm::vec4 lightpos)
{
    float cascadeSplitLambda = 0.95f;

	float cascadeSplits[CASCADED_COUNT];

	float nearClip = vulkanAPP->GetNear();
	float farClip = vulkanAPP->GetFar();
	float clipRange = farClip - nearClip;

	float minZ = nearClip;
	float maxZ = nearClip + clipRange;

	float range = maxZ - minZ;
	float ratio = maxZ / minZ;

	// Calculate split depths based on view camera frustum
	// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (uint32_t i = 0; i < CASCADED_COUNT; i++)
    {
        float p = (i + 1) / static_cast<float>(CASCADED_COUNT);
        float log = minZ * std::pow(ratio, p);
        float uniform = minZ + range * p;
        float d = cascadeSplitLambda * (log - uniform) + uniform;
        cascadeSplits[i] = (d - nearClip) / clipRange;
    }


    float lastSplitDist = 0.0;
    for (int i = 0; i < CASCADED_COUNT; i++)
    {
		glm::vec3 frustumCorners[8] = {
			glm::vec3(-1.0f,  1.0f, 0.0f),
			glm::vec3(1.0f,  1.0f, 0.0f),
			glm::vec3(1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f, -1.0f,  1.0f),
			glm::vec3(-1.0f, -1.0f,  1.0f),
		};

        glm::mat4 invView = glm::inverse(view);
        glm::mat4 invProj = glm::inverse(proj);

        for (int i = 0; i < 8; i++)
        {
            glm::vec4 cornor = glm::vec4(frustumCorners[i], 1.0);
            cornor = invProj * cornor;
            cornor = cornor / cornor.w;
            cornor = invView * cornor;

            frustumCorners[i] = cornor;
        }

        float splitDist = cascadeSplits[i];
        for (int i = 0; i < 4; i++)
        {
            glm::vec3 dist = (frustumCorners[i + 4] - frustumCorners[i]);
            frustumCorners[i + 4] = frustumCorners[i] + dist * splitDist;
            frustumCorners[i] = frustumCorners[i] + dist * lastSplitDist;
        }

        glm::vec3 center = glm::vec3(0.0);
        for (int i = 0; i < 8; i++)
        {
            center += frustumCorners[i];
        }

        center /= 8.0f;

        float radius = 0.0f;
		for (int i = 0; i < 8; i++)
		{
			float dist = glm::length(frustumCorners[i] - center);
            radius = glm::max(radius, dist);
		}
        radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 maxExtents = glm::vec3(radius);
        glm::vec3 minExtents = -maxExtents;

		glm::vec3 lightDir = normalize(-lightpos);
		glm::mat4 lightViewMatrix = glm::lookAt(center - lightDir * -minExtents.z, center, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

		// Store split distance and matrix in cascade
		casadedInfos[i].splitDepth = (nearClip + splitDist * clipRange) * -1.0f;
        casadedInfos[i].depthVP = lightOrthoMatrix * lightViewMatrix;

		lastSplitDist = cascadeSplits[i];
    }
}

void CascadedShadow::UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos)
{
    UpdateCascaded(view, proj, lightPos);

    ShadowUniformBufferObject ubo;

    for (int i = 0; i < CASCADED_COUNT; i++)
        ubo.depthVP[i] = casadedInfos[i].depthVP;

    void* data;
    vkMapMemory(device, uniformMemory, 0, sizeof(ShadowUniformBufferObject), 0, &data);
    memcpy(data, &ubo, sizeof(ShadowUniformBufferObject));
    vkUnmapMemory(device, uniformMemory);
}

void CascadedShadow::Cleanup()
{
    for(CasadedInfo& info : casadedInfos)
    {
        info.Cleanup(device);
    }
    
    Shadow::Cleanup();
}

void CasadedInfo::Cleanup(VkDevice device)
{
    vkDestroyFramebuffer(device, frameBuffer, nullptr);
    vkDestroyImageView(device, shadowMapImageView, nullptr);
}
