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

用来传递Shader用到的Uniform变量的数据结构