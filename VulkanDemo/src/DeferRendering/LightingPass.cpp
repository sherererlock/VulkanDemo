

#include <stdexcept>

#include "HelloVulkan.h"
#include "Shadow.h"
#include "LightingPass.h"
#include "BasePass.h"

void LightingPass::Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h)
{
	__super::Init(app, vkdevice, w, h);
	vertexShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/quad.vert.spv";
	fragmentShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/LightingPass.frag.spv";
	bufferSize = sizeof(UniformBufferObject);

	basePass = vulkanAPP->GetBasePass();
}

void LightingPass::CreatePass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = vulkanAPP->GetFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // ������
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // �洢����
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = nullptr;
	subpass.pResolveAttachments = nullptr;

	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;

	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; 
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT; 

	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; 
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; 
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

void LightingPass::CreateFrameBuffer()
{
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &color.view;
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create framebuffer!");
	}
}

void LightingPass::CreateGBuffer()
{
	CreateAttachment(&color, vulkanAPP->GetFormat(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
}

void LightingPass::CreateDescriptSetLayout()
{
	VkDescriptorSetLayoutBinding uniformLayoutBinding = {};

	uniformLayoutBinding.binding = 0;
	uniformLayoutBinding.descriptorCount = 1;
	uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	uniformLayoutBinding.pImmutableSamplers = nullptr;
	uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 9> uniformLayoutBindings;
	for (int i = 0; i < 9; i++)
	{
		uniformLayoutBindings[i] = uniformLayoutBinding;
		uniformLayoutBindings[i].binding = i;
	}

	uniformLayoutBindings[8].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = (uint32_t)uniformLayoutBindings.size();
	layoutInfo.pBindings = uniformLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create SinglePipeline descriptor set layout!");
	}
}

void LightingPass::CreatePipeline(PipelineCreateInfo& pipelineCreateInfo, VkGraphicsPipelineCreateInfo& creatInfo)
{
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = { };
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	creatInfo.pVertexInputState = &vertexInputCreateInfo;

	pipelineCreateInfo.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineCreateInfo.rasterizer.depthClampEnable = VK_FALSE;
	pipelineCreateInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineCreateInfo.rasterizer.lineWidth = 1.0f;
	pipelineCreateInfo.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineCreateInfo.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pipelineCreateInfo.rasterizer.depthBiasEnable = VK_FALSE;
	pipelineCreateInfo.rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	pipelineCreateInfo.rasterizer.depthBiasClamp = 0.0f; // Optional
	pipelineCreateInfo.rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	pipelineCreateInfo.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineCreateInfo.multisampling.sampleShadingEnable = VK_FALSE;
	pipelineCreateInfo.multisampling.rasterizationSamples = vulkanAPP->GetSampleCountFlag();
	pipelineCreateInfo.multisampling.minSampleShading = 0.0f; // Optional
	pipelineCreateInfo.multisampling.pSampleMask = nullptr; // Optional
	pipelineCreateInfo.multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	pipelineCreateInfo.multisampling.alphaToOneEnable = VK_FALSE; // Optional

	pipelineCreateInfo.colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipelineCreateInfo.colorBlending.logicOpEnable = VK_FALSE;
	pipelineCreateInfo.colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	pipelineCreateInfo.colorBlending.blendConstants[0] = 0.0f; // Optional
	pipelineCreateInfo.colorBlending.blendConstants[1] = 0.0f; // Optional
	pipelineCreateInfo.colorBlending.blendConstants[2] = 0.0f; // Optional
	pipelineCreateInfo.colorBlending.blendConstants[3] = 0.0f; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	pipelineCreateInfo.colorBlending.pAttachments = &colorBlendAttachment;
	pipelineCreateInfo.colorBlending.attachmentCount = 1;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	pipelineCreateInfo.dynamicState.dynamicStateCount = 2;
	pipelineCreateInfo.dynamicState.pDynamicStates = dynamicStates;

	pipelineCreateInfo.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineCreateInfo.depthStencil.depthTestEnable = VK_TRUE;
	pipelineCreateInfo.depthStencil.depthWriteEnable = VK_TRUE;
	pipelineCreateInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	auto shaderStages = vulkanAPP->CreaterShader(vertexShader, fragmentShader);
	creatInfo.stageCount = 2;
	creatInfo.pStages = shaderStages.data();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	creatInfo.renderPass = renderPass;
	creatInfo.layout = pipelineLayout;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &creatInfo, nullptr, &pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}

void LightingPass::SetupDescriptSet(VkDescriptorPool pool)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate SinglePipeline descriptor set!");
	}

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = nullptr;
	descriptorWrite.pImageInfo = nullptr; // Optional
	descriptorWrite.pTexelBufferView = nullptr; // Optional

	std::array<VkWriteDescriptorSet, 9> descriptorWrites;
	descriptorWrites.fill(descriptorWrite);
	std::array<VkDescriptorImageInfo, 8> imageInfos =
	{
		basePass->GetPositionDescriptorImageInfo(),
		basePass->GetNormalDescriptorImageInfo(),
		basePass->GetRoughnessDescriptorImageInfo(),
		basePass->GetAlbedoDescriptorImageInfo(),
		basePass->GetEmissiveDescriptorImageInfo(),
		vulkanAPP->GetEmu().descriptor,
		vulkanAPP->GetEavg().descriptor,
		vulkanAPP->GetShadow()->GetDescriptorImageInfo()
	};

	for (int i = 0; i < 8; i++)
	{
		descriptorWrites[i].dstBinding = i;
		descriptorWrites[i].pImageInfo = &imageInfos[i];
	}

	descriptorWrites[8].dstBinding = 8;
	descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[8].pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(device, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void LightingPass::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = frameBuffer;

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = {width, height};

	VkClearValue clearValue = {};
	clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)width;
	viewport.height = (float) height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { width, height };

	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearValue;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
}

void LightingPass::UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::mat4 depthVP, glm::vec4 lightpos)
{
	UniformBufferObject ubo;
	ubo.view = view;
	ubo.projection = proj;
	ubo.viewPos = vulkanAPP->GetCamera().viewPos;
	ubo.depthVP = depthVP;

	ubo.lightPos[0] = lightpos;

	glm::mat4 rotation;
	glm::vec3 yaxis(0.0f, 1.0f, 0.0f);
	rotation = glm::rotate(rotation, glm::radians(45.0f), yaxis);
	ubo.lightPos[1] = rotation * lightpos;

	rotation = glm::rotate(rotation, glm::radians(90.0f), yaxis);
	ubo.lightPos[1] = rotation * lightpos;

	rotation = glm::rotate(rotation, glm::radians(135.0f), yaxis);
	ubo.lightPos[2] = lightpos;

	rotation = glm::rotate(rotation, glm::radians(180.0f), yaxis);
	ubo.lightPos[3] = lightpos;

	Trans_Data_To_GPU
}

void LightingPass::Cleanup()
{
	color.Cleanup(device);
	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroyFramebuffer(device, frameBuffer, nullptr);

	__super::Cleanup();
}
