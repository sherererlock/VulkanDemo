# Screen Space Reflections : Implementation and optimization – Part 2 : HI-Z Tracing Method

## 1. Overview

在之前的文章中，我详细解释了如何使用线性追踪方法来实现屏幕空间反射技术，并展示了如何通过一些技巧来优化该方法。这是那篇文章的链接。

在本文中，我想介绍一种不同的追踪方法，它在保持准确性的同时比线性追踪方法表现出色。这个方法被称为“HI-Z Tracing”方法。

这个方法最早是在书籍《GPU PRO 5》的章节“Hi-Z Screen-Space Cone-Traced Reflections by Yasin Uludag”中介绍的。虽然我很多年前就买了这本书，并且购买时也读了这个章节，但直到最近我才尝试自己实现它。

好的。既然已经有一本书解释了这个技术，那么我写这篇文章有什么意义呢？

首先，这本书并没有完全解释这个技术。它没有提供完整的源代码。虽然主要部分的源代码是有的，但却缺少从主体调用的函数的代码。甚至主体部分的代码也可能有一些错误。而且，这本书没有给出如何实现这些缺失函数的指导，所有的补充工作都留给读者。因此，在本文中，我将详细介绍如何实现这个技术。此外，我将分享我的实现的完整源代码，你可以在这里找到它。

其次，书中介绍的方法有一些限制。首先，它只适用于2的幂次方屏幕分辨率。而大多数游戏中使用的分辨率，比如1920×1080，并不是2的幂次方。因此，如果该技术不能用于非2的幂次方分辨率，那么它就没什么用了。其次，我发现该方法的准确性取决于反射光线的方向。当光线主要远离摄像机时（换句话说，|dir.z| >> |dir.xy|），它的效果很好。然而，当光线主要在水平或垂直方向移动时（换句话说，|dir.xy| >> |dir.z|），它的准确性就会下降。第三，在书中介绍的方法中，反射光线只能远离摄像机。换句话说，当反射朝向摄像机移动时，该方法就无法使用。我找到了可以克服这些限制的解决方案，所以我打算谈谈这个问题。

第三，我觉得这个技术真的很棒。因此，通过撰写这篇文章，我希望将这个技术传播给更多的人，并帮助他们在可能的情况下充分利用它。

## 2. Overview of the HI-Z Tracing Method

Let me first show you what the problem is in the linear tracing that we want to improve using the Hi-Z tracing. Below image depicts how a ray is traced in the linear tracing.

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-17-at-8.17.03-am.png)

为了到达交叉采样点，必须在起始采样点和交叉采样点之间的每个单独采样点处停下来进行深度比较。但是，人们可以注意到，在大多数情况下，光线只是在一个空白的空间中移动，没有任何击中点。那么，是否有办法更快地跳过这些空白空间呢？答案是Hi-Z追踪。

Hi-Z追踪方法创建了一种加速结构，称为Hi-Z缓冲区（或纹理）。该结构本质上是场景深度的四叉树，其中四叉树层次中的每个单元都被设置为上一层中4个单元的最小值（或最大值，取决于z轴方向），如下所示。

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-17-at-8.06.04-am.png)


Hi-Z追踪方法从完整分辨率一直创建级别，直到1×1。它使用纹理的多级别细化（mip levels）来存储每个四叉树级别并访问它们。

下面的图片给出了使用Hi-Z追踪方法进行追踪时的样子，并将其与上面线性追踪图片进行对比。

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-17-at-8.19.17-am-1.png)

正如图片所示，Hi-Z追踪在到达交叉点之前只停在较少的采样点上。这就是这种方法能够优于线性追踪方法的原因。

因此，自然地，该方法被分为两个阶段。在第一阶段，它使用场景深度纹理作为基本级别生成Hi-Z纹理。在第二阶段，它使用前一阶段生成的Hi-Z纹理运行SSR（屏幕空间反射）。

## 3. The Hi-Z pass


在这个阶段，我们正在构建称为Hi-Z纹理的加速结构，它将在后续的SSR阶段中使用。该阶段由多个子阶段组成，每个子阶段生成四叉树的每个级别并存储在Hi-Z纹理的mipmap中。

第一个子阶段通过将场景深度纹理复制到Hi-Z纹理的0级mip中来生成Hi-Z纹理的基本级别。

然后，随后进行多个子阶段，每个子阶段使用前一个mip级别作为输入，使用Hi-Z生成计算内核来生成下一个mip级别，并在生成最后一个1×1大小的mip后结束。下面的图示描述了这些子阶段的工作方式。

![img](https://sugulee.files.wordpress.com/2021/01/screen-shot-2021-01-17-at-6.34.33-pm.png?w=1024)

According to the book, each cell in a level is set to the minimum of 2×2 cells in the above level as below.

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-17-at-8.06.04-am1.png)

```
kernel void kernel_createHiZ(texture2d<float, access::read> depth [[ texture(0) ]],
                           texture2d<float, access::write> outDepth [[ texture(1) ]],
                                  uint2 tid [[thread_position_in_grid]])
{
    uint2 vReadCoord = tid<<1;
    uint2 vWriteCoord = tid;
    
    float4 depth_samples = float4(
                                  depth.read(vReadCoord).x,
                                  depth.read(vReadCoord + uint2(1,0)).x,
                                  depth.read(vReadCoord + uint2(0,1)).x,
                                  depth.read(vReadCoord + uint2(1,1)).x
                                  );
	
    float min_depth = min(depth_samples.x, min(depth_samples.y, min(depth_samples.z, depth_samples.w)));
    
    outDepth.write(float4(min_depth), vWriteCoord);
}
```

然而，这种方法只在上一级的维度是当前级别维度的2的倍数时才起作用，这可能在屏幕分辨率不是2的幂次方时出现。看一下上面的代码是如何在级别0维度为5×5时生成级别1的单元格的。

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-17-at-11.23.09-am.png)

考虑级别0中底部边缘和右边缘的单元格。因此，很明显，当维度不是2的倍数时，这个实现是无效的。那么，我们该如何纠正这个问题呢？

为了找到正确的解决方案，让我们可视化级别0中每个单元格在级别1中所覆盖的区域。

![img](https://sugulee.files.wordpress.com/2021/01/screen-shot-2021-01-17-at-6.04.27-pm.png?w=426)

每个黄色框对应级别1中每个单元格所覆盖的区域。正如图中所示，级别1中的每个单元格覆盖的是2.5×2.5个单元格，而不是级别0中的2×2个单元格。因此，当上一级的维度不是2的倍数时，这些单元格需要设置为3×3单元格的最小值。下面是修正后的实现代码

```
kernel void kernel_createHiZ(texture2d<float, access::read> depth [[ texture(0) ]],
                           texture2d<float, access::write> outDepth [[ texture(1) ]],
                                  uint2 tid [[thread_position_in_grid]])
{
	float2 depth_dim = float2(depth.get_width(), depth.get_height());
	float2 out_depth_dim = float2(outDepth.get_width(), outDepth.get_height());
	
	float2 ratio = depth_dim / out_depth_dim;
	
	uint2 vReadCoord = tid<<1;
        uint2 vWriteCoord = tid;
    
        float4 depth_samples = float4(
                                  depth.read(vReadCoord).x,
                                  depth.read(vReadCoord + uint2(1,0)).x,
                                  depth.read(vReadCoord + uint2(0,1)).x,
                                  depth.read(vReadCoord + uint2(1,1)).x
                                  );
	
        float min_depth = min(depth_samples.x, min(depth_samples.y, min(depth_samples.z, depth_samples.w)));
    
	bool needExtraSampleX = ratio.x>2;
        bool needExtraSampleY = ratio.y>2;
    
        min_depth = needExtraSampleX ? min(min_depth, min(depth.read(vReadCoord + uint2(2,0)).x, depth.read(vReadCoord + uint2(2,1)).x)) : min_depth;
        min_depth = needExtraSampleY ? min(min_depth, min(depth.read(vReadCoord + uint2(0,2)).x, depth.read(vReadCoord + uint2(1,2)).x)) : min_depth;
        min_depth = (needExtraSampleX && needExtraSampleY) ? min(min_depth, depth.read(vReadCoord + uint2(2,2)).x) : min_depth;
    
        outDepth.write(float4(min_depth), vWriteCoord);
}
```

## The SSR pass with Hi-Z Tracing


现在，我们已经在之前的阶段创建了Hi-Z纹理，我们可以继续进行有趣的SSR阶段，使用Hi-Z追踪方法。

在这个阶段，我们将使用与线性追踪方法相同的主体代码。唯一的区别是，它会以Hi-Z纹理作为输入，而不是场景深度纹理，并且在追踪光线时调用'FindIntersection_HiZ'而不是'FindIntersection_Linear

```
kernel void kernel_screen_space_reflection_hi_z(texture2d<float, access::sample> tex_normal_refl_mask [[texture(0)]],
                                                texture2d<float, access::sample> tex_hi_z [[texture(1)]],
                                                texture2d<float, access::sample> tex_scene_color [[texture(2)]],
                                                texture2d<float, access::write> tex_output [[texture(3)]],
                                                const constant SceneInfo& sceneInfo [[buffer(0)]],
                                                uint2 tid [[thread_position_in_grid]])
{
    float4 finalColor = 0;
    
    float4 NormalAndReflectionMask = tex_normal_refl_mask.read(tid);
    float4 color = tex_scene_color.read(tid);
    float4 normalInWS = float4(normalize(NormalAndReflectionMask.xyz), 0);
    float3 normal = (sceneInfo.ViewMat * normalInWS).xyz;
    float reflection_mask = NormalAndReflectionMask.w;
    
    float4 skyColor = float4(0,0,1,1);
	
    float4 reflectionColor = 0;
    if(reflection_mask != 0)
    {
	reflectionColor = skyColor;
        float3 samplePosInTS = 0;
        float3 vReflDirInTS = 0;
        float maxTraceDistance = 0;
        // Compute the position, the reflection vector, maxTraceDistance of this sample in texture space.
        ComputePosAndReflection(tid, sceneInfo, normal, tex_hi_z, samplePosInTS, vReflDirInTS, maxTraceDistance);
        
        // Find intersection in texture space by tracing the reflection ray
        float3 intersection = 0;
        float intensity = FindIntersection_HiZ(samplePosInTS, vReflDirInTS, maxTraceDistance, tex_hi_z, sceneInfo, intersection);
            
        // Compute reflection color if intersected
	reflectionColor = ComputeReflectedColor(intensity,intersection, skyColor, tex_scene_color);
    }
    
    // Add the reflection color to the color of the sample.
    finalColor = color + reflectionColor;
    
    tex_output.write(finalColor, tid);
}
```

我之前在关于线性追踪的先前文章中已经讨论过这部分代码。因此，我们直接跳过这一部分，进入名为'FindIntersection_HiZ'的函数，它使用Hi-Z追踪来寻找交点。

```
float FindIntersection_HiZ(float3 samplePosInTS,
                         float3 vReflDirInTS,
                          float maxTraceDistance,
                         texture2d<float, access::sample> tex_hi_z,
                         const constant SceneInfo& sceneInfo,
                         thread float3& intersection)
{
    const int maxLevel = tex_hi_z.get_num_mip_levels()-1;
	
    float2 crossStep = float2(vReflDirInTS.x>=0 ? 1 : -1, vReflDirInTS.y>=0 ? 1 : -1);
    float2 crossOffset = crossStep / sceneInfo.ViewSize / 128;
    crossStep = saturate(crossStep);
    
    float3 ray = samplePosInTS.xyz;
    float minZ = ray.z;
    float maxZ = ray.z + vReflDirInTS.z * maxTraceDistance;
    float deltaZ = (maxZ - minZ);

    float3 o = ray;
    float3 d = vReflDirInTS * maxTraceDistance;
	
    int startLevel = 2;
    int stopLevel = 0;
    float2 startCellCount = getCellCount(startLevel, tex_hi_z);
	
    float2 rayCell = getCell(ray.xy, startCellCount);
    ray = intersectCellBoundary(o, d, rayCell, startCellCount, crossStep, crossOffset*64);
    
    int level = startLevel;
    uint iter = 0;
    bool isBackwardRay = vReflDirInTS.z<0;
    float rayDir = isBackwardRay ? -1 : 1;

    while(level >=stopLevel && ray.z*rayDir <= maxZ*rayDir && iter<sceneInfo.maxIteration)
    {
        const float2 cellCount = getCellCount(level, tex_hi_z);
        const float2 oldCellIdx = getCell(ray.xy, cellCount);
        
        float cell_minZ = getMinimumDepthPlane((oldCellIdx+0.5f)/cellCount, level, tex_hi_z);
        float3 tmpRay = ((cell_minZ > ray.z) && !isBackwardRay) ? intersectDepthPlane(o, d, (cell_minZ - minZ)/deltaZ) : ray;
        
        const float2 newCellIdx = getCell(tmpRay.xy, cellCount);
        
        float thickness = level == 0 ? (ray.z - cell_minZ) : 0;
        bool crossed = (isBackwardRay && (cell_minZ > ray.z)) || (thickness>(MAX_THICKNESS)) || crossedCellBoundary(oldCellIdx, newCellIdx);
        ray = crossed ? intersectCellBoundary(o, d, oldCellIdx, cellCount, crossStep, crossOffset) : tmpRay;
        level = crossed ? min((float)maxLevel, level + 1.0f) : level-1;
        
        ++iter;
    }
    
    bool intersected = (level < stopLevel);
    intersection = ray;
	
    float intensity = intersected ? 1 : 0;
    
    return intensity;
}
```

```
    const int maxLevel = tex_hi_z.get_num_mip_levels()-1;
	
    float2 crossStep = float2(vReflDirInTS.x>=0 ? 1 : -1, vReflDirInTS.y>=0 ? 1 : -1);
    float2 crossOffset = crossStep / sceneInfo.ViewSize / 128;
    crossStep = saturate(crossStep);
    
    crossOffset.xy = crossStep.xy * HIZ_CROSS_EPILSON.xy;
```

这里，它设置了maxLevel，这只是Hi-Z纹理中最后一个mip的级别。

接下来，它设置了两个变量，'crossStep'和'crossOffset'。这两个变量用于将光线移动到四叉树网格中的下一个单元格。在我的代码中，设置'crossOffset'的方法与书中的代码略有不同。在书中，'crossOffset'是通过以下代码设置的。

几乎和我的代码一样。但问题在于书中完全没有解释'HIZ_CROSS_EPILSON'是如何计算的。因此，我不得不自己想出一个公式。'crossOffset'用于将光线推进一个很小的距离，以确保在进行单元格穿越后光线在下一个单元格内。（至于单元格穿越，我们稍后再详细讨论。）所以，我认为 'sceneInfo.ViewSize / 128' 是一个合理的值用于这个目的。

```c
float3 ray = samplePosInTS.xyz;
    float minZ = ray.z;
    float maxZ = ray.z + vReflDirInTS.z * maxTraceDistance;
    float deltaZ = (maxZ - minZ);
```

在接下来的部分，它将光线初始化到纹理空间中当前样本的位置。然后，它设置了'minZ'和'maxZ'，它们是光线在z轴上的最小和最大位置。这是我们将在z轴上追踪光线的范围。然后，它将'deltaZ'设置为'maxZ'和'minZ'之间的距离。

```
    float3 o = ray;
    float3 d = vReflDirInTS * maxTraceDistance;
```


下一步，代码设置了'o'和'd'。这是另一个我代码与书中原始代码不同的地方。这两个变量，'o'和'd'，用于使用深度来参数化反射光线的位置。

首先，让我展示一下书中的参数化方法。然后，我将解释该方法存在的问题以及我提出的解决方案。

在书中，'o'被设置为沿着反射方向的一个点，其z分量为0，从而将该位置放置在相机的近平面上。然后，'d'被设置为当它添加到'o'上时，将'o'沿着反射方向移动到z分量为1的点，从而将该位置放置在相机的远平面上。下面的图示说明了这个思想。

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-18-at-7.14.07-am.png)


使用'o'和'd'，我们可以使用下面的方程来根据深度作为参数获取沿着射线的任意点的位置。

ray(depth) = o + d * depth

使用这个方程，当深度为0时，射线变为'o'，当深度为1时，射线变为(o + d)。要获取射线起点的位置，我们只需要将深度插入到射线起点的位置（换句话说，就是当前样本的深度）。

这种方法的问题在于，根据反射射线的方向，虽然'o'和'd'的z分量始终为0和1，但'o'和'd'的xy分量可能会变得非常大（正或负）。这可能会导致精度问题，特别是当使用32位浮点数来表示这些数值时。请参见下面的图示作为例子。

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-18-at-7.32.30-am.png)

在这个例子中，我们可以看到，随着反射方向的x或y分量大于z分量，'o'的位置距离视图中心越来越远，'d'的长度变得更长，导致'o'和'd'的大小增加。

保持'o'和'd'的大小尽可能小是非常重要的，以最小化精度问题。因此，这是我用来最小化问题的解决方案。

首先，我将'o'设置为射线的起点，因为我们永远不会在起点之后追踪射线。

其次，我将以一种方式设置'd'，使得(o + d)是最大追踪点，其中最大追踪点是射线在离开可见边界之前的一个点。同样，这是可以的，因为我们永远不会需要在射线离开可见边界之后追踪射线。我们已经计算了这一点的位置。该点的距离从射线起点到最大追踪点的距离被存储在'maxTraceDistance'中。

通过这个解决方案，上面的例子变成了这样。

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-18-at-1.48.57-pm.png)


当使用这个解决方案时，'o'和'd'的XYZ分量的大小将永远不会超过1。这将有助于在后续的方程中使用'o'和'd'时最小化精度问题。

好的，现在让我们继续下一部分

```
    int startLevel = 2;
    int stopLevel = 0;
```

在这里，我们设置了'startLevel'和'stopLevel'。'startLevel'是Hi-Z四叉树中我们开始追踪光线的级别。'stopLevel'是Hi-Z四叉树中，当当前级别高于'stopLevel'时我们将停止追踪光线的级别。我们将在后续部分看到这些数字是如何使用的。

```
    float2 startCellCount = getCellCount(startLevel, tex_hi_z);
	
    float2 rayCell = getCell(ray.xy, startCellCount);
    ray = intersectCellBoundary(o, d, rayCell, startCellCount, crossStep, crossOffset*64, minZ, maxZ);
```


在代码的这一部分，目标是将当前光线沿反射方向移动到下一个单元格，以避免“自相交”。

函数'getCellCount'返回给定级别的四叉树中单元格的数量。其实现如下。

```
float2 getCellCount(int mipLevel, texture2d<float, access::sample> tex_hi_z)
{
    return float2(tex_hi_z.get_width(mipLevel) ,tex_hi_z.get_height(mipLevel));
}
```


所以，单元格计数就是特定mip级别的Hi-Z纹理的分辨率，其中mip级别对应着四叉树的级别。

函数'getCell'返回包含给定2D位置的单元格的二维整数索引。它以'pos'和'cell_count'作为输入。'pos'是要查找单元格的位置。'cell_count'是要查找单元格的四叉树级别的宽度和高度。代码如下。

```
float2 getCell(float2 pos, float2 cell_count)
{
    return float2(floor(pos*cell_count));
}
```

所以，使用getCellCount()，我们可以得到起始级别的四叉树的单元格数。然后，利用该单元格数，我们可以调用getCell来获取当前射线在起始级别四叉树上的单元格索引。

现在，我们有了射线所在的当前单元格索引。接下来，我们将使用'intersectCellBoundary'将射线推到靠近当前单元格的下一个单元格的边界上。因此，这之后，射线将位于下一个单元格，但仍然非常靠近当前单元格与下一个单元格之间的边界。下面的图示展示了这个函数的作用。

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-18-at-5.13.28-pm.png)

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-18-at-5.14.15-pm.png)


红色点表示当前位置。蓝色点表示使用'intersectCellBoundary'计算得到的新位置。箭头表示反射射线的方向。

以下是该函数的代码。

```c
float3 intersectCellBoundary(float3 o, float3 d, float2 cell, float2 cell_count, float2 crossStep, float2 crossOffset)
{
	float3 intersection = 0;
	
	float2 index = cell + crossStep;
	float2 boundary = index / cell_count;
	boundary += crossOffset;
	
	float2 delta = boundary - o.xy;
	delta /= d.xy;
	float t = min(delta.x, delta.y);
	
	intersection = intersectDepthPlane(o, d, t);
	
	return intersection;
}
```


在这个函数中，'crossStep'和'crossOffset'被使用了。'crossStep'被添加到当前单元格索引中以获取下一个单元格索引。通过将单元格索引除以单元格数，我们可以得到当前单元格和新单元格之间边界的位置。然后，'crossOffset'用于将位置稍微推进一点，以确保新位置不会正好在边界上。

接下来，计算新位置和原点之间的差值'delta'。'delta'被除以'd'向量的xy分量。在除法之后，'delta'的x和y分量将有0到1之间的值，表示delta位置离射线起点有多远。

然后，我们取'delta'的两个分量x和y中的最小值，因为我们想要穿越最近的边界。然后，我们最终可以调用'intersectDepthPlane'来计算新位置。

```
float3 intersectDepthPlane(float3 o, float3 d, float z)
{
	return o + d * z;
}
```

这个函数只是我之前介绍的用于计算反射射线上位置的参数化形式。

顺便说一下，在书中将起始位置推进到下一个单元格的代码如下：

```
ray = intersectCellBoundary(o, d, rayCell, startCellCount, crossStep, crossOffset);
```

在书中代码与我实现的代码之间唯一的区别是，在调用intersectCellBoundary时，我的实现会将64乘以'crossOffset'。这个乘法是必要的，以便在屏幕分辨率不是2的幂时，正确地将射线推入下一个单元格。为什么这是必要的呢？

我发现，如果没有这个乘法，并且startLevel大于0，调用'intersectCellBoundary'将射线推入下一个单元格的行为只会在起始级别中生效，而在起始级别以上的级别中却不会生效。请参考下面的例子：

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-18-at-6.03.45-pm.png)

绿色单元格的级别是5×5。黑色单元格的级别是2×2。有两个位置被推入到下一个单元格。顶部的位置工作得很正常，但中间的那个位置效果不太好。它将位置移动到了2×2级别的下一个单元格，但未能将位置移动到5×5级别的下一个单元格。当屏幕分辨率为2的幂时，不会出现这个问题。

增大crossOffset确实有助于解决这个问题，因为它将位置从边界进一步移开。这就是为什么我最终将crossOffset乘以64的原因。

好的，现在我们可以继续下一部分，实际跟踪光线

```
    int level = startLevel;
    uint iter = 0;
    bool isBackwardRay = vReflDirInTS.z<0;
    float rayDir = isBackwardRay ? -1 : 1;
    
    while(level >=stopLevel && ray.z*rayDir <= maxZ*rayDir && iter<sceneInfo.maxIteration)
    {
        const float2 cellCount = getCellCount(level, tex_hi_z);
        const float2 oldCellIdx = getCell(ray.xy, cellCount);
        
        float cell_minZ = getMinimumDepthPlane((oldCellIdx+0.5f)/cellCount, level, tex_hi_z);
        float3 tmpRay = ((cell_minZ > ray.z) && !isBackwardRay) ? intersectDepthPlane(o, d, (cell_minZ - minZ)/deltaZ) : ray;
        
        const float2 newCellIdx = getCell(tmpRay.xy, cellCount);
        
        float thickness = level == 0 ? (ray.z - cell_minZ) : 0;
        bool crossed = (isBackwardRay && (cell_minZ > ray.z)) || (thickness>(MAX_THICKNESS)) || crossedCellBoundary(oldCellIdx, newCellIdx);
        ray = crossed ? intersectCellBoundary(o, d, oldCellIdx, cellCount, crossStep, crossOffset) : tmpRay;
        level = crossed ? min((float)maxLevel, level + 1.0f) : level-1;
        
        ++iter;
    }
```


让我们按代码顺序看一下它做了什么。

首先，它开始一个while循环。退出条件是当前级别高于停止级别，或者射线的z坐标超过了maxZ。同时，我们通过'maxIteration'限制迭代次数。

在循环中，首先获取当前级别的单元格数量。然后，它获取当前射线在当前级别上的单元格索引。

接下来，它使用以下代码实现的'getMinimumDepthPlane'获取当前四叉树级别中当前单元格的最小深度。

```
float getMinimumDepthPlane(float2 p, int mipLevel, texture2d<float, access::sample> tex_hi_z)
{
	constexpr sampler pointSampler(mip_filter::nearest);
	return tex_hi_z.sample(pointSampler, p, level(mipLevel)).x;
}
```

这个函数只是简单地在给定的mip级别和采样位置上对Hi-Z纹理进行采样。

接下来，这里是另一个地方，我的实现与书中略有不同。以下是书中的代码。

```
float minZ = getMinimumDepthplane(ray.xy, level, rootLevel);
```

现在，书中没有提供'getMinimumDepthPlane'函数的具体实现。因此，我不知道这三个参数的实际用途。但是，如果我猜测正确的话，可能会使用'ray.xy'作为采样坐标，'level'作为mip级别。至于'rootLevel'的用途，我不确定。

因此，起初我尝试了与书中相同的方法，使用'ray.xy'从纹理中采样。但结果是，有时会导致采样错误的纹理位置。我猜测这是因为有时'ray.xy'正好位于两个采样之间的边界，而不幸的是，我得到了错误的采样位置。

因此，我没有使用'ray.xy'作为采样坐标，而是使用cellIndex来计算当前单元格中心的位置。然后，我使用该位置从纹理中采样。这样完全解决了错误采样的问题。

好的。一旦通过'getMinimumDepthPlane'获得了当前单元格的最小深度，我们需要将其与射线的当前深度进行比较。比较可能有两种结果。

A. 当前深度 >= 最小深度：这意味着射线在当前单元格的体积内。如果当前级别是第一级别，那么我们已经找到了一个交点。但是，如果当前级别不是第一级别，我们还不确定射线是否真正与任何物体相交。为了确定，我们需要回到上面的级别，一直回溯到第一级别。

B. 当前深度 < 最小深度：这意味着射线在当前单元格的体积外。在这种情况下，我们可以将射线进一步移动，直到新位置的z等于最小深度的位置。

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-19-at-1.25.08-pm.png)

```
float3 tmpRay = ((cell_minZ > ray.z) && !isBackwardRay) ? intersectDepthPlane(o, d, (cell_minZ - minZ)/deltaZ) : ray;
```

即使将射线进一步移动，新位置也会存储在一个临时变量中。这是因为当新位置不在当前单元格内时，需要重新计算新位置。

接下来的代码就是做这个事情。

它检查新位置的单元格索引是否与当前单元格索引相同。如果不同，意味着新位置在与当前单元格不同的单元格上。在这种情况下，我们不直接使用新位置，而是使用'intersectCellBoundary'来获取射线移动到下一个单元格的位置。

当厚度超过'MAX_THICKNESS'时，我们也会将射线移动到下一个单元格。这是一种允许射线穿过物体后面的方法。这必须仅在当前级别为0时进行。否则，我们会得到意外的结果。

如果射线移动到了下一个单元格，我们就会进入四叉树的较低级别。如果没有，我们就会进入较高级别的四叉树。

下面的视频演示了整个过程的工作原理。

https://youtu.be/gx91fb4gwuQ

黑色箭头和绿色点表示每次迭代时射线的位置。红色虚线和红色点表示每次迭代时射线的临时位置。绿色框表示每个四叉树级别的单元格。在示例中，起始级别和停止级别都设置为0。

好的，几乎完成了循环部分，除了还有一个细节要提出。

到目前为止，我所解释的都是反射射线远离相机（向前移动）的情况。但是，当射线朝向相机（向后移动）时，会发生什么呢？

这是书中完全忽略解释的部分。但是，为此想出自己的解决方案并不难。如果您查看代码，您可能已经注意到变量'isBackwardRay'，并且在一些代码行中使用了该变量以不同方式处理射线。正如您所看到的，'isBackwardRay'只在几行代码中使用。这些代码就足够使实现可以跟踪后向射线。

对于向前的射线，射线的z分量在每次迭代中递增，而对于向后的射线，z递减。这是因为后向射线移动到了深度为0的近平面。因此，在射线向后移动时，终止循环的条件需要进行翻转。

对于向前的射线，跟踪算法不适用于向后的射线。我们需要一个新的算法来跟踪后向射线，而这个算法比向前射线的算法甚至更简单。

在每次迭代中，我们获得当前单元格的最小z值。如果当前射线在当前单元格内部，则射线保持不变，并且我们转向更高的级别。另一方面，如果当前射线在当前单元格外部，我们可以直接穿越到下一个单元格，然后再进入较低的级别。

好的，这就是关于循环部分的全部内容。

最后一部分是检查射线是否相交。我们可以检查'level'是否高于'stopLevel'。

此外，我们可以将'intersection'设置为射线的当前位置。

##  Performance comparisons with linear tracing method.

这里我将展示线性追踪方法和Hi-Z追踪方法之间的性能比较。我将进行一系列比较，每个比较都使用不同的射线追踪循环最大迭代次数。循环将在迭代次数超过最大迭代次数、射线达到最大移动位置或者找到交点时停止。

GPU时间是在我的iMac Pro 2017型号上测量的，该GPU是'Radeon Pro Vega 64 16 GB'。

|                     | Linear(SSR Pass) | Hi-Z(HiZ Pass + SSR Pass) |
| ------------------- | ---------------- | ------------------------- |
| MaxIteration = 1000 | 2.33ms           | 1.77ms                    |
| MaxIteration = 300  | 1.3ms            | 1.2ms                     |
| MaxIteration = 100  | 0.7ms            | 1ms                       |
| MaxIteration = 25   | 0.45ms           | 0.8ms                     |

在看这个表格时，似乎在MaxIteraion >= 300时，Hi-Z方法运行得更快，而在MaxIteraion < 300时，线性方法运行得更快。但实际上，这并不是真实情况。这是因为即使使用相同的最大迭代次数，Hi-Z方法可以追踪得比线性方法更远。请看下面每个最大迭代次数对应的图片。

A. MaxIteration = 1000

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-19-at-1.40.41-pm.png)

B. MaxIteration = 300

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-19-at-1.49.09-pm.png)

C. MaxIteration = 100

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-19-at-1.52.28-pm.png)

D. MaxIteration = 25

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-19-at-1.56.34-pm.png)

如您所见，最大迭代次数为100的Hi-Z方法产生的结果几乎与最大迭代次数为1000的线性方法相同。现在，我们可以看到Hi-Z追踪方法的强大之处。

从书中提供的指导和代码片段开始，我花了一些时间和努力来使我的实现正确运行。我很高兴我做到了。

正如我所说，书中并没有完全详细地说明如何正确实现这个技术。因此，我希望这篇文章能帮助那些对这个技术感兴趣并试图自己实现它的人。

感谢您的阅读！