#include <stdexcept>

#include "CommonShadow.h"
#include "HelloVulkan.h"

CommonShadow::CommonShadow()
{
    viewType = VK_IMAGE_VIEW_TYPE_2D;
    layerCount = 1;
    size = sizeof(ShadowUniformBufferObject);
}

void CommonShadow::CreateUniformBuffer()
{
    VkDeviceSize bufferSize = sizeof(ShadowUniformBufferObject);
    vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformMemory);
}

void CommonShadow::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
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

    renderPassInfo.framebuffer = frameBuffer;
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);
	gltfmodel.draw(commandBuffer, pipelineLayout, 1, 2);
	vkCmdEndRenderPass(commandBuffer);
}

void CommonShadow::CreateFrameBuffer()
{
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = shadowPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = 1;
    framebufferInfo.pAttachments = &shadowMapImageView;

	if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shadow map framebuffer!");
	}
}

void CommonShadow::UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos)
{
    ShadowUniformBufferObject ubo;
    ubo.depthVP = proj * view;

    void* data;
    vkMapMemory(device, uniformMemory, 0, sizeof(ShadowUniformBufferObject), 0, &data);
    memcpy(data, &ubo, sizeof(ShadowUniformBufferObject));
    vkUnmapMemory(device, uniformMemory);
}

void CommonShadow::Cleanup()
{
	Shadow::Cleanup();
    vkDestroyFramebuffer(device, frameBuffer, nullptr);
}