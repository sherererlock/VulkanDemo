#include <array>
#include <stdexcept>

#include "HelloVulkan.h"
#include "SkyboxRenderer.h"

const std::string SKYBOX_PATH = "D:/Games/VulkanDemo/VulkanDemo/models/cube.gltf";
const std::string TEXTURE_PATH = "D:/Games/VulkanDemo/VulkanDemo/textures/hdr/gcanyon_cube.ktx";

void SkyboxRenderer::LoadSkyBox()
{
	vulkanAPP->loadgltfModel(SKYBOX_PATH, skyboxModel);
	cubeMap.loadFromFile(vulkanAPP, TEXTURE_PATH, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
}

void SkyboxRenderer::Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h)
{
	__super::Init(app, vkdevice, w, h);
	vertexShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/skybox.vert.spv";
	fragmentShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/skybox.frag.spv";
	bufferSize = sizeof(UniformBufferObject);
}

void SkyboxRenderer::CreatePipeline(PipelineCreateInfo& pipelineCreateInfo, VkGraphicsPipelineCreateInfo& creatInfo)
{
	auto attributeDescriptions = Vertex::getAttributeDescriptions({ Vertex::VertexComponent::Position, Vertex::VertexComponent::Normal, Vertex::VertexComponent::UV });
	auto vertexInputDescriptionBindings = Vertex::getBindingDescription();

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
	 //pipelineCreateInfo.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
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

	pipelineCreateInfo.Apply(creatInfo);

	auto shaderStages = vulkanAPP->CreaterShader(vertexShader, fragmentShader);
	creatInfo.stageCount = (uint32_t)shaderStages.size();
	creatInfo.pStages = shaderStages.data();

	struct SpecializationData
	{
		float exposure = 4.5f;
		float gamma = 2.2f;
	}specializationData;

	std::array<VkSpecializationMapEntry, 2> entries;

	entries[0].constantID = 0;
	entries[0].offset = offsetof(SpecializationData, exposure);
	entries[0].size = sizeof(SpecializationData::exposure);

	entries[1].constantID = 1;
	entries[1].offset = offsetof(SpecializationData, gamma);
	entries[1].size = sizeof(SpecializationData::gamma);

	VkSpecializationInfo specializationInfo{};
	specializationInfo.mapEntryCount = 2;
	specializationInfo.pMapEntries = entries.data();
	specializationInfo.dataSize = sizeof(specializationData);
	specializationInfo.pData = &specializationData;

	shaderStages[1].pSpecializationInfo = &specializationInfo;

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
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate shadow descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = bufferSize;

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

	std::array<VkWriteDescriptorSet, 2> descriptorWrites = { descriptorWrite, descriptorWrite };
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].pBufferInfo = nullptr;
	descriptorWrites[1].pImageInfo = &cubeMap.descriptor; // Optional

	vkUpdateDescriptorSets(device, 2, descriptorWrites.data(), 0, nullptr);
}

void SkyboxRenderer::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	skyboxModel.draw(commandBuffer, pipelineLayout, 2, 0);
}

void SkyboxRenderer::UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos)
{
	UniformBufferObject ubo;
	ubo.model = view;
	ubo.projection = proj;
	ubo.model = glm::scale(ubo.model, glm::vec3(10.0f));

	Trans_Data_To_GPU
}

void SkyboxRenderer::Cleanup()
{
	skyboxModel.Cleanup();
	cubeMap.destroy();

	__super::Cleanup();
}

