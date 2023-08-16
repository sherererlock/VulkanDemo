
#include <array>
#include <stdexcept>

#include "HelloVulkan.h"
#include "PBRLighting.h"

void PBRLighting::CreateDescriptSetLayout()
{
    VkDescriptorSetLayoutBinding binding;

    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags =  VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.pImmutableSamplers = nullptr; // Optional

   std::array<VkDescriptorSetLayoutBinding, 5> bindings;
   bindings.fill(binding);

   for (int i = 1; i < 5; i++)
        bindings[i].binding = i;

	bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void PBRLighting::CreatePipeline(PipelineCreateInfo& info, VkGraphicsPipelineCreateInfo& creatInfo)
{
	std::array<VkPushConstantRange, 2> pushConstantRanges;
	pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRanges[0].offset = 0;
	pushConstantRanges[0].size = sizeof(glm::mat4);

	pushConstantRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRanges[1].offset = sizeof(glm::mat4);
	pushConstantRanges[1].size = sizeof(float);

    std::vector<VkDescriptorSetLayout> setLayouts = vulkanAPP->GetDescriptorSetLayouts();
    setLayouts.push_back(descriptorSetLayout);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = (uint32_t)setLayouts.size(); // Optional
    pipelineLayoutInfo.pSetLayouts = setLayouts.data(); // Optional
    pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size(); // Optional
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data(); // Optional

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    auto attributeDescriptoins = Vertex::getAttributeDescriptions();
    auto attributeDescriptionBindings = Vertex::getBindingDescription();

    info.vertexInputInfo.pVertexBindingDescriptions = &attributeDescriptionBindings; // Optional
    info.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptoins.size());
    info.vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptoins.data(); // Optional

    info.rasterizer.depthClampEnable = VK_FALSE;
    info.rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // VK_POLYGON_MODE_LINE
    //pipelineCreateInfo.rasterizer.polygonMode = VK_POLYGON_MODE_LINE; // VK_POLYGON_MODE_LINE

    info.rasterizer.lineWidth = 1.0f;

    info.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    info.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    info.rasterizer.depthBiasEnable = VK_FALSE;
    info.rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    info.rasterizer.depthBiasClamp = 0.0f; // Optional
    info.rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    info.dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    info.dynamicState.dynamicStateCount = 2;
    info.dynamicState.pDynamicStates = dynamicStates;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    info.colorBlending.pAttachments = &colorBlendAttachment;

    auto shaderStages = vulkanAPP->CreaterShader(vertexShader, fragmentShader);

    creatInfo.layout = pipelineLayout; 
    creatInfo.stageCount = (uint32_t)shaderStages.size();
    creatInfo.pStages = shaderStages.data();
    creatInfo.subpass = 0; 
    creatInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    creatInfo.basePipelineIndex = -1; // Optional

    info.Apply(creatInfo);

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &creatInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}

void PBRLighting::SetupDescriptSet(VkDescriptorPool pool)
{
    std::array<VkWriteDescriptorSet, 5> descriptorWrites = {};

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    for (gltfModel* gltfmodel : models)
    {
        for (int i = 0; i < gltfmodel->materials.size(); i++)
        {
            Material& material = gltfmodel->materials[i];
            if (vkAllocateDescriptorSets(device, &allocInfo, &material.descriptorSet) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate descriptor set!");
            }

		    VkDeviceSize bufferSize = sizeof(Material::MaterialData);
		    vulkanAPP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, material.materialBuffer, material.materialBufferMemory);
            material.UpdateMaterialBuffer(device);

            uint32_t indices[4] = {material.baseColorTextureIndex, material.normalTextureIndex,  material.roughnessTextureIndex, material.emissiveTextureIndex };
            for (int j = 0; j < 4; j++)
            {
                descriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[j].dstSet = material.descriptorSet;
                descriptorWrites[j].dstBinding = j;
                descriptorWrites[j].dstArrayElement = 0;
                descriptorWrites[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[j].descriptorCount = 1;
                descriptorWrites[j].pBufferInfo = nullptr;
			    if (indices[j] != -1)
                    descriptorWrites[j].pImageInfo =  &gltfmodel->images[indices[j]].texture.descriptor; // Optional
                else
                    descriptorWrites[j].pImageInfo = &vulkanAPP->GetEmptyTexture().descriptor; // Optional

                descriptorWrites[j].pTexelBufferView = nullptr; // Optional
            }

		    bufferInfo.buffer = material.materialBuffer;
		    bufferInfo.offset = 0;
		    bufferInfo.range = sizeof(Material::MaterialData);

		    descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		    descriptorWrites[4].dstSet = material.descriptorSet;
		    descriptorWrites[4].dstBinding = 4;
		    descriptorWrites[4].dstArrayElement = 0;
		    descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		    descriptorWrites[4].descriptorCount = 1;
		    descriptorWrites[4].pBufferInfo = &bufferInfo;
		    descriptorWrites[4].pImageInfo = nullptr; // Optional
		    descriptorWrites[4].pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
}

void PBRLighting::BuildCommandBuffer(VkCommandBuffer commandBuffer, const gltfModel& gltfmodel)
{
    auto descriptorSets = vulkanAPP->GetDescriptorSets();
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[0], 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &descriptorSets[1], 0, nullptr);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    if (CASCADED_COUNT == 1)
    {
        gltfmodel.draw(commandBuffer, pipelineLayout, 0, 2);
    }
    else
    {
        gltfmodel.drawWithOffset(commandBuffer, pipelineLayout, 0, 2);
    }
}

