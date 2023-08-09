#include <stdexcept>

#include "HelloVulkan.h"
#include "PBRTest.h"


void PBRTest::Init(HelloVulkan* app, VkDevice vkdevice, uint32_t w, uint32_t h)
{
	__super::Init(app, vkdevice, w, h);
    bufferSize = sizeof(UniformBufferObject);
	vertexShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/pbrtest.vert.spv";
	fragmentShader = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/pbrtest.frag.spv";

	// Setup some default materials (source: https://seblagarde.wordpress.com/2011/08/17/feeding-a-physical-based-lighting-mode/)
	materials.push_back(Material("Gold", glm::vec3(1.0f, 0.765557f, 0.336057f), 0.1f, 1.0f));
	materials.push_back(Material("Copper", glm::vec3(0.955008f, 0.637427f, 0.538163f), 0.1f, 1.0f));
	materials.push_back(Material("Chromium", glm::vec3(0.549585f, 0.556114f, 0.554256f), 0.1f, 1.0f));
	materials.push_back(Material("Nickel", glm::vec3(0.659777f, 0.608679f, 0.525649f), 0.1f, 1.0f));
	materials.push_back(Material("Titanium", glm::vec3(0.541931f, 0.496791f, 0.449419f), 0.1f, 1.0f));
	materials.push_back(Material("Cobalt", glm::vec3(0.662124f, 0.654864f, 0.633732f), 0.1f, 1.0f));
	materials.push_back(Material("Platinum", glm::vec3(0.672411f, 0.637331f, 0.585456f), 0.1f, 1.0f));
	// Testing materials
	materials.push_back(Material("White", glm::vec3(1.0f), 0.1f, 1.0f));
	materials.push_back(Material("Red", glm::vec3(1.0f, 0.0f, 0.0f), 0.1f, 1.0f));
	materials.push_back(Material("Blue", glm::vec3(0.0f, 0.0f, 1.0f), 0.1f, 1.0f));
	materials.push_back(Material("Black", glm::vec3(0.0f), 0.1f, 1.0f));
}

void PBRTest::CreateDescriptSetLayout()
{
	VkDescriptorSetLayoutBinding binding;
	binding.binding = 0;
	binding.descriptorCount = 1;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding.pImmutableSamplers = nullptr;
	binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.bindingCount = 1;
	info.pBindings = &binding;

	if (vkCreateDescriptorSetLayout(device, &info, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create SinglePipeline descriptor set layout!");
	}
}

void PBRTest::CreatePipeline(PipelineCreateInfo& pipelineCreateInfo, VkGraphicsPipelineCreateInfo& creatInfo)
{
	auto attributeDescriptoins = Vertex::getAttributeDescriptions({ Vertex::VertexComponent::Position, Vertex::VertexComponent::Normal});
	auto attributeDescriptionBindings = Vertex::getBindingDescription();

	pipelineCreateInfo.vertexInputInfo.vertexBindingDescriptionCount = 1;
	pipelineCreateInfo.vertexInputInfo.pVertexBindingDescriptions = &attributeDescriptionBindings;
	pipelineCreateInfo.vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptoins.data();
	pipelineCreateInfo.vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptoins.size();

	pipelineCreateInfo.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineCreateInfo.rasterizer.depthClampEnable = VK_FALSE;
    pipelineCreateInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL; 
    pipelineCreateInfo.rasterizer.lineWidth = 1.0f;
    pipelineCreateInfo.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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

	pipelineCreateInfo.Apply(creatInfo);

	auto shaderStages = vulkanAPP->CreaterShader(vertexShader, fragmentShader);
	creatInfo.stageCount = 2;
	creatInfo.pStages = shaderStages.data();

	std::array<VkPushConstantRange, 2> pushConstantRanges;
	pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRanges[0].offset = 0;
	pushConstantRanges[0].size = sizeof(glm::vec3);

	pushConstantRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRanges[1].offset = sizeof(glm::vec3);
	pushConstantRanges[1].size = sizeof(Material::PushBlock);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 2; // Optional
	pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data(); // Optional

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

void PBRTest::SetupDescriptSet(VkDescriptorPool pool)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	allocInfo.pNext = nullptr;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate SinglePipeline descriptor set!");
	}

	VkWriteDescriptorSet writeDescriptorSet = {};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet.dstBinding = 0;
	writeDescriptorSet.dstSet = descriptorSet;
	writeDescriptorSet.pBufferInfo = &bufferInfo;
	writeDescriptorSet.pImageInfo = nullptr;
	writeDescriptorSet.dstArrayElement = 0;

	vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
}

void PBRTest::CreateUniformBuffer()
{
	if (bufferSize == 0)
		return;

	vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformMemory);

	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = bufferSize;
}

#define GRID_DIM 8

void PBRTest::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	materialIndex = 8;
	Material mat = materials[materialIndex];

	for (uint32_t y = 0; y < GRID_DIM; y++) {
		for (uint32_t x = 0; x < GRID_DIM; x++) {
			glm::vec3 pos = glm::vec3(float(x - (GRID_DIM / 2.0f)) * 2.5f, 0.0f, float(y - (GRID_DIM / 2.0f)) * 2.5f);
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec3), &pos);
			mat.params.metallic = glm::clamp((float)x / (float)(GRID_DIM - 1) , 0.1f, 1.0f);
			mat.params.roughness = glm::clamp((float)y / (float)(GRID_DIM - 1), 0.05f, 1.0f);
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::vec3), sizeof(Material::PushBlock), &mat);
			gltfmodel.draw(commandBuffer, pipelineLayout, 2, 0);
		}
	}
}

void PBRTest::UpateLightMVP(glm::mat4 view, glm::mat4 proj, glm::vec4 lightPos)
{
	UniformBufferObject ubo;
	ubo.lightPos[0] = ubo.lightPos[1] = ubo.lightPos[2] = ubo.lightPos[3] = lightPos;
	ubo.model = glm::mat4(1.0);
	ubo.view = view;
	ubo.proj = proj;
	ubo.viewPos = vulkanAPP->GetCamera().viewPos;

	Trans_Data_To_GPU
}

void PBRTest::Cleanup()
{
	__super::Cleanup();
}
