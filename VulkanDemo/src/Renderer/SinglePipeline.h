#pragma once
#include <string>

#include <glm/glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "macros.h"
#include "Renderer.h"

class HelloVulkan;
class gltfModel;
struct PipelineCreateInfo;

class SinglePipeline : public Renderer
{

protected:
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformMemory;
	VkDescriptorBufferInfo bufferInfo;

	VkDevice device;
	uint32_t width, height;
	HelloVulkan* vulkanAPP;

	std::string vertexShader;
	std::string fragmentShader;

	VkDeviceSize bufferSize;
public:

	void CreateAttachment(FrameBufferAttachment* attachment, VkFormat format, VkImageUsageFlags usage);

	virtual void Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h);
	virtual void CreatePass() {}
	virtual void CreateDescriptSetLayout();
	virtual void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo);
	virtual void SetupDescriptSet(VkDescriptorPool pool);
	virtual void CreateFrameBuffer(){}
	virtual void CreateGBuffer() {}
	virtual	void CreateUniformBuffer();
	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel) = 0;
	virtual void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos) {};
	virtual void Cleanup();
};

