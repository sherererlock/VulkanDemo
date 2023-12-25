# 完整的Vulkan流程

### VkInstance

- **VkApplicationInfo**

- **extensions**：VK_KHR_Surface、VK_KHR_win32_surface、VK_EXT_debug_utils

- **validation layers**：VK_LAYER_KHRONOS_validation

  检查命令，参数等等是否合法，是否线程安全等等

### VkPhysicalDevice

物理设备，显卡(GPU)

- **VkPhysicalDeviceProperties** 

  显卡设备的属性，如name，type，支持的vulkan版本

- **VkPhysicalDeviceFeatures** 

  纹理压缩，浮点数支持，MRT的支持的渲染特性，对GeometryShader的支持

- **队列族**（QueueFamily）

  命令有不同的类型，会提交给不同的队列，而队列的分类就是队列族

### VkDevice

和物理设备交互的接口。

- 支持队列类型，创建后返回VkQueue
- 设备Feature（各向异性， cubeMap）
- 支持的扩展（SWAPCHAIN或只支持计算）
- 是否支持校验层

### VkSurfaceKHR

- window的句柄
- VkInstance的句柄

### VkSwapchainKHR

一个包含了若干等待呈现的图形的队列，从交换链中读取一张图形，进行渲染，然后塞进队列让屏幕呈现，创建Swapchain之后就可以从中获取用于渲染的VkImage

- surface format
  - format
  - colorspace
- present mode
  - 即时
  - 先进先出
  - Mailbox 三重缓冲
- 分辨率

### VkImageView

描述了访问VkImage的方式，以及可访问图形的哪一部分

### 图形管线

图 形管线是一系列将我们提交的顶点和纹理转换为渲染目标上的像素的操作,一个DC?

IA-VS-TS-GS-Raster-FS-ColorBlending

**以下是准备创建VkPipeline的所需要的资源**

### Shader Module

#### 需要先将glsl或者hlsl文件编译为SPIR-V字节码文件

#### VkShaderModule

#### VkPipelineShaderStageCreateInfo

- VkShaderModule
- shader的类型（VS/PS）
- main函数的名称

### 固定功能

#### VkPipelineVertexInputStateCreateInfo

描述vertex buffer中的结构，比如一个顶点占用多长的buffer，每个顶点的buffer内的结构

- VkVertexInputBindingDescription
  - binding ?
  - stride 每个顶点数据量的大小
- VkVertexInputAttributeDescription 
  - binding 
  - location
  - format
  - offset

#### VkPipelineInputAssemblyStateCreateInfo

顶点数据定义了哪种类型的几何图元，以及是否启用几何图元重启

- VkPrimitiveTopology

  描述图元的类型

  Point, Line, Triangle

  List, Stripe(重用顶点)

- 是否重启

#### VkPipelineViewportStateCreateInfo 

描述Viewport和Scirror

- VkViewport
- Scissors

注意数据类型是struct，而创建时使用的是指针

#### VkPipelineRasterizationStateCreateInfo

配置光栅化参数

- depthClampEnable 超出远近平面的物体是否会被截断到远近平面上或者直接丢弃
- rasterizerDiscardEnable 设置为true表示所有几何图元都不能通过光栅化阶段
- polygonMode 以填充的方式还是以wireframe的方式渲染多边形
- cullMode 剔除类型，是否背面剔除
- frontFace 根据三角形的环绕方式是顺时针还是逆时针来决定frontFace
- 一些depthbias的设置，用来配合shadowmap

#### VkPipelineMultisampleStateCreateInfo

MSAA的配置参数，用来进行抗锯齿处理

- rasterizationSamples  每个像素几个采样

#### VkPipelineColorBlendStateCreateInfo

如何进行颜色混合

- VkPipelineColorBlendAttachmentState
  - blendEnable
  - srcColorBlendFactor  pixel shader返回的颜色占用的比例
  - dstColorBlendFactor  帧缓冲中的颜色占的比例
  - VkBlendOp 对二者进行的算术操作
- attachmentCount
- pAttachments

#### VkPipelineDynamicStateCreateInfo

不重建管线的情况下进行动态修改的状态。视口大小，线宽和混合常量

#### VkPipelineDepthStencilStateCreateInfo

深度和模板测试

- depthTestEnable
- depthWriteEnable
- depthCompareOp
- stencilTestEnable
- front
- back

#### VkPipelineLayout

用来传递Shader用到的Uniform变量的数据结构模板，包含一组`VkDescriptorSetLayout`,和 `VkPushConstantRange`

#### VkDescriptorSetLayout

用来定义Uniform数据的模板，包含一组`VkDescriptorSetLayoutBinding`

##### VkDescriptorSetLayoutBinding

描述uniform变量的类型，数量，绑定在哪，以及在ps还是vs中使用到了

- binding 当前set中的位置
- descriptorCount 数量
- descriptorType 是buffer还是image sampler
- stageFlags 在vs或者ps中用到了

#### VkBuffer/VkDeviceMemory

vulkan中的buffer对象，创建buffer和DeviceMemory绑定，

buffer对象描述了内存的大小和用途，VkDeviceMemory提供了访问显存的途径。更新时，将内存map到显存

#### VkImage

贴图或者RT都用VkImage对象

贴图加载进来会存在VkBuffer中，后续用map映射到显存中

##### VkImageCreateInfo

- width,height
- imageType  2D/3D
- mipLevels 
- arrayLayers  cubeMap有6层
- format
- tiling 纹素的排列方式
- initialLayout  
- usage  图片会被如何使用，贴图加载时需要有trans_dst标志位，采样时有sample标志
- sharingMode
- samples 只对多重采样有效

贴图资源被创建时，也需要vulkan的deviceMemory对象作为显存的接口，需要绑定Image对象和memory对象

##### 布局变换

将内存中的图片数据拷贝到显存中时，image的layout需要符合一定要求

使用VkImageMemoryBarrier对图像布局进行变换，这个对象主要用来同步操作，确保图像被读取时已经被完全写入了

###### VkImageMemoryBarrier

变换后就可以使用VkBufferImageCopy对象和vkCmdCopyBufferToImage函数拷贝数据到image显存

拷贝之后需要将图像的布局改变为shader_read

##### VkSampler

shader中通过采样器访问纹理数据

纹理采样过密：每个纹素占据了多个像素（fragment）

纹理采样过疏：多个纹素被映射到一个像素（fragment）上

###### VkSamplerCreateInfo

- magFilter        纹理被放大时的采样方式， linear或nearest

- minFilter          纹理被缩小时的采样方式，

- anisotropyEnable  开启各向异性过滤

- addressModeU，addressModeV，addressModeW， borderColor

  寻址模式，uv的范围超出1时，该如何采样，有重复纹理，重复镜像后的纹理，以及使用最近的纹理，返回设置的borderColor

##### Mipmap





#### VkDescriptorSet

以`VkDescriptorSetLayout`为模板创建出来的对象，包含渲染过程中真正使用的数据

#### VkDescriptorPool

`VkDescriptorSet`需要通过池子来分配和管理，这个池子定义了

`VkDescriptorPoolSize`描述了各种类型的`VkDescriptorSet`的最大个数

- type 类型
- descriptorCount 最大个数

#### VkWriteDescriptorSet

`vkUpdateDescriptorSets`会更新一组`VkDescriptorSet`到显存，`VkWriteDescriptorSet`描述了每个VkDescriptorSet更新的内容

- dstSet  需要更新的VkDescriptorSet对象
- dstBinding 在shader中的binding位置
- descriptorType 类型
- descriptorCount 数量
- pBufferInfo 真正的buffer对象
- pImageInfo 真正的sampler对象

#### VkPushConstantRange

uniform变量，可以像uniform块那样使用，但是并不需要存储在内存里，它由Vulkan自身持有和更新。**这些常量的新值可以被直接从命令缓冲区推送到管线**。`vkCmdPushConstants`

- stageFlags 在vs还是ps中用到了
- offset 数据的偏移
- size 数据的大小

#### VkPipeline

表示一个drawCall，里面包含了一个draw所需的所有数据和状态

### VkRenderPass

表示一个Pass，比如shadowPass,BasePass, 其特点是更换不同的pass要设置不同的rt

指定颜色和深度缓冲，采样数，渲染操作如何处理缓冲的内容等

#### VkAttachmentDescription

描述一张RT的属性，可以color RT, ResolveRT或depthRT

- format 对于color可能rgba，对于depth是depth
- samples msaa用到的采样数
- loadOp 加载这张Color RT时的操作
- storeOp 存储这张Color RT时的操作
- stencilLoadOp    加载这张depthRT时的操作
- stencilStoreOp   存储这张depthRT时的操作
- initialLayout  图形布局
- finalLayout

#### SubPass

RenderPass至少有一个子流程，子流程将上一个流程的输出RT作为输入RT来进行操作，每个子流程可以有多个附着，通过`VkAttachmentReference`引用附着

##### VkAttachmentReference

- attachment    Attachment在`VkAttachmentDescription`数组中位置
- layout  Attachment的final layout

##### VkSubpassDescription

对subpass的描述，描述其colorAttachment和DepthStencilAttachment

#### VkRenderPassCreateInfo

- pAttachments  `VkAttachmentDescription`的数组
- pSubpasses  subpass描述的数组，表示有几个subpass
- pDependencies  描述每个subpass是如何依赖上一个subpass的

pAttachments中的顺序最终要与接下里介绍的frameBuffer中pAttachments一致

### VkFramebuffer

描述了真正用于渲染的RT的属性，包括color, depth等等，有几个swapChainImage就需要几个`VkFramebuffer`

#### VkFramebufferCreateInfo

- renderPass   绑定的pass
- pAttachments   一组VkImageView对象
- width     RT的宽度
- height  RT的高度
- layers  有几层RT

### VkCommandPool

Vulkan下的指令，比如绘制指令和内存传输指令并不是直接通过函数 调用执行的。我们需要将所有要执行的操作记录在一个指令缓冲对象，然 后提交给可以执行这些操作的队列才能执行。

指令池对象 用于管理指令缓冲对象使用的内存，并负责指令缓冲对象的分配

#### VkCommandBuffer

指令缓冲对象，存储指令的buffer。每个swapChain中的RT必须要有不同的指令缓冲

##### VkCommandBufferAllocateInfo

- commandPool
- level 
- commandBufferCount

#### 记录指令

vkBeginCommandBuffer

vkEndCommandBuffer

### 开始渲染流程

```c++
vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = shadowPass;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent.width = width;
    renderPassInfo.renderArea.extent.height = height;

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

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdSetDepthBias(
        commandBuffer,
		depthBiasConstant,
		0.0f,
		depthBiasSlope);

    renderPassInfo.framebuffer = frameBuffer;
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(primitiveInfo), &info);
	vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
	
vkEndCommandBuffer(commandBuffers[i]) 
```

