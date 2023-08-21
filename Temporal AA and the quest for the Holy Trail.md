# Temporal AA and the quest for the Holy Trail

很久以前，时域抗锯齿曾是一项新颖的技术，而现在越来越多的文章开始涵盖其动机、实现和解决方案。我将以编程者的身份参与其中，详细介绍这一技术，几乎像是一篇教程，不仅是为了未来的我，也是为了所有对此感兴趣的人。我正在使用Matt Pettineo的MSAAFilter演示来展示不同的阶段。其中的内容大部分来自许多才华横溢的开发者所做的宝贵工作，还有一些来自于我的个人经验。我将介绍一些我自己探索出来的一些技巧，这些技巧在论文或演示中并没有见过。

### **Sources of aliasing**

在计算机图形图像中，走样的起因多种多样。几何（边缘）走样、透明度测试、镜面高光、高频法线、视差贴图、低分辨率效果（SSAO、SSR）、抖动和噪声等都会合谋破坏我们的视觉效果。一些解决方案，如硬件多重采样抗锯齿（MSAA）和屏幕空间边缘检测技术，在某些情况下有效，但在不同情况下可能失败。时域技术通过将计算分布到多个帧中，试图实现超采样，同时解决所有形式的走样。这稳定了图像，但也产生了一些具有挑战性的伪影。

### **Jitter**

TAA的主要原则是在多个帧中计算多个子像素采样，然后将它们组合成一个最终的像素。最简单的方案是在像素内生成随机采样，但有更好的方法可以产生固定的采样序列。关于准随机序列的简短概述可以在这里找到。为了避免聚集，选择一个好的序列很重要，以及在序列内离散的采样数量：通常在4-8个之间效果良好。在实践中，这对于静态图像比动态图像更为重要。以下是带有4个样本的像素示例。

![img](D:\Games\VulkanDemo\doc\TAA-Pixel-Samples-300x300.png)

为了在像素内产生随机子采样，我们将投影矩阵沿视锥体平面的一个像素分数进行平移。合法的抖动偏移范围（相对于像素中心）是屏幕尺寸的反向一半。我们将偏移矩阵（只是一个普通的平移矩阵）与投影矩阵相乘，以获得修改后的投影矩阵，如下所示。

![image-20230818145359447](D:\Games\VulkanDemo\doc\image-20230818145359447.png)

一旦我们有了一组样本，我们使用这个矩阵像平常一样光栅化几何图形，以产生与样本相对应的图像。如果一切顺利，每一帧都会获得一个新的抖动，图像应该会像这样晃动。

![img](D:\Games\VulkanDemo\doc\TAA-Jitter.webp)

### **Resolve**

在我们的TAA之旅中的下一个阶段是解算过程。我们将收集样本并将它们合并在一起。解算过程可以采用两种形式，一种是使用累积缓冲区，另一种是使用多个过去的缓冲区，就像Guerrilla那样。在本文中，我们将坚持使用第一种方法，因为它更常见和稳定。累积缓冲区存储多帧的结果，并且通过将当前抖动帧的一小部分（例如10％）混合在一起，每帧都会更新。对于静态摄像机来说，这应该足够了。图像仍然显示出一些镜面走样，我们稍后会解决这个问题，但它是稳定的（这是一个动态的webp图片）。

![img](D:\Games\VulkanDemo\doc\TAA-Resolve-Simple-1.webp)

So far so good. What happens if we jiggle the camera about a little?![img](D:\Games\VulkanDemo\doc\TAA-Resolve-Simple-Camera-Motion.webp)

### **Ghosting**

我们之所以会得到轨迹，是因为我们在与当前帧相同的位置对上一帧进行采样。结果是图像的叠加，随着我们积累新帧而逐渐消失。由于问题是由相机移动引入的，让我们首先解决这个问题。对于不透明对象来说，**摄像机运动相对较容易修复，因为我们知道它们的世界空间位置可以使用深度缓冲和相机投影的逆矩阵来重构**。有关更多细节，请阅读这里和这里，或查阅演示文件BackgroundVelocity.hlsl。这个过程被称为重投影，涉及以下步骤：

1. 从当前相机C生成的当前深度缓冲区中读取深度。
2. 使用视图投影矩阵的逆矩阵进行反投影，将我们的屏幕空间位置转换为世界空间。
3. 使用先前的视图投影矩阵将其投影到先前相机P的屏幕空间。
4. 将屏幕空间位置转换为UV坐标，并对累积纹理进行采样。

![img](D:\Games\VulkanDemo\doc\TAA-Camera-Reprojection.png)

恰如其分，细节之处常有魔鬼，需要考虑诸多因素，比如位置是否在先前相机外部，如果有动态分辨率则涉及视口变化等等。

```

float2 reprojectedUV = CameraReproject(uv); 
float3 currentColor = CurrentTexture.Sample(uv);
float3 previousColor = PreviousTexture.Sample(reprojectedUV);
float3 output = currentColor * 0.1 + previousColor * 0.9;
```

一种替代方案是将这种重投影偏移存储为纹理中的速度。我们稍后会看到为什么这很有用。在存储速度时，我们需要确保移除抖动偏移，正如我们将在motion vectors部分中看到的那样。

```
float2 velocityUV = CurrentVelocityTexture.Sample(uv);
float2 reprojectedUV = uv + velocityUV;
 
float3 currentColor = CurrentTexture.Sample(uv);
float3 previousColor = PreviousTexture.Sample(reprojectedUV);
float3 output = currentColor * 0.1 + previousColor * 0.9;
```

![img](D:\Games\VulkanDemo\doc\TAA-Resolve-Camera-Reprojection-Linear.webp)

结果有点模糊，但幽灵效应已经消失了。或者真的是这样吗？现在让我们尝试一下平移相机。

![img](D:\Games\VulkanDemo\doc\TAA-Resolve-Trails.webp)

### **Disocclusion**

随着物体相对移动，之前未曾显示的表面可能会出现在视野中，我们称之为去遮挡（disocclusion）。在上面的图像中，将相机向侧面移动会显示出之前被模型遮挡的部分背景。请注意，看起来像是手在移动，因为背景是静态的。这两种移动并不等价，我们稍后会看到。新显示的表面将会被正确地重新投影，但在累积缓冲区中会遇到之前存在的模型的无效信息。解决这个问题有多种方法。

#### Color Clamping

**Color Clamping的假设是当前样本附近的颜色对积累过程是有效贡献**。理论上，从积累缓冲区获取的值如果差异很大，应该被丢弃。然而，与其丢弃这个值并重置积累过程，我们会对其进行调整，使其适应附近的范围，然后使用它。有不同的技术，但三种流行的方法是截断（clamp）、剪切（clip）和方差剪切（variance clipping）。下面的紫色示例是一个3×3邻域。不同技术的实现可以在Playdead的这里找到，以及他们的演示“INSIDE中的时域投影抗锯齿”，以及UE4的高质量时域超采样。

![img](D:\Games\VulkanDemo\doc\TAA-Color-Neighborhood.png)

```
// Arbitrary out of range numbers
float3 minColor = 9999.0, maxColor = -9999.0;
 
// Sample a 3x3 neighborhood to create a box in color space
for(int x = -1; x <= 1; ++x)
{
    for(int y = -1; y <= 1; ++y)
    {
        float3 color = CurrentTexture.Sample(uv + float2(x, y) / textureSize); // Sample neighbor
        minColor = min(minColor, color); // Take min and max
        maxColor = max(maxColor, color);
    }
}
 
// Clamp previous color to min/max bounding box
float3 previousColorClamped = clamp(previousColor, minColor, maxColor);
 
// Blend
float3 output = currentColor * 0.1 + previousColorClamped * 0.9;
```

为了更直观地展示这个算法的工作原理，我在Unity中创建了一个小程序，它采用一些位置（位置的值是颜色），创建有颜色的球体（邻域），从中推导出一个框，并对历史样本进行截断以适应那个框。在2D中更容易看到。您可以看到如何将迥异的颜色近似为类似于原始颜色的内容。

![img](D:\Games\VulkanDemo\doc\TAA-Color-Clamping-Illustration.webp)

在TAA的实现中，任何这种变体都是必不可少的。如果邻域中有很多颜色变化，边界框会变得很大，而轨迹可能会再次变得明显。为此，我们需要额外的信息。以下是clamp的效果。

![img](D:\Games\VulkanDemo\doc\TAA-Color-Clamping.webp)

#### Depth Rejection

Depth Rejection的想法是，我们可以假设具有非常不同深度值的像素属于不同的表面。为此，我们需要存储上一帧的深度缓冲区。这对于第一人称射击游戏可能效果良好，其中枪支和环境之间的距离非常远。然而，这不是一个通用的启发式方法，在多种情况下可能会出现问题，例如树叶或具有大量深度复杂性的噪声几何体。有关用例，请参阅：

#### Stencil Rejection

模板拒绝是一种定制的解决方案，对于一组有限的内容可能效果良好。其思想是使用与背景不同的模板值来标记“特殊”对象。这可以是主角、汽车等。为此，我们需要存储上一帧的模板缓冲区。在进行解算时，我们会丢弃任何具有不同模板值的表面。需要特别注意避免出现硬边缘。有关用例，请参阅：

更新：一位在Twitter上的亲切读者提到，可以使用ID缓冲区来实现类似的方案。

#### Velocity Rejection

基于速度拒绝表面在我看来更加稳健，因为根据定义，去遮挡是由两个物体相对于相机的相对运动差异引起的。如果两个表面在两帧之间的速度差异很大，那么要么是加速度很大，要么是物体以不同的速度运动，并且其中一个突然变得可见。为此，我们需要存储上一帧的速度缓冲区。过程如下：

1. 读取当前速度 
2. 使用速度确定先前的位置 
3. 将位置转换为UV 
4. 读取先前的速度 
5. 使用速度相似度指标确定它们是否属于不同的表面

在Twitter上的讨论提到了两种方法：两个速度的点积和速度幅度的差异。两者都存在问题。

- 点积在其中一个向量为0时具有不连续性，并且将对立向量视为非常不同，即使它们的大小很小 
- 幅度差异将相同大小的对立向量视为相同

我提出的方法是使用两个向量之间差异的长度，顺便说一下，这就是每帧的加速度，作为相似度指标。较大的加速度意味着去遮挡，我们可以创建一个平滑的坡道，从无遮挡到完全遮挡。以下是一些图示，展示了我的意思。

![img](D:\Games\VulkanDemo\doc\TAA-Velocity-Rejection-1.png)

![img](D:\Games\VulkanDemo\doc\TAA-Velocity-Rejection-2-300x257.png)

一旦我们有了相似度指标，我们就可以对其作出反应。在这种情况下，我们将向稍微模糊的屏幕版本进行插值，以避免已汇聚部分和新部分之间出现明显差异。另一种选择是修改汇聚因子。

```

// Assume we store UV offsets
float2 currentVelocityUV = CurrentVelocityTexture.Sample(uv);
 
// Read previous velocity
float2 previousVelocityUV = PreviousVelocityTexture.Sample(uv + currentVelocityUV);
 
// Compute length between vectors
float velocityLength = length(previousVelocityUV - currentVelocityUV);
 
// Adjust value
float velocityDisocclusion = saturate((velocityLength - 0.001) * 10.0);
 
// Calculate base accumulated quantity
float3 accumulation = currentFrame * 0.1 + previousFrameClamped * 0.9;
 
// Lerp towards a backup value - could be a blurred version derived from the neighborhood
float3 output = lerp(accumulation, currentFrameBlurred, velocityDisocclusion);
```

#### Alternative Hacks

另一种简单的使用速度的方法是根据物体的运动速度来权衡其贡献，因为它们通常较难看到，或者实际上受到了运动模糊等影响。追逐关卡或赛车游戏是很好的例子。

### **Motion Vectors**

到目前为止，我们已经在相机移动时改善了静态场景，但是当物体本身移动时呢？我们将进行比较，分别是没有和有颜色截断（从左到右）。

![img](D:\Games\VulkanDemo\doc\TAA-Moving-Object-No-Velocity-No-Clamping.webp)

![img](D:\Games\VulkanDemo\doc\TAA-Moving-Object-No-Velocity-With-Clamping.webp)

就像之前一样，出现了模糊，但现在甚至内部像素也受到影响。颜色截断（右侧）尽力修复颜色，但仍然是一团混乱。有趣的是，这在发布的视频游戏中可能是常见的效果。下面的图像是在UE4游戏中捕获的，其中植被缺乏motion vectors。

![img](D:\Games\VulkanDemo\doc\TAA-Partisans-Smearing.png)

这是因为仅从相机派生运动不足以应对，我们还需要考虑物体的运动。通常的解决方法是顶点着色器计算位置两次，一次用于当前帧，一次用于先前帧。它将这些传递给像素着色器，后者计算差异并将其输出到速度纹理中。对于不移动或变形的静态几何体，我们仍然可以继续只使用相机运动。

```
// Vertex Shader
{
    // Compute positions in NDC space
    VS_OUT vsOut;
    vsOut.previousPosition = ComputePreviousPosition();
    vsOut.currentPosition = ComputeCurrentPosition();
}
 
// Pixel Shader
{
    PS_OUT psOut;
    float2 previousPosition = vsIn.previousPosition.xy / vsIn.previousPosition.w; // Perspective divide
    float2 currentPosition = vsIn.currentPosition.xy / vsIn.currentPosition.w;
    
    // The difference in positions is the velocity
    float2 velocity = previousPosition - currentPosition;
    psOut.velocity = velocity * float2(0.5, -0.5) + 0.5; // Put in UV space
 
    // Remember to remove the jitters in the space you've uploaded them in (we assume UV space)
    // We are making it very explicit that there are two jitters here, in practice you can combine them CPU-side
    psOut.velocity -= currentJitter.xy;
    psOut.velocity -= previousJitter.xy;
}
```

现在生成的图像效果正常。左侧没有颜色截断，右侧有。

![img](D:\Games\VulkanDemo\doc\TAA-Moving-Object-With-Velocity-No-Clamping.webp)

![img](D:\Games\VulkanDemo\doc\TAA-Moving-Object-With-Velocity-With-Clamping.webp)

速度通常是一个2通道的16位浮点纹理，但它可能会有所不同。计算位置两次的替代方法包括为每个顶点保留一个缓冲区，其中包含先前的位置。这将占用更多内存，在最简单的情况下每个顶点32位，因此只建议在位置计算非常昂贵的情况下使用。

### **Flicker**

添加颜色截断的一个后果是可能会在静态图像中引入闪烁。由于走样，高强度的子像素可能会在交替的帧中出现和消失。颜色邻域要么截断，要么让它们通过。实质上，积累过程不断被重置，这会导致闪烁。修复这个问题的典型方法是对图像进行色调映射，试图降低亮度异常值的重要性，使图像变得更加稳定。我见过几种不同的技术。

![img](D:\Games\VulkanDemo\doc\TAA-Flicker-With-Clamp.webp)

#### Blend Factor Attenuation

在某些情况下，这会修改混合因子。UE4https://de45xmedrsdbp.cloudfront.net/Resources/files/TemporalAA_small-59732822.pdf#page=45提到他们会检测何时会发生截断事件，并减小混合因子。然而，这会重新引入抖动，必须小心处理。

#### Intensity/Color Weighing

由于闪烁的原因是连续邻域中方差较大，强度加权试图减弱强度较高的像素。这在稳定图像的同时会减弱镜面高光（它们会变得更暗，所以对于像闪烁的沙子这样的情况，您可以增加强度或在TAA后添加它）。演示中附带了亮度加权，我过去使用过对数加权，但它们相似。对数加权在进行任何线性操作之前将颜色转换为对数空间（要小心NaN！），这会偏向低强度值。以下是一个简短的比较和伪代码。

```
// Store weight in w component
float4 AdjustHDRColor(float3 color)
{
    if(InverseLuminance)
    {
        float luminance = dot(color, float3(0.299, 0.587, 0.114));
        float luminanceWeight = 1.0 / (1.0 + luminance);
        return float4(floatcolor, 1.0) * luminanceWeight;
    }
    else if(Log)
    {
        return float4(x > 0.0 ? log(x) : -10.0, 1.0); // Guard against nan
    }
}
 
// Read neighborhood and adjust using weights
float3 neighborhoodPixel0 = AdjustHDRColor(ReadNeighborhood)
 
// [...]
 
// Read previous color
float4 previousColor = AdjustHDRColor(Reproject(uv));
 
// Read current color
float4 currentColor = AdjustHDRColor(CurrentTexture.Sample(uv));
 
// Do color clamping
float3 previousColorClamped = clamp(previousColor, minColor, maxColor);
 
// Blend
 
float currentWeight = 0.1 * currentColor.a;
float previousWeight = 0.9 * previousColor.a;
 
float3 output = (currentColor.rgb * currentWeight + previousColorClamped.rgb * previousWeight);
 
output /= (currentWeight + previousWeight); // Normalize back. Note that this has no effect in the log case
 
if(Log)
{
    output = exp(output); // Undo log transformation
}
```

### **Blurring**

对Temporal AA的常见批评是它看起来模糊。当我刚开始学习这个主题时，我对这个问题从未完全理解过。在静态图像上，我们可以获得清晰的结果，但由于重构误差，在运动中会出现模糊。为了理解其中的原因，让我们来看一下以下的重投影图像。

![img](D:\Games\VulkanDemo\doc\TAA-Reprojection-Reconstruction.png)

当前像素被重新投影到先前的帧，在那里它很可能不会落在像素中心，而是在4个样本之间的某个位置。在先前帧中，没有一个确切的值对应于我们的位置，所以我们应该取哪个？这是一个重构问题。选择任何一个样本都会产生类似线状的捕捉伪影。另一个选项是对最近的4个样本进行双线性滤波，这实际上是一种模糊。由于有一个累积缓冲区，重构误差会累积，导致进一步的模糊。另一个选项是进行高阶滤波。虽然有几种方法，但最流行的是Catmull-Rom滤波器，在B = 0且C = 0.5时计算如下，作为广义双三次滤波。

这个双三次滤波器有负的叶片（即引入了高通成分），从而产生更锐利的图像。移动滑块C以改变“锐度”。标准的Catmull-Rom需要16次纹理读取，通过利用双线性滤波可以优化为9个样本。这在UE4中使用。Jorge Jiménez通过将角落样本减少到5次读取来进一步优化了Call of Duty的实现。以下是在手臂朝相机移动时双线性和Catmull-Rom之间的比较。

- 进一步增加图像的清晰度的一种额外方法是在TAA之后应用锐化处理。一些算法，如AMD的FidelityFX，在放大过程中已经执行了这个操作。
- 一个有趣但可能更复杂的方法在“Hybrid Reconstruction Anti Aliasing”https://michaldrobot.files.wordpress.com/2014/08/hraa.pptx中提出。它估计了重构引入的误差，并试图对其进行补偿。

#### **Texture Blurring**

纹理模糊是TAA的另一个批评。纹理在Mipmap过程中已经被模糊处理，而运行时调整为选择适当的Mipmap级别，以最小化走样，同时保持细节清晰。屏幕空间中的抖动会在纹理空间中引起进一步的模糊。据我所知，有两种方法可以直接应对这个问题：

1. 引入负的Mipmap偏差。这会强制GPU采样更详细的Mipmap级别。需要注意的是，不能重新引入我们费力去除的走样，还需要测量现在以更高的Mipmap级别采样的性能影响，但它可以很好地恢复细节。
2. 去掉纹理的UV抖动。目的是保持UV与没有屏幕空间抖动时的相同。我要感谢Martin Sobek https://twitter.com/MartinSobek13，他向我介绍了这个很酷（而且成本低廉！）的技巧。在实际中，我们通过导数来将像素抖动（屏幕空间中的增量）表示为纹理坐标的增量（UV空间中的增量）：
3. ![image-20230818160350951](D:\Games\VulkanDemo\doc\image-20230818160350951.png)

```

float2 UnjitterTextureUV(float2 uv, float2 currentJitterInPixels)
{
    // Note: We negate the y because UV and screen space run in opposite directions
    return uv - ddx_fine(uv) * currentJitterInPixels.x + ddy_fine(uv) * currentJitterInPixels.y;
}
```

### **Edges**

**在重新投影当前像素时，我们需要意识到速度纹理与历史颜色缓冲区不同，存在走样**。如果不小心，我们可能会间接地重新引入边缘走样。为了更好地处理边缘，一个典型的解决方案是扩张走样信息。我们将使用速度作为示例，但您也可以用深度和模板来做。我知道有几种方法可以实现这一点：

1. **Depth Dilation**：取与邻域中最近深度像素对应的速度。
2. **Magnitude Dilation**：在邻域中取速度幅度最大的速度。

```
float2 GetVelocity(float uv)
{
    if(DilationMode == None)
    {
        return CurrentVelocityTexture.Sample(uv);
    }
    else if(DilationMode == ClosestDepth)
    {
        float closestDepth = 100.0;
        float2 closestUVOffset = 0.0;
        for(int j = -1; j <= 1; ++j)
        {
            for(int i = -1; i <= 1; ++i)
            {
                 float2 uvOffset = float2(i, j) / TextureSize;
                 float neighborDepth = Depth.Sample(uv + uvOffset);
                 if(neighborDepth < closestDepth)
                 {
                     closestUVOffset = uvOffset;
                     closestDepth = neighborDepth;
                 }
            }
        }
        return CurrentVelocityTexture.Sample(uv + closestUVOffset);
    }
    else
    {
        // Similar to closest depth. Read multiple velocities and compare magnitudes
    }
}
```

这两种技术本质上都会“膨胀”边缘，以减少边缘走样。

### **Transparency**

透明度是一个棘手的问题，因为透明对象通常不会渲染到深度缓冲区。低分辨率的烟雾和尘土等效果通常不受Temporal AA的影响，而颜色截断可以在不引起太多问题的情况下完成其工作。然而，更一般的透明度，如玻璃、全息图等，可能会受到影响，如果没有正确地重新投影，效果可能会不佳。对于这个问题有一些解决方案，但取决于内容：

1. 将混合的motion vectors写入速度缓冲区。这取决于内容，但它可能会起作用。实际上，即使将motion vectors写入速度缓冲区时，就好像它们是实体的一样，如果对象的不透明度足够高，也可以起作用。

2. 引入每像素累积因子：这就是UE4所称的“响应式AA”。基本上，它会在鬼影和像素抖动之间做权衡。对于非常详细的VFX非常有用，如这里

   https://de45xmedrsdbp.cloudfront.net/Resources/files/TemporalAA_small-59732822.pdf#page=38

   所示。

3. 在TAA之后渲染透明度。这不推荐，除非您将它们渲染到离屏缓冲区，使用边缘检测解决方案（如FXAA或SMAA）进行抗锯齿处理，然后进行组合。它在边缘可能会出现抖动，因为它与抖动的深度缓冲区进行比较。

### **Camera Cuts**

在使用TAA时，相机切换会带来挑战。相机切换会强制我们使历史缓冲区失效，因为其内容不再代表当前渲染的帧。因此，我们不能依赖历史记录来生成一个好的抗锯齿图像。以下是一些解决此问题的方法：

1. 对收敛进行偏移以加速过程。在相机切换后，我们需要尽快积累内容。
2. 使用淡出和淡入效果。TAA将在黑色部分进行累积，并在淡入时收敛。
3. 在前几帧应用另一种形式的抗锯齿或模糊效果，以减少收敛的不连贯性。如果您已经掌握了该技术，这种方法会更简单。

总之，归根结底，它们都是一些技巧，但是需要加以处理。另一件需要记住的事情是，帧率越高，这个问题就越不明显。

### **Epilogue**

我希望您喜欢这篇关于TAA的详细介绍。我肯定还遗漏了很多内容，但希望这是一个很好的起点。如果您有任何问题或建议，请让我知道，我希望您今天学到了一些东西。

### **Additional Bibliography**

大部分相关的链接都已经在相应的位置提供了，但以下是一些额外的链接。它们要么涵盖了广泛的内容，要么具有历史意义。

http://behindthepixels.io/assets/files/TemporalAA.pdf

https://gfx.cs.princeton.edu/pubs/Nehab_2007_ARS/NehEtAl07.pdf

https://cdn2.unrealengine.com/Resources/files/GDC09_Smedberg_RenderingTechniques-1415210295.pdf