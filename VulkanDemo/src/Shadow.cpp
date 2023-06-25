
#include "HelloVulkan.h"
#include "Shadow.h"

#include <stdexcept>

void Shadow::Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h)
{
    vulkanAPP = app;
    device = vkdevice;
    width = w;
    height = h;
}

void Shadow::CreateShadowPipeline(PipelineCreateInfo&  pipelineCreateInfo,  VkGraphicsPipelineCreateInfo& creatInfo)
{
    auto attributeDescriptoins = Vertex1::getAttributeDescriptions();
    auto attributeDescriptionBindings = Vertex1::getBindingDescription();

    pipelineCreateInfo.vertexInputInfo.pVertexBindingDescriptions = &attributeDescriptionBindings; // Optional
    pipelineCreateInfo.vertexInputInfo.vertexAttributeDescriptionCount = 1;
    pipelineCreateInfo.vertexInputInfo.pVertexAttributeDescriptions = &attributeDescriptoins[0]; // Optional

    auto shaderStages = vulkanAPP->CreaterShader("./shaders/GLSL/shadow.vert.spv", "./shaders/GLSL/shadow.frag.spv");

    // No blend attachment states (no color attachments used)
    pipelineCreateInfo.colorBlending.attachmentCount = 0;

    // Disable culling, so all faces contribute to shadows
    pipelineCreateInfo.rasterizer.cullMode = VK_CULL_MODE_NONE;
    pipelineCreateInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	// Enable depth bias
	pipelineCreateInfo.rasterizer.depthBiasEnable = VK_TRUE;

    /*
    * ​x validation layer: Validation Error: [ VUID-VkGraphicsPipelineCreateInfo-multisampledRenderToSingleSampled-06853 ] Object 0: handle = 0x2c38e531f70, type = VK_OBJECT_TYPE_DEVICE; | MessageID = 0x3108bb9b | vkCreateGraphicsPipelines: pCreateInfo[0].pMultisampleState->rasterizationSamples (8) does not match the number of samples of the RenderPass color and/or depth attachment. The Vulkan spec states: If the pipeline is being created with fragment output interface state, and none of the VK_AMD_mixed_attachment_samples extension, the VK_NV_framebuffer_mixed_samples extension, or the multisampledRenderToSingleSampled feature are enabled, rasterizationSamples is not dynamic, and if subpass uses color and/or depth/stencil attachments, then the rasterizationSamples member of pMultisampleState must be the same as the sample count for those subpass attachments (https://vulkan.lunarg.com/doc/view/1.3.243.0/windows/1.3-extensions/vkspec.html#VUID-VkGraphicsPipelineCreateInfo-multisampledRenderToSingleSampled-06853) 
    */
    pipelineCreateInfo.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    pipelineCreateInfo.dynamicState.dynamicStateCount = 2;
    pipelineCreateInfo.dynamicState.pDynamicStates = dynamicStates;

    creatInfo.stageCount = 1;
    creatInfo.pStages = shaderStages.data();

    creatInfo.renderPass = shadowPass;

    VkPushConstantRange pushConstantRange;
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; // Optional
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    creatInfo.layout = pipelineLayout;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &creatInfo, nullptr, &shadowPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}

void Shadow::CreateShadowPass()
{
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D16_UNORM;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // Attachment will be transitioned to shader read at render pass end

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // 图像渲染的子流程

    subpass.colorAttachmentCount = 0;
    subpass.pColorAttachments = nullptr; // fragment shader使用 location = 0 outcolor,输出
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = nullptr;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;   //  A subpass layout发生转换的阶段
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; //  B subpass 等待执行的阶段
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT; // A subpass 中需要完成的操作
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; // b subpass等待执行操作
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &depthAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    
    renderPassInfo.dependencyCount = dependencies.size();
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &shadowPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void Shadow::CreateDescriptSetLayout()
{
    VkDescriptorSetLayoutBinding uniformLayoutBinding = {};

    uniformLayoutBinding.binding = 0;
    uniformLayoutBinding.descriptorCount = 1;
    uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformLayoutBinding.pImmutableSamplers = nullptr;
    uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uniformLayoutBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow descriptor set layout!");
    }
}

void Shadow::SetupDescriptSet(VkDescriptorPool pool)
{
    VkDescriptorSetLayout layouts[] = {descriptorSetLayout};
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;

    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to allocate shadow descriptor set!");
    }

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(ShadowUniformBufferObject);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = nullptr; // Optional
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
}

void Shadow::CreateFrameBuffer()
{
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = shadowPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &shadowMapImageView;
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow map framebuffer!");
    }
}

void Shadow::CreateUniformBuffer()
{
    VkDeviceSize bufferSize = sizeof(ShadowUniformBufferObject);
    vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformMemory);
}

void Shadow::CreateShadowMap()
{
	// For shadow mapping we only need a depth attachment
    VkImageCreateInfo image = {};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.extent.width = width;
	image.extent.height = height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.format = VK_FORMAT_D16_UNORM;																// Depth stencil attachment
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;		// We will sample directly from the depth attachment for the shadow mapping
    image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateImage(device, &image, nullptr, &shadowMapImage);

    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(device, shadowMapImage, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = vulkanAPP->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vkAllocateMemory(device, &memAlloc, nullptr, &shadowMapMemory);
	vkBindImageMemory(device, shadowMapImage, shadowMapMemory, 0);

    VkImageViewCreateInfo depthStencilView = {};
    depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = VK_FORMAT_D16_UNORM;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;
	depthStencilView.image = shadowMapImage;
	vkCreateImageView(device, &depthStencilView, nullptr, &shadowMapImageView);


	// Create sampler to sample from to depth attachment
	// Used to sample in the fragment shader for shadowed rendering
	//VkFilter shadowmap_filter = vks::tools::formatIsFilterable(physicalDevice, DEPTH_FORMAT, VK_IMAGE_TILING_OPTIMAL) ?
	//	DEFAULT_SHADOWMAP_FILTER :
	//	VK_FILTER_NEAREST;

    VkSamplerCreateInfo sampler = {};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.maxAnisotropy = 1.0f;

	sampler.magFilter = VK_FILTER_NEAREST;
	sampler.minFilter = VK_FILTER_NEAREST;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.maxAnisotropy = 1.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1.0f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	vkCreateSampler(device, &sampler, nullptr, &shadowMapSampler);
}

void Shadow::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = shadowPass;
    renderPassInfo.framebuffer = frameBuffer;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent.width = width;
    renderPassInfo.renderArea.extent.width = height;

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

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

    gltfmodel.draw(commandBuffer, pipelineLayout, 1);

    vkCmdEndRenderPass(commandBuffer);
}

void Shadow::UpateLightMVP(glm::mat4 translation)
{
    ShadowUniformBufferObject ubo;

    // TODO


    void* data;
    vkMapMemory(device, uniformMemory, 0, sizeof(ShadowUniformBufferObject), 0, &data);
    memcpy(data, &ubo, sizeof(ShadowUniformBufferObject));
    vkUnmapMemory(device, uniformMemory);
}

void Shadow::Cleanup()
{
    vkDestroyImage(device, shadowMapImage, nullptr); 
    vkDestroySampler(device, shadowMapSampler, nullptr);
    vkDestroyImageView(device, shadowMapImageView, nullptr);
    vkFreeMemory(device, shadowMapMemory, nullptr);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    vkDestroyBuffer(device, uniformBuffer, nullptr);
    vkFreeMemory(device, uniformMemory, nullptr);

    vkDestroyPipeline(device, shadowPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, shadowPass, nullptr);

    vkDestroyFramebuffer(device, frameBuffer, nullptr);
}
