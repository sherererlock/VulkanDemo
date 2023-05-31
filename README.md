# VulkanDemo

## uniform

#### uniform种类

- buffer中的uniform变量
- image

### 流程

#### 在CPU端创建对应的资源

##### uniformbuffer

1. 准备buffer在CPU端的数据
2. 创建buffer
   - usage
   - size
3. 给buffer分配内存资源
4. 拷贝CPU端的数据到分配的Buffer中
5. 绑定buffer和memory
6. 创建VkDescriptorBufferInfo并且将buffer绑定到info上

##### Image

1. 从Image加载数据
2. 创建cmdBuffer用来之后执行copy操作
3. 创建stagingBuffer和stagingMemory
4. 将image数据拷贝到stagingMemory中
5. 绑定stagingBuffer和stagingMemory
6. 创建VkBufferImageCopy来存储mipmap信息
7. 创建image对象
8. 为image对象分配内存
9. 将image和memory对象绑定起来
10. 转换image的layout为VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
11. 拷贝数据到image中
12. 转换image的layout为VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
13. 创建VkSampler对象
14. 创建VkImageView对象
15. 创建VkDescriptorImageInfo对象，并将VkSampler和VkImageView对象绑定到info中

#### 创建Descriptor

1. 创建DescriptorPoolSize

   - type:uniform和sampler
   - count

2. 根据DescriptorPoolSize创建DescriptorPool

3. 创建layout binding

   - type:uniform和sampler
   - stages
   - bindings

4. 根据layout binding创建VkDescriptorSetLayout

5. 根据VkDescriptorSetLayout在DescriptorPool中分配DescriptorSet

6. 创建VkWriteDescriptorSet更新DescriptorSet

   - type:uniform和sampler
   - binding
   - bufferInfo 或者ImageInfo
   - descriptorCount

7. vkUpdateDescriptorSets

8. 创建pipelineLayout（若干个VkDescriptorSetLayout）

   ```
   [ VUID-VkDescriptorSetLayoutCreateInfo-binding-00279 ] Object 0: handle = 0x220f8caaea0, type = VK_OBJECT_TYPE_DEVICE; | MessageID = 0xf6f49662 | vkCreateDescriptorSetLayout(): pBindings[4] has duplicated binding number (2). The Vulkan spec states: The VkDescriptorSetLayoutBinding::binding members of the elements of the pBindings array must each have different values (https://vulkan.lunarg.com/doc/view/1.3.243.0/windows/1.3-extensions/vkspec.html#VUID-VkDescriptorSetLayoutCreateInfo-binding-00279)
   
   validation layer: Validation Error: [ VUID-VkDescriptorPoolSize-descriptorCount-00302 ] Object 0: handle = 0x1f1f52579f0, type = VK_OBJECT_TYPE_DEVICE; | MessageID = 0x4dab60fe | vkCreateDescriptorPool(): pCreateInfo->pPoolSizes[1].descriptorCount is not greater than 0. The Vulkan spec states: descriptorCount must be greater than 0 (https://vulkan.lunarg.com/doc/view/1.3.243.0/windows/1.3-extensions/vkspec.html#VUID-VkDescriptorPoolSize-descriptorCount-00302)
   
   validation layer: Validation Error: [ VUID-VkGraphicsPipelineCreateInfo-layout-00756 ] Object 0: handle = 0x967dd1000000000e, type = VK_OBJECT_TYPE_SHADER_MODULE; Object 1: handle = 0xe7e6d0000000000f, type = VK_OBJECT_TYPE_PIPELINE_LAYOUT; | MessageID = 0x45717876 | vkCreateGraphicsPipelines(): pCreateInfos[0] Set 0 Binding 0 in shader (VK_SHADER_STAGE_FRAGMENT_BIT) uses descriptor slot but descriptor not accessible from stage VK_SHADER_STAGE_FRAGMENT_BIT The Vulkan spec states: layout must be consistent with all shaders specified in pStages (https://vulkan.lunarg.com/doc/view/1.3.243.0/windows/1.3-extensions/vkspec.html#VUID-VkGraphicsPipelineCreateInfo-layout-00756)
   
   validation layer: Validation Error: [ VUID-vkCmdBindDescriptorSets-pDescriptorSets-06563 ] Object 0: VK_NULL_HANDLE, type = VK_OBJECT_TYPE_DESCRIPTOR_SET; | MessageID = 0x747d089 | vkCmdBindDescriptorSets(): Attempt to bind pDescriptorSets[0] (VkDescriptorSet 0x0[]) that does not exist, and VK_EXT_graphics_pipeline_library is not enabled. The Vulkan spec states: Each element of pDescriptorSets must be a valid VkDescriptorSet (https://vulkan.lunarg.com/doc/view/1.3.243.0/windows/1.3-extensions/vkspec.html#VUID-vkCmdBindDescriptorSets-pDescriptorSets-06563)
   
   validation layer: Validation Error: [ VUID-vkCmdBindDescriptorSets-pDescriptorSets-06563 ] Object 0: VK_NULL_HANDLE, type = VK_OBJECT_TYPE_DESCRIPTOR_SET; | MessageID = 0x747d089 | vkCmdBindDescriptorSets(): Attempt to bind pDescriptorSets[0] (VkDescriptorSet 0x0[]) that does not exist, and VK_EXT_graphics_pipeline_library is not enabled. The Vulkan spec states: Each element of pDescriptorSets must be a valid VkDescriptorSet (https://vulkan.lunarg.com/doc/view/1.3.243.0/windows/1.3-extensions/vkspec.html#VUID-vkCmdBindDescriptorSets-pDescriptorSets-06563)
   
   validation layer: Validation Error: [ VUID-VkWriteDescriptorSet-descriptorType-04150 ] Object 0: handle = 0x2d93ac000000006f, type = VK_OBJECT_TYPE_DESCRIPTOR_SET; | MessageID = 0xf72a82b8 | vkUpdateDescriptorSets() pDescriptorWrites[2] failed write update validation for VkDescriptorSet 0x2d93ac000000006f[] with error: Write update to VkDescriptorSet 0x2d93ac000000006f[] allocated with VkDescriptorSetLayout 0xec4bec000000000b[] binding #2 failed with error message: Attempted write update to combined image sampler descriptor failed due to: Descriptor update with descriptorType VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER is being updated with invalid imageLayout VK_IMAGE_LAYOUT_UNDEFINED for image VkImage 0xc4f3070000000021[] in imageView VkImageView 0xb991fa0000000024[]. Allowed layouts are: VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL. The Vulkan spec states: If descriptorType is VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER the imageLayout member of each element of pImageInfo must be a member of the list given in Combined Image Sampler (https://vulkan.lunarg.com/doc/view/1.3.243.0/windows/1.3-extensions/vkspec.html#VUID-VkWriteDescriptorSet-descriptorType-04150)
   
   validation layer: Validation Error: [ VUID-vkUnmapMemory-memory-00689 ] Object 0: handle = 0xcfcda0000000001e, type = VK_OBJECT_TYPE_DEVICE_MEMORY; | MessageID = 0x36bc763c | Unmapping Memory without memory being mapped: VkDeviceMemory 0xcfcda0000000001e[]. The Vulkan spec states: memory must be currently host mapped (https://vulkan.lunarg.com/doc/view/1.3.243.0/windows/1.3-extensions/vkspec.html#VUID-vkUnmapMemory-memory-00689)
   ```
   
   

