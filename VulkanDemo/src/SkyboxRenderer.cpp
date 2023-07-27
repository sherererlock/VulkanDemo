#include <array>
#include <stdexcept>

#include "HelloVulkan.h"
#include "SkyboxRenderer.h"

void SkyboxRenderer::Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h)
{
	__super::Init(app, vkdevice, w, h);
	vertexShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/skybox.vert.spv";
	fragmentShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/skybox.frag.spv";
}

void SkyboxRenderer::CreatePipeline(PipelineCreateInfo& pipelineCreateInfo, VkGraphicsPipelineCreateInfo& creatInfo)
{
	auto attributeDescriptions = Vertex1::getAttributeDescriptions({ Vertex1::VertexComponent::Position, Vertex1::VertexComponent::Normal, Vertex1::VertexComponent::UV });
	auto vertexInputDescriptionBindings = Vertex1::getBindingDescription();

	pipelineCreateInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineCreateInfo.vertexInputInfo.vertexBindingDescriptionCount = 1;
	pipelineCreateInfo.vertexInputInfo.pVertexBindingDescriptions = &vertexInputDescriptionBindings;
	pipelineCreateInfo.vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	pipelineCreateInfo.vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();

	pipelineCreateInfo.inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipelineCreateInfo.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineCreateInfo.inputAssembly.primitiveRestartEnable = VK_FALSE;

	pipelineCreateInfo.viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineCreateInfo.viewportState.viewportCount = 1;
	pipelineCreateInfo.viewportState.scissorCount = 1;

	pipelineCreateInfo.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineCreateInfo.rasterizer.depthClampEnable = VK_FALSE;
	pipelineCreateInfo.rasterizer.rasterizerDiscardEnable = VK_FALSE;
	pipelineCreateInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL; 
	pipelineCreateInfo.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineCreateInfo.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pipelineCreateInfo.rasterizer.depthBiasEnable = VK_FALSE;
	pipelineCreateInfo.rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	pipelineCreateInfo.rasterizer.depthBiasClamp = 0.0f; // Optional
	pipelineCreateInfo.rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
	pipelineCreateInfo.rasterizer.lineWidth = 1.0f;

	pipelineCreateInfo.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineCreateInfo.multisampling.sampleShadingEnable = VK_TRUE;
	 pipelineCreateInfo.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipelineCreateInfo.multisampling.minSampleShading = 0.2f; // Optional
	pipelineCreateInfo.multisampling.pSampleMask = nullptr; // Optional
	pipelineCreateInfo.multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	pipelineCreateInfo.multisampling.alphaToOneEnable = VK_FALSE; // Optional

	pipelineCreateInfo.colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipelineCreateInfo.colorBlending.logicOpEnable = VK_FALSE;
	pipelineCreateInfo.colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	pipelineCreateInfo.colorBlending.attachmentCount = 1;
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

	pipelineCreateInfo.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineCreateInfo.depthStencil.depthTestEnable = VK_TRUE;
	pipelineCreateInfo.depthStencil.depthWriteEnable = VK_TRUE;
	pipelineCreateInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	pipelineCreateInfo.dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineCreateInfo.dynamicState.dynamicStateCount = 2;
	pipelineCreateInfo.dynamicState.pDynamicStates = dynamicStates;

	creatInfo.pVertexInputState = &pipelineCreateInfo.vertexInputInfo; // bindings and attribute
	creatInfo.pInputAssemblyState = &pipelineCreateInfo.inputAssembly; // topology
	creatInfo.pViewportState = &pipelineCreateInfo.viewportState;
	creatInfo.pRasterizationState = &pipelineCreateInfo.rasterizer;
	creatInfo.pMultisampleState = &pipelineCreateInfo.multisampling;
	creatInfo.pDepthStencilState = &pipelineCreateInfo.depthStencil; // Optional
	creatInfo.pColorBlendState = &pipelineCreateInfo.colorBlending;
	creatInfo.pDynamicState = &pipelineCreateInfo.dynamicState; // Optional

	auto shaderStages = vulkanAPP->CreaterShader(vertexShader, fragmentShader);
	creatInfo.stageCount = (uint32_t)shaderStages.size();
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

	creatInfo.layout = pipelineLayout;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &creatInfo, nullptr, &pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}

void SkyboxRenderer::CreateDescriptSetLayout()
{
	VkDescriptorSetLayoutBinding uniformLayoutBinding = {};
	uniformLayoutBinding.binding = 0;
	uniformLayoutBinding.descriptorCount = 1;
	uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformLayoutBinding.pImmutableSamplers = nullptr;
	uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> uniformLayoutBindings = { uniformLayoutBinding, uniformLayoutBinding };
	uniformLayoutBindings[1].binding = 1;
	uniformLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = (uint32_t)uniformLayoutBindings.size();
	layoutInfo.pBindings = uniformLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void SkyboxRenderer::SetupDescriptSet(VkDescriptorPool pool)
{
}

void SkyboxRenderer::CreateUniformBuffer()
{
}

void SkyboxRenderer::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
}

void SkyboxRenderer::UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos)
{
}

void SkyboxRenderer::Cleanup()
{
}
