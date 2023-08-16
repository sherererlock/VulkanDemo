#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm/glm.hpp>

class HelloVulkan;
class gltfModel;

struct PipelineCreateInfo
{
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineColorBlendStateCreateInfo colorBlending;
    VkPipelineDynamicStateCreateInfo dynamicState;
    VkPipelineDepthStencilStateCreateInfo depthStencil;

    PipelineCreateInfo() : vertexInputInfo({}), inputAssembly({}), viewportState({}), rasterizer({}),
        multisampling({}), colorBlending({}), dynamicState({}), depthStencil({})
    {        
    }

    void Apply(VkGraphicsPipelineCreateInfo& createInfo) const
    {
        createInfo.pVertexInputState = &vertexInputInfo; // bindings and attribute
        createInfo.pInputAssemblyState = &inputAssembly; // topology
        createInfo.pViewportState = &viewportState; 
        createInfo.pRasterizationState = &rasterizer;
        createInfo.pMultisampleState = &multisampling;
        createInfo.pDepthStencilState = &depthStencil; // Optional
        createInfo.pColorBlendState = &colorBlending;
        createInfo.pDynamicState = &dynamicState; // Optional
    }
};

struct FrameBufferAttachment
{
	VkImage image;
	VkDeviceMemory mem;
	VkImageView view;
	VkFormat format;
	VkSampler sampler;
	VkDescriptorImageInfo descriptor;

	void Cleanup(VkDevice device)
	{
		vkDestroyImage(device, image, nullptr);
		vkDestroyImageView(device, view, nullptr);
		vkFreeMemory(device, mem, nullptr);
		vkDestroySampler(device, sampler, nullptr);
	}

	void UpdateDescriptor()
	{
		descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptor.imageView = view;
		descriptor.sampler = sampler;
	}
};


class Renderer
{
public:

	virtual void Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h) = 0;
    virtual void CreatePass() = 0;
	virtual void CreateDescriptSetLayout() = 0;
	virtual void CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo) = 0;
	virtual void SetupDescriptSet(VkDescriptorPool pool) = 0;
	virtual void CreateFrameBuffer() = 0;
    virtual void CreateGBuffer() = 0;
	virtual	void CreateUniformBuffer() = 0;
	virtual void BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel) = 0;
	virtual void UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos) = 0;
	virtual void Cleanup() = 0;
};