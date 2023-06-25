## Misc

```mermaid
classDiagram
class VkFramebuffer
class VkColorImageView
class VkDepthImageView
class VkResolveImageView

VkFramebuffer o--  VkColorImageView
VkFramebuffer o--  VkDepthImageView
VkFramebuffer o--  VkResolveImageView

class VkRenderPass
class VkColorAttachment
class VkDepthAttachment
class VkResolveAttachment

VkRenderPass o-- VkColorAttachment
VkRenderPass o-- VkDepthAttachment
VkRenderPass o-- VkResolveAttachment

class VkSubpass
class VkColorAttachmentRef
class VkDepthAttachmentRef
class VkRolveAttachmentRef

VkSubpass o-- VkColorAttachmentRef
VkSubpass o-- VkDepthAttachmentRef
VkSubpass o-- VkRolveAttachmentRef

VkRenderPass o-- VkSubpass
VkFramebuffer *-- VkRenderPass

```

```mermaid
classDiagram
class PipelineLayout
class DescriptorPool
class VkDescriptorPoolSize{
	+VkDescriptorType type
	+int count
}
DescriptorPool o-- VkDescriptorPoolSize

class DescriptorSetLayout
class VkDescriptorSetLayoutBinding{
	int binding
	int descriptorCount
	VkDescriptorType type
}

DescriptorSetLayout o-- VkDescriptorSetLayoutBinding

class DescriptorSet

class WriteDescriptorSet{
	VkWriteDescriptorSet dstSet
	int dstBinding
	VkDescriptorType descriptorType
	VkDescriptorImageInfo* pImageInfo
	VkDescriptorBufferInfo* pBufferInfo
}

class DescriptorBufferInfo{
	VkBuffer buffer
}

class DescriptorImageInfo{
	VkSampler	sampler
	VkImageView	imageView
	VkImageLayout	imageLayout
}

DescriptorSet o-- WriteDescriptorSet
WriteDescriptorSet o-- DescriptorBufferInfo
WriteDescriptorSet o-- DescriptorImageInfo
```

```c
validation layer: Validation Error: [ VUID-VkGraphicsPipelineCreateInfo-layout-00756 ] Object 0: handle = 0xdcc8fd0000000012, type = VK_OBJECT_TYPE_SHADER_MODULE; Object 1: handle = 0xd175b40000000013, type = VK_OBJECT_TYPE_PIPELINE_LAYOUT; | MessageID = 0x45717876 | vkCreateGraphicsPipelines(): pCreateInfos[0] Set 3 Binding 0 in shader (VK_SHADER_STAGE_FRAGMENT_BIT) uses descriptor slot (expected `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`) but not declared in pipeline layout The Vulkan spec states: layout must be consistent with all shaders specified in pStages (https://vulkan.lunarg.com/doc/view/1.3.243.0/windows/1.3-extensions/vkspec.html#VUID-VkGraphicsPipelineCreateInfo-layout-00756)
validation layer: Validation Error: [ VUID-VkGraphicsPipelineCreateInfo-layout-00756 ] Object 0: handle = 0xdcc8fd0000000012, type = VK_OBJECT_TYPE_SHADER_MODULE; Object 1: handle = 0xd175b40000000013, type = VK_OBJECT_TYPE_PIPELINE_LAYOUT; | MessageID = 0x45717876 | vkCreateGraphicsPipelines(): pCreateInfos[0] Set 3 Binding 1 in shader (VK_SHADER_STAGE_FRAGMENT_BIT) uses descriptor slot (expected `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`) but not declared in pipeline layout The Vulkan spec states: layout must be consistent with all shaders specified in pStages (https://vulkan.lunarg.com/doc/view/1.3.243.0/windows/1.3-extensions/vkspec.html#VUID-VkGraphicsPipelineCreateInfo-layout-00756)
validation layer: Validation Error: [ VUID-VkGraphicsPipelineCreateInfo-layout-00756 ] Object 0: handle = 0xdcc8fd0000000012, type = VK_OBJECT_TYPE_SHADER_MODULE; Object 1: handle = 0xd175b40000000013, type = VK_OBJECT_TYPE_PIPELINE_LAYOUT; | MessageID = 0x45717876 | vkCreateGraphicsPipelines(): pCreateInfos[0] Set 3 Binding 2 in shader (VK_SHADER_STAGE_FRAGMENT_BIT) uses descriptor slot (expected `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`) but not declared in pipeline layout The Vulkan spec states: layout must be consistent with all shaders specified in pStages (https://vulkan.lunarg.com/doc/view/1.3.243.0/windows/1.3-extensions/vkspec.html#VUID-VkGraphicsPipelineCreateInfo-layout-00756)

```

