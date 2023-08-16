## Step by step recipe for Temporal Anti-Aliasing

### What is TAA?

Temporal Anti-Aliasing (TAA)是一种抗锯齿（AA）方法，它利用时间过滤方法在运动中提高了AA的稳定性。与其他AA方法不同的是，TAA产生的视觉效果在性能开销较小的情况下优于或等于其他AA方法。

不仅如此，TAA还可以用于其他领域，例如平滑阴影贴图和更平滑的屏幕空间环境遮蔽技术。

### Ingredients

- Modern OpenGL context (preferably core profile) with double buffer rendering
- Geometry buffer including:
  - Color buffer
  - Velocity buffer
  - Depth buffer
- History buffer. An array containing 2 geometry buffers (for storing the previous frame)
  - A bool/flag used for toggling which history buffer is active for a given frame
- Uniform Buffers
  - velocity reprojection
  - TAA settings
  - Jittering settings
- A textured model (preferably a highly aliased model like a tree)	
- The geometry translation, camera projection and view matrices.

### The high-level explanation

#### Initialization

1. Create framebuffers
2. Create history buffer
3. Load shaders
4. Create Halton [2, 3] number values
5. Load model
6. Load textures
7. 3D camera with projection in perspective mode

#### The draw sequences

1. Update the uniform buffersSet 

2. camera projection to perspective mode

3. Jitter pass
   Vertex:

   - Apply jittering.
   - 使用前一帧和当前帧的相机数据创建两组位置数据，将在下一个着色器阶段中使用。

   Fragment:

   - 利用这两组位置数据来确定速度，并将其存储在速度缓冲区中。
   - 如有必要，在具有透明度的纹理可能出现深度排序问题时，应用抖动以避免这些问题。

4. 将相机切换为正交投影，以进行2D绘图。

5. TAA Pass - 使用当前和前一深度以及颜色渲染纹理中的速度信息，用于确定对当前和前一场景进行多少模糊处理。

   Vertex: Pass through quad shader

   Fragment:

   - 在当前帧的深度缓冲区中的3x3内核中获取最接近深度值的UV坐标
   - 使用该UV数据从速度缓冲区中采样，然后将其取负。然后，将该速度UV用于从历史缓冲区的相应位置采样深度。
   - 接下来是解析函数，它将返回最终的图像（内容太长无法在这里展示）。

6. Final Pass 将最终的TAA纹理绘制到后向缓冲区，或者将当前的历史缓冲区复制到后向缓冲区。

7. Prepare to render the next frame

8. Repeat steps 1-7

### TAA in depth

TAA pass

这是TAA的最终阶段，其中使用当前和之前场景的深度、颜色或亮度（光）以及速度来确定何时以及以多大程度将当前场景与之前场景混合在一起。 过度与当前场景混合在一起在运动中不会产生太大的差异，而过于依赖之前的场景将导致幽灵伪影现象（本质上是伪递归渲染）。 我的实现方法简单地获取一个3x3邻域内像素的平均深度，并检查该平均值是否高于一个任意值。如果平均值过低，我们可以猜测该像素处没有内容，或者在其边缘，或者是一个物体，因此我们停止与之前的场景混合，以减少幽灵伪影现象的产生。

TAA resolve functions

有许多解析函数可以用来提升TAA的视觉质量，目前已经存在了一些选项，但对于这个项目，我选择了“Inside TAA”解析方法，主要是因为它的视觉效果，此外，这种解析方法比我找到的其他方法更容易理解。

### Jittering

Jittering是一种通常用于静态场景的抗锯齿方法，它通过在视点空间内微小地移动（抖动）世界几何体，使用一系列均匀分布的噪声值（Halton [2, 3] 序列）来实现。移动的幅度不能超过1像素，否则抖动将从子像素模糊（对所有受影响的几何体产生良好的AA效果）转变为完全模糊。然后，抖动的像素在多帧之间自然混合（TAA通道），以平滑锯齿边缘。

抖动通过一个顶点着色器（jitter.vert）来实现，通过像下面这样操纵投影矩阵来进行静态场景的抗锯齿处理。为了确定速度，使用未抖动的当前场景和前一场景非常重要，因此最好复制投影矩阵并对其进行操作。

```
mat4 jitterMat = mat4(1.0);
mat4 newProj = projection;
if(haltonScale > 0)
{
   jitterMat[3][0] += jitter.x * deltaWidth * haltonScale;
   jitterMat[3][1] += jitter.y * deltaHeight * haltonScale;
}

gl_Position = jitterMat * projection * view * translation * position;
```

在下面的图片中，说明了为什么抖动顶点而不是抖动图像的每个像素更可取。此外，请确保不要在片段着色器中应用抖动，因为这会导致错误的模糊效果（以及模糊整个图像而不是几何体，忽略基于深度的模糊，导致远处的物体比应该更模糊）。

![img](D:\Games\VulkanDemo\doc\taa1.jpg)

### Velocity

我们在抖动的顶点着色器中使用前一帧相机视图和几何体平移矩阵，这样可以得到几何体的当前和前一帧抖动后的位置。这些数据在着色器流程的下一步中使用，以便通过计算当前帧和前一帧中几何体的位置（在屏幕空间中）之间的差异，生成所需的速度信息。

这个速度值随后将被存储在渲染纹理中，以便在应用最终的TAA效果时使用。在TAA通道中使用时，它用于确定从历史深度缓冲区采样的位置。深度值越大（无论是正值还是负值），在TAA的最终混合中，之前的深度被采样的距离就越远。

Calculating velocity – vertex shader

在几何体渲染阶段，从当前帧和前一帧获取位置差异。首先创建两个额外的顶点着色器输出，通过使用当前场景和前一场景的视图和平移矩阵，将当前和前一场景渲染到这些输出中。

```
//these 2 should be in screen space
outBlock.newPos = projection * view * translation * position;
outBlock.prePos = projection * previousView * previousTranslation * position;
```

确保保存前一帧的视图和平移矩阵以在此处使用。此外，请注意，在计算速度时，不要使用可能导致速度不准确的抖动位置。

Calculating velocity – fragment shader

首先，将从顶点着色器输出的数据从屏幕空间（-1到1）移动到UV空间（0到1）。

```
vec2 newPos = ((inBlock.newPos.xy / inBlock.newPos.w) * 0.5 + 0.5);
vec2 prePos = ((inBlock.prePos.xy / inBlock.prePos.w) * 0.5 + 0.5);
```

然后，计算速度为（新位置 - 前一位置），并将其输出到速度渲染纹理中。

注意：速度值可能非常小，这可能需要使用高精度纹理（这可能会占用大量内存）。解决这个问题的一种方法是首先将速度乘以一个大数，然后在下一次读取速度纹理时，将值除以该数以恢复原始值。这样做是为了将这些值保存到一个较低精度的纹理中（0.01 -> 1（较低精度））。

### History Buffers


这些是为了在当前帧和前一帧之间进行混合所必需的，使用当前帧标志（通常为布尔值）来确定要从哪个活动历史记录绘制、绘制到和清除缓冲区。这个标志将在两种状态之间每帧切换，以在每帧中交换缓冲区的功能。

在TAA通道中，所选的历史缓冲区将使用其颜色和深度进行混合，将当前帧和前一帧混合在一起。在渲染之后，记得首先清除前一帧缓冲区，并将当前帧的深度从几何体缓冲区复制到当前历史缓冲区的深度附件中（在翻转当前帧标志之前）。

```
historyFrames[!currentFrame]->Bind(); //clear the previous, the next frame current becomes previous
historyFrames[!currentFrame]->ClearTexture(historyFrames[!currentFrame]->attachments[0], clearColor1);
historyFrames[!currentFrame]->ClearTexture(historyFrames[!currentFrame]->attachments[1], clearColor2);
//copy current depth to previous or vice versa?
historyFrames[currentFrame]->attachments[1]->Copy(geometryBuffer->attachments[2]); //copy depth over
```

### Ghosting


幽灵伪影是由当前帧和前一帧混合不正确造成的，导致累积的数据从一个绘制调用的帧传递到下一个绘制调用的帧，当相机或场景快速移动时，产生了“飘渺”效果。

这种现象发生在最终的着色器通道中，该通道混合了当前帧和前一帧（取决于像素速度），以在运动中平滑图像。然而，有几种方法可以减轻幽灵伪影，最基本的方法是在最终的TAA着色器中进行邻域深度测试（使用当前帧的深度）。

```
float averageDepth = 0;
for(uint iter = 0; iter < kNeighborsCount; iter++)
{
	averageDepth += curNeighborDepths[iter];
}

averageDepth /= kNeighborsCount;

//for dithered edges, detect if the edge has been dithered? 
//use a 3x3 grid to see if anything around it has high enough depth?
if(averageDepth < maxDepthFalloff)
{
	res = taa;
}

else
{
	res = texture2D(currentColorTex, inBlock.uv);;
}

```

### Halton [2, 3] sequence

在进行抖动时，使用一个二维的噪声数组来均匀地移动场景。为了获得这种均匀分布，Halton 2, 3序列是比较理想的，因为这个序列产生的随机数范围均匀地覆盖了广泛的区域，比其他噪声生成序列要平滑得多。

![img](D:\Games\VulkanDemo\doc\taa2.jpg)

![img](D:\Games\VulkanDemo\doc\taa3.jpg)

Creating the sequence:

```
glm::vec2 haltonSequence[6];
float CreateHaltonSequence(unsigned int index, int base)
{
	float f = 1;
	float r = 0;
	int current = index;
	do 
	{
		f = f / base;
		r = r + f * (current % base);
		current = glm::floor(current / base);
	} while (current > 0);

      return r;
}
```

```
for (int iter = 0; iter < 6; iter++)
{
	jitterUniforms.data.haltonSequence[iter] = glm::vec2(CreateHaltonSequence(iter + 1, 2), CreateHaltonSequence(iter + 1, 3));
}
```

另一种方法是将Halton序列样本的数量限制为6的幂次方，这是由于Halton序列的独特属性，任何少于6个样本的数量都能等距地分布。此外，超过此数量的样本可能会导致“环绕”问题（在2D样本中），从而降低采样质量。

### Quincunx shape

一个更简单的替代方法是使用一个五角星形状。

![img](D:\Games\VulkanDemo\doc\taa4.jpg)

### Framebuffer configurations

![](D:\Games\VulkanDemo\doc\Snipaste_2023-08-16_19-06-26.png)

### Uniform buffers

```
struct jitterSettings_t
{
	glm::vec2			haltonSequence[128];
	float				haltonScale;
	int				haltonIndex;
	int				enableDithering;
	float				ditheringScale;

	jitterSettings_t()
	{
		haltonIndex = 16;
		enableDithering = 1;
		haltonScale = 1.0f;
		ditheringScale = 0.0f;
	}
};

struct reprojectSettings_t
{
	glm::mat4		previousProjection;
	glm::mat4		previousView;
	glm::mat4		prevTranslation;

	glm::mat4		currentView;

	reprojectSettings_t()
	{
		this->previousProjection = glm::mat4(1.0f);
		this->previousView = glm::mat4(1.0f);
		this->prevTranslation = glm::mat4(1.0f);
		this->currentView = glm::mat4(1.0f);
	}
};

struct TAASettings_t
{
	//velocity
	float velocityScale;
	float feedbackFactor;
	float maxDepthFalloff;

	TAASettings_t()
	{
		this->feedbackFactor = 0.9f;
		this->maxDepthFalloff = 1.0f;
		this->velocityScale = 1.0f;
	}
};
```

### Other notes

#### Dithering opacity

抖动可以与TAA混合结合使用，以减轻一些透明度和深度相关的问题，还可以通过在静态场景中引入像素在几何体渲染阶段被渲染的概率来辅助抗锯齿。

这个概率是基于像素的alpha值确定的，较高的alpha值更可能被绘制，而较低的alpha值更不可能。例如，如果alpha值为0.5，那么只有一半的像素应该被绘制，如果为0.25，则只有25%的像素应该被绘制，依此类推。

然后，在TAA通道中通过混合当前帧和前一帧来平滑抖动效果，从而减少通常与抖动相关的视觉噪声。

#### Sharpening

如果您的实现让图像看起来有些过于“柔和”，您还可以在TAA通道之后应用锐化。这在某些游戏的TAA实现中是一个常见的抱怨。

#### TXAA = TAA + FXAA

这相当简单，在使用抖动作为抗锯齿手段的同时，还会在抖动/几何体渲染阶段之后（但在TAA通道之前）使用FXAA着色器。这是一种常见的额外抗锯齿方法，因为FXAA在消除线条和网格形状的锯齿边缘方面表现出色，而抖动通常不具备这个优势。

#### TSMAA = TAA + SMAA

这将次像素形态抗锯齿与时域滤波相结合，以生成一个非常清晰的最终图像。

#### Other uses

时域滤波也可以在渲染的其他领域中使用，比如平滑阴影贴图和环境光遮蔽噪声。

### My criticisms

学习如何实现TAA的过程非常困难，因为缺乏详细的逐步教程和易于理解的文档，而且许多已有的文档对于数学基础不强的人来说很难理解。UE4的论文关于抖动的信息模糊不清，这往往会引起混淆，具体来说，抖动应该在剪辑空间中发生，在某些实现中必须与投影矩阵中的深度相乘。

在我第一次的实现中，我使用了glm::mat4来更改这个数学部分为[3][0]，[3][1]。然而，根据上面的图示，添加预期的抖动是为了使抖动与线性深度成比例，通过在剪辑空间（而不是屏幕空间）中进行抖动。

### Performance

CPU and GPU Time (recorded by the [MicroProfiler library](https://github.com/zeux/microprofile))

![img](D:\Games\VulkanDemo\doc\taa5.jpg)

根据上面的数据，每个渲染通道的时间平均为2.19毫秒，用于在GPU上渲染抖动通道、时域AA通道和最终通道（绘制到后向缓冲区）。此外，由于绘制调用数量极少，每个绘制通道的CPU时间微不足道。这意味着将TAA添加到您的项目中可能会使每帧渲染时间额外增加（抖动 = （2 - 1.88 = 0.12）+ TAA = 0.15）~0.27毫秒。

renderDoc gives a similar story:

![img](D:\Games\VulkanDemo\doc\taa6.jpg)

根据这份数据，添加抖动和速度计算大致会在几何体渲染通道中增加约0.3毫秒的时间，而TAA解析通道也可能会增加最多0.18毫秒的渲染时间。 注意，此程序渲染一个单独的树对象，包含3个网格，因此只需要3个绘制调用。