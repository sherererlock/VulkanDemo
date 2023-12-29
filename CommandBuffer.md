# CommandBuffer

Vulkan通过Command Buffer完成Record Command的操作。这些Commnad之后会通过Command Buffer被提交到Queue再被提交到GPU中执行。

命令包括将Pipeline和DescriptorSet绑定到Command Buffer的命令，修改Dynamic State、以及DrawCall相关的Command(用于图形渲染),Dispatch的命令(用于计算)， 复制Buffer和Image的Command以及其他VkCmdXXXX所有的调用

**在Command Buffer中所有的Command是无序执行并且整批提交到Queue中的Command Buffer也是无序执行。不同的Queue提交的Command同样也是无序执行。这也体现了GPU的工作方式都是异步并且并行执行**

**在渲染流程内的关于Framebuffer操作是按顺序完成的**

## Command Pool

Command Buffer是通过Command Pool来分配的，由它来分配Command Buffer从而来实现在多个Command Buffer中摊平资源创建的成本，Command Pool是外部同步的这意味着一个Command Pool不能在多个线程中同时使用。这包括从Pool中分配的任何Command Buffer的使用以及分配、释放和重置Command Pool/Buffer本身的操作

#### VkCommandPoolCreateInfo

- queueFamilyIndex字段指定一个Queue Family
- flags字段是VkCommandPoolCreateFlagBits值，分别如下
  - `VK_COMMAND_POOL_CREATE_TRANSIENT_BIT` 指定从Pool中分配的Command Buffer将是短暂的，这意味着它们将在相对较短的时间内被重置或释放。这个标志可以被用来控制Pool的内存分配。
  - `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`允许从Pool中分配的任何Command Buffer被单独重置到Inital状态，可以通过调用vkResetCommandBuffer或者调用vkBeginCommandBuffer时的隐式重置。如果在一个Pool上没有设置这个flag，那么对于从该Pool中分配的任何Command Buffer都不能调用vkResetCommandBuffer
  - `VK_COMMAND_POOL_CREATE_PROTECTED_BIT` 指定从Pool中分配的Command Buffer是受保护的Command Buffer。

#### vkCreateCommandPool, VkCommandPool

## Command Buffer

一般会为SwapChain中的每一个图像创建一个CommandBuffer来渲染它

#### VkCommandBufferAllocateInfo

- commandPool
- level 字段是一个VkCommandBufferLevel值，指定Command Buffer的级别。
  - `VK_COMMAND_BUFFER_LEVEL_PRIMARY`指定一个Primary Command Buffer。
  - `VK_COMMAND_BUFFER_LEVEL_SECONDARY`指定一个Secondary Command Buffer。
- commandBufferCount

#### vkAllocateCommandBuffers, VkCommandBuffer

## 使用Command Buffer

### Record Command

#### VkCommandBufferBeginInfo

- flags字段是一个VkCommandBufferUsageFlagBits类型用于指定Command Buffer的使用行为。
  - `VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT`指定Command Buffer的每个Record Command只会提交一次，不会去复用其中任何Command，在每次提交之间Command Buffer将被重置并再次进入Recording状态
  - `VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT`指定其中的Secondary Command Buffer被认为是完全在一个RenderPass内。如果这是一个Primary Command Buffer那么这个将会被忽略
  - VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT指定一个Command Buffer在Pending状态时可以重新提交给队列，并记录到多个Primary Command Buffer
- pInitanceInfo是一个指向VkCommandBufferInheritanceInfo结构的指针，如果Command Buffer是一个Secondary Command Buffer则需要使用它。如果这是一个Primary Command Buffer那么这个值将被忽略。后续将会介绍Secondary Command Buffer

#### vkBeginCommandBuffer,vkEndCommandBuffer

### Submit Command

#### Queue

Queue通常代表一个GPU线程，GPU执行的就是提交到Queues中的工作。物理设备中Queue可能不止一个，每一个Queue都被包含在Queue Families中。Queue Families是一个有相同功能的Queue的集合，但是一个物理设备中可以存在多个Queue Families，不同的Queue Families有不同的特性。相同Queue Families中的Queues所有功能都相同并且可以并行运行，但是会有一个优先级的问题，这个需要慎重考虑(优先级较低的Queue可能会等待很长时间都不会处理)。不同的Queue Families主要分为以下几类(通过VkQueueFlagBits来分类)：

- VK_QUEUE_GRAPHICS_BIT代表其中的Queue都支持图形操作。
- VK_QUEUE_COMPUTE_BIT指代表其中的Queue都支持计算操作。
- VK_QUEUE_TRANSFER_BIT指代表其中的Queue都支持传输操作。
- VK_QUEUE_TRANSFER_BIT指代表其中的Queue都支持稀疏内存管理操作。
- VK_QUEUE_VIDEO_DECODE_BIT_KHR指代表其中的Queue都支持视频解码操作。
- VK_QUEUE_VIDEO_ENCODE_BIT_KHR指代表其中的Queue都支持视频编码操作。

#### Queue Submit

##### VkSubmitInfo

- waitSemaphoreCount是在提交Command Buffer之前要等待的Semaphore的数量。
- pWaitSemaphores是一个指向VkSemaphore数组的指针，在这个Command Buffer前要等待该其中的Semaphore。如果提供了Semaphore则定义了一个等待该Semaphore信号操作。
- pWaitDstageMask是一个指向管道阶段数组的指针，在这个数组中每个相应的semaphore等待将发生。
- commandBufferCount是要在批处理中执行的Command Buffer的数量。
- pCommandBuffers是一个指向在批处理中执行的VkCommandBuffer数组的指针。
- signalSemaphoreCount是在pCommandBuffers中指定的命令执行完毕后，要发出信号的semaphores的数量
- pSignalSemaphores是一个指向VkSemaphore数组的指针，当这个Command Buffer完成执行时将对其发出信号。如果提供了Semaphore则定义了一个Semaphore发信号的操作

##### vkQueueSubmit

## Secondary Command Buffer

------

## 渲染指令的记录和提交

```c
for (size_t i = 0; i < commandBuffers.size(); i++) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[i];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;
    
    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);    
    vkCmdEndRenderPass(commandBuffers[i]);
    
    if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}
```

