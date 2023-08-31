# Screen Space Reflections : Implementation and optimization – Part 1 : Linear Tracing Method

屏幕空间反射（SSR）是一种**实时渲染技术**，用于在表面上创建反射效果。多年来，它是许多游戏选择使用的最受欢迎的渲染技术之一。为什么这种技术如此受欢迎呢？首先，它**与延迟渲染管线非常兼容**，而延迟渲染管线是许多游戏的主要光照管线。其次，该技术的**实现与游戏系统的其他组件相互独立**。这意味着可以将此效果添加到现有的渲染系统中，而不会破坏其他组件。第三，**实现相当简单（尤其是线性追踪），代码量非常少**，但该效果在屏幕上显示时产生相当惊人的差异。

这种技术也有一些缺陷。由于它**使用场景深度缓冲作为世界几何的近似**，所以该技术无法创建完美的反射效果。屏幕之外的内容、被其他物体遮挡的内容或者透明物体都无法通过此技术捕捉到，因为它们并不存在于场景深度缓冲中。有一些方法可以在一定程度上减轻由这些限制造成的不良效果，但在本文中不会详细讨论这些方法。

相反，我将专注于对这种技术的性能优化。就像用于视频游戏的任何渲染技术一样，关键是使其运行速度尽可能快且高效。

在本文中，我将使用苹果的Metal作为渲染API来创建一个针对OS-X的示例应用程序。但我非常有信心这里介绍的优化方法可以应用于任何现代渲染API。

在示例应用程序中，我们在地板上创建了几个神庙模型和一个球体。SSR被应用于地板和球体。它会追踪反射光线，直到达到屏幕边缘。下面的图像显示了使用本文中解释的实现启用SSR（左）和禁用SSR（右）时的效果。

在深入讨论优化部分之前，我想简要解释一下SSR的实现原理。

SSR的工作原理基于这样一个事实：某些像素的反射像素存在于场景颜色纹理中，如下图所示。

它利用每个像素的深度和法线方向来计算反射光线。然后，它在屏幕空间中跟踪该光线，直到光线与场景几何体相交。光线与几何体相交的点即为要反射的像素位置。通过将反射像素的颜色与原始像素的颜色相加，它创建了反射效果。

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-15-at-3.17.02-pm.png)


SSR对于屏幕上的每个像素需要四个关键信息，以实现其反射效果：

1. 场景法线方向：用于计算反射向量。反射向量决定光线从表面反射的方向。
2. 场景深度：用于计算像素在场景中的三维位置。通过知道深度，SSR算法可以在屏幕空间中跟踪反射光线，并确定其与几何体相交的位置。
3. 场景颜色：场景颜色信息用于获取反射颜色。反射像素的颜色是从场景颜色纹理中相应位置获得的。
4. **反射掩码：反射掩码用于确定像素是否具有反射性质。它有助于跳过非反射像素的SSR计算，从而节省计算和性能资源。**

通过结合每个像素的这四个关键信息，SSR可以准确计算反射光线，以屏幕空间追踪光线，并实现实时渲染中反射表面的效果。


在使用延迟光照管线时，这4个纹理是由G缓冲pass（场景法线、场景深度、反射掩码）和光照pass（场景颜色）生成的。因此，在将SSR与延迟光照管线集成时，生成这些纹理不会增加额外的成本。这也意味着在开始SSRpass之前，G缓冲pass和光照pass必须已经完成。

对于正向光照管线，场景颜色纹理和场景深度纹理仍然是作为主渲染pass的一部分生成的。但是，场景法线纹理和反射掩码将需要额外的pass来生成，这就增加了额外的成本。

同时，我们也可以将法线向量和反射掩码存储在同一个纹理中，而不是将它们存储在各自的纹理中。这是因为法线向量需要3个分量，而反射掩码只需要1个分量，所以它们可以适合于具有4个分量格式的单一纹理中。这也是示例应用程序中的做法。

## The pseudo code

The SSR pass is executed using a **compute shader kernel**. Below is the pseudo code.

```
=Inputs : tex_sceneColor, tex_sceneDepth, tex_sceneNormalAndReflMask, sceneInfo, tid
// tex_sceneColor : color for each pixel
// tex_sceneDepth : depth in clip space for each pixel
// tex_sceneNormalAndReflMask : normal in world space, reflection mask (0:non-reflective, 1:reflective)
// tid : the thread position in grid. This is equivalent to the pixel position in the screen.
// sceneInfo is a structure contains information below
	// ViewSize : (width, height)
	// ViewMat : view transform matrix
	// ProjMat : projection matrix
	// InvProjMat : inverse projection matrix
Outputs : tex_outcolor

1. Get the normal and the reflection mask from 'tex_sceneNormalAndReflMask'
2. if reflection mask is not 0,
	3. Compute the position, the reflection vector, the max trace distance for the current sample in texture space.
	4. if the reflection vector is moving away from the camera
		5. Find the intersection between the reflection ray and the scene geometry by tracing the ray.
		6. If intersected
			7. Compute the reflection color by sampling the scene color texture at the intersection.
8. Add the reflection color to the color of the current sample to create the final color
```

## 4. The bottleneck

识别瓶颈是优化任何事物时的首要步骤之一。对于屏幕空间反射来说，它在GPU上花费大部分时间的部分是在**追踪反射光线**，以找到屏幕上光线与几何体（由场景深度缓冲近似）相交的位置。这在上面的伪代码中是步骤5。

因此，我将介绍一些针对追踪部分的优化技术，这些技术可能对提高性能非常有效。

## 5. The Linear Tracing Method

在屏幕空间中追踪光线的最简单方法是线性追踪。那么它是如何工作的呢？顾名思义，我们从原点开始追踪光线，在每一步中沿着光线方向线性地移动到深度纹理中的下一个样本。下面的图像展示了追踪过程的一般思路。

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-16-at-1.51.11-pm.png)

每一步中，它在光线的位置处对深度纹理中的样本进行采样，并将其与光线的深度进行比较。当采样深度小于光线的深度时，我们找到了一个交点（交点位置即为光线相交的位置）。

让我们来看一下使用线性追踪方法实现屏幕空间反射（SSR）的实际着色器代码。

**5-1 The Main Body.**

```c
kernel void kernel_screen_space_reflection_linear(texture2d<float, access::sample> tex_normal_refl_mask [[texture(0)]],
                                                  texture2d<float, access::sample> tex_depth [[texture(1)]],
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
        ComputePosAndReflection(tid, sceneInfo, normal, tex_depth, samplePosInTS, vReflDirInTS, maxTraceDistance);

        // Find intersection in texture space by tracing the reflection ray
        float3 intersection = 0;
	float intensity = FindIntersection_Linear(samplePosInTS, vReflDirInTS, maxTraceDistance, tex_depth, sceneInfo, intersection);
			
	// Compute reflection color if intersected
	reflectionColor = ComputeReflectedColor(intensity, intersection, skyColor, tex_scene_color);
    }

    // Add the reflection color to the color of the sample.
    finalColor = color + reflectionColor;

    tex_output.write(color, tid);
}
```

一行一行解释

```c
    float4 NormalAndReflectionMask = tex_normal_refl_mask.read(tid);
    float4 color = tex_scene_color.read(tid);
    float4 normalInWS = float4(normalize(NormalAndReflectionMask.xyz), 0);
    float3 normal = (sceneInfo.ViewMat * normalInWS).xyz;
    float reflection_mask = NormalAndReflectionMask.w;
```

这里，它在当前样本位置读取法线和反射掩码。这里的法线方向是世界空间中的。它还读取样本颜色。然后，将世界空间法线乘以视图矩阵以获得视图空间法线。接下来，它获取存储在法线反射掩码纹理的第四个分量中的反射掩码。

```
       // Compute the position, the reflection vector, maxTraceDistance of this sample in texture space.
        ComputePosAndReflection(tid, sceneInfo, normal, tex_depth, samplePosInTS, vReflDirInTS, maxTraceDistance);
```

然后，在这里我们计算当前像素的三维位置和反射向量，这两者都在纹理空间中。我们还计算了最大追踪距离。这个距离是为了限制光线追踪在可见区域内进行。

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-15-at-5.22.48-pm.png)

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-15-at-5.26.49-pm.png)

在这两个空间中有几个共同点。首先，在上面的图像所示边界之外的任何东西都不会显示在屏幕上。这就是为什么剪裁空间被称为“剪裁空间”，因为该空间用于剪裁掉不可见的像素。3D基元通常首先转换到世界空间，然后再转换到视图空间，接着使用透视矩阵和透视除法转换到剪裁空间。然后，在SSR着色器中，我们将它们转换到纹理空间。

![image-20230802205234838](D:\games\VulkanDemo\doc\image-20230802205234838.png)

```c
        // Find intersection in texture space by tracing the reflection ray
        float3 intersection = 0;
	float intensity = FindIntersection_Linear(samplePosInTS, vReflDirInTS, maxTraceDistance, tex_depth, sceneInfo, intersection);
			
	// Compute reflection color if intersected
	reflectionColor = ComputeReflectedColor(intensity, intersection, skyColor, tex_scene_color);
```

**5-2. ComputePosAndReflection**

```c
// Compute the position, the reflection direction, maxTraceDistance of the sample in texture space.
void ComputePosAndReflection(uint2 tid,
                             const constant SceneInfo& sceneInfo,
                             float3 vSampleNormalInVS,
                             texture2d<float, access::sample> tex_depth,
                             thread float3& outSamplePosInTS,
                             thread float3& outReflDirInTS,
                             thread float& outMaxDistance)
{
    float sampleDepth = tex_depth.read(tid).x;
    float4 samplePosInCS =  float4(((float2(tid)+0.5)/sceneInfo.ViewSize)*2-1.0f, sampleDepth, 1);
    samplePosInCS.y *= -1;

    float4 samplePosInVS = sceneInfo.InvProjMat * samplePosInCS;
    samplePosInVS /= samplePosInVS.w;

    float3 vCamToSampleInVS = normalize(samplePosInVS.xyz);
    float4 vReflectionInVS = float4(reflect(vCamToSampleInVS.xyz, vSampleNormalInVS.xyz),0);

    float4 vReflectionEndPosInVS = samplePosInVS + vReflectionInVS * 1000;
    vReflectionEndPosInVS /= (vReflectionEndPosInVS.z < 0 ? vReflectionEndPosInVS.z : 1);
    float4 vReflectionEndPosInCS = sceneInfo.ProjMat * float4(vReflectionEndPosInVS.xyz, 1);
    vReflectionEndPosInCS /= vReflectionEndPosInCS.w;
    float3 vReflectionDir = normalize((vReflectionEndPosInCS - samplePosInCS).xyz);

    // Transform to texture space
    samplePosInCS.xy *= float2(0.5f, -0.5f);
    samplePosInCS.xy += float2(0.5f, 0.5f);
    
    vReflectionDir.xy *= float2(0.5f, -0.5f);
    
    outSamplePosInTS = samplePosInCS.xyz;
    outReflDirInTS = vReflectionDir;
    
	// Compute the maximum distance to trace before the ray goes outside of the visible area.
    outMaxDistance = outReflDirInTS.x>=0 ? (1 - outSamplePosInTS.x)/outReflDirInTS.x  : -outSamplePosInTS.x/outReflDirInTS.x;
    outMaxDistance = min(outMaxDistance, outReflDirInTS.y<0 ? (-outSamplePosInTS.y/outReflDirInTS.y)  : ((1-outSamplePosInTS.y)/outReflDirInTS.y));
    outMaxDistance = min(outMaxDistance, outReflDirInTS.z<0 ? (-outSamplePosInTS.z/outReflDirInTS.z) : ((1-outSamplePosInTS.z)/outReflDirInTS.z));
}
```

```
    float sampleDepth = tex_depth.read(tid).x;
    float4 samplePosInCS =  float4(((float2(tid)+0.5)/sceneInfo.ViewSize)*2-1.0f, sampleDepth, 1);
    samplePosInCS.y *= -1;
```


在这里，它从深度纹理中读取当前样本的深度。下一步是计算样本在剪裁空间中的位置。'tid'是样本在屏幕上的像素位置。'sceneInfo.ViewSize'是屏幕分辨率。

**在进行这个计算时，添加0.5非常重要。这将像素位置移动到像素区域的中心**。这是为了确保在将剪裁空间中的位置转换回纹理像素空间进行采样时，我们得到正确的纹理采样位置。

下面的图像将展示当屏幕分辨率为1600×1200时，如何将'tid'转换为剪裁空间，以获取剪裁空间位置的xy分量。

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-15-at-6.05.21-pm.png)


正如您所看到的，剪裁空间中的y坐标是颠倒的，这就是为什么在代码中我们对y分量乘以-1。

剪裁空间位置的z分量就是从深度纹理中采样得到的深度值。

```c
    float4 samplePosInVS = sceneInfo.InvProjMat * samplePosInCS;
    samplePosInVS /= samplePosInVS.w;

    float3 vCamToSampleInVS = normalize(samplePosInVS.xyz);
    float4 vReflectionInVS = float4(reflect(vCamToSampleInVS.xyz, vSampleNormalInVS.xyz),0);
```

在这部分中，它在视图空间中计算反射方向。为了计算反射方向，我们需要两个向量：入射向量和表面法线向量，如下图所示：

![img](D:\games\VulkanDemo\doc\screen-shot-2021-01-15-at-6.10.28-pm.png)


关于法线向量，我们在之前的代码中已经获得了它。入射向量是从相机指向样本的向量。但是，由于我们是在视图空间中工作，相机的位置是(0,0,0)。因此，入射向量等于样本位置减去相机位置(0,0,0)，即入射向量等于样本位置本身。

接下来，我们使用Metal内置函数'reflect'来计算在视图空间中的反射方向。

下一步是计算在剪裁空间中的反射方向。

```c
    float4 vReflectionEndPosInVS = samplePosInVS + vReflectionInVS * 1000;
    vReflectionEndPosInVS /= (vReflectionEndPosInVS.z < 0 ? vReflectionEndPosInVS.z : 1);
    float4 vReflectionEndPosInCS = sceneInfo.ProjMat * float4(vReflectionEndPosInVS.xyz, 1);
    vReflectionEndPosInCS /= vReflectionEndPosInCS.w;
    float3 vReflectionDir = normalize((vReflectionEndPosInCS - samplePosInCS).xyz);
```


首先，我们通过在视图空间中的反射方向上移动当前样本位置的一个任意距离来计算射线的终点。这样可以计算出射线的终点。

在这里，我们需要注意处理反射终点的z分量成为负值的情况。如果出现这种情况，我们需要调整位置，以确保z分量不为负值。这是因为在下一步中应用的透视变换在位置的z分量为负值时无法正常工作。代码 'vReflectionEndPosInVS /= (vReflectionEndPosInVS.z < 0 ? vReflectionEndPosInVS.z : 1)' 就是处理这种情况的地方。

然后，我们通过乘以投影矩阵，然后除以w分量，将终点转换为剪裁空间。现在，可以通过在剪裁空间中对终点和样本位置之间的差值进行归一化来计算反射方向。

现在我们已经得到了剪裁空间中的反射方向，接下来的步骤是使用以下代码将样本位置和反射方向转换为纹理空间。

```
    samplePosInCS.xy *= float2(0.5f, -0.5f);
    samplePosInCS.xy += float2(0.5f, 0.5f);
    
    vReflectionDir.xy *= float2(0.5f, -0.5f);
    
    outSamplePosInTS = samplePosInCS.xyz;
    outReflDirInTS = vReflectionDir;
```

它使用了前面介绍的公式来将剪裁空间位置/方向转换为纹理空间位置/方向。

接下来的部分是计算最大的行进距离。

```
	// Compute the maximum distance to trace before the ray goes outside of the visible area.
    outMaxDistance = outReflDirInTS.x>=0 ? (1 - outSamplePosInTS.x)/outReflDirInTS.x  : -outSamplePosInTS.x/outReflDirInTS.x;
    outMaxDistance = min(outMaxDistance, outReflDirInTS.y<0 ? (-outSamplePosInTS.y/outReflDirInTS.y)  : ((1-outSamplePosInTS.y)/outReflDirInTS.y));
    outMaxDistance = min(outMaxDistance, outReflDirInTS.z<0 ? (-outSamplePosInTS.z/outReflDirInTS.z) : ((1-outSamplePosInTS.z)/outReflDirInTS.z));
```


最大行进距离是指当光线行进到这个距离时，它到达了纹理空间中可见区域的边界。因此，超过这个距离的行进只会浪费GPU的时间，因为在这个距离之外将没有可见的像素。

好的。这样就完成了函数'ComputePosAndReflection'的解释。这个函数只是为接下来的函数做准备，而接下来的函数将是SSR着色器的主要部分。

**5-3. FindIntersection_Linear**

```c
bool FindIntersection_Linear(float3 samplePosInTS,
                             float3 vReflDirInTS,
                             float maxTraceDistance,
                             texture2d<float, access::sample> tex_depth,
                             const constant SceneInfo& sceneInfo,
                             thread float3& intersection)
{
    constexpr sampler pointSampler;
    
    float3 vReflectionEndPosInTS = samplePosInTS + vReflDirInTS * maxTraceDistance;
    
    float3 dp = vReflectionEndPosInTS.xyz - samplePosInTS.xyz;
    int2 sampleScreenPos = int2(samplePosInTS.xy * sceneInfo.ViewSize.xy);
    int2 endPosScreenPos = int2(vReflectionEndPosInTS.xy * sceneInfo.ViewSize.xy);
    int2 dp2 = endPosScreenPos - sampleScreenPos;
    const int max_dist = max(abs(dp2.x), abs(dp2.y));
    dp /= max_dist;
    
    float4 rayPosInTS = float4(samplePosInTS.xyz + dp, 0);
    float4 vRayDirInTS = float4(dp.xyz, 0);
	float4 rayStartPos = rayPosInTS;

    int32_t hitIndex = -1;
    for(int i = 0;i<=max_dist && i<MAX_ITERATION;i += 4)
    {
        float depth0 = 0;
        float depth1 = 0;
        float depth2 = 0;
        float depth3 = 0;

        float4 rayPosInTS0 = rayPosInTS+vRayDirInTS*0;
        float4 rayPosInTS1 = rayPosInTS+vRayDirInTS*1;
        float4 rayPosInTS2 = rayPosInTS+vRayDirInTS*2;
        float4 rayPosInTS3 = rayPosInTS+vRayDirInTS*3;

        depth3 = tex_depth.sample(pointSampler, rayPosInTS3.xy).x;
        depth2 = tex_depth.sample(pointSampler, rayPosInTS2.xy).x;
        depth1 = tex_depth.sample(pointSampler, rayPosInTS1.xy).x;
        depth0 = tex_depth.sample(pointSampler, rayPosInTS0.xy).x;

        {
            float thickness = rayPosInTS3.z - depth3;
            hitIndex = (thickness>=0 && thickness < MAX_THICKNESS) ? (i+3) : hitIndex;
        }
        {
            float thickness = rayPosInTS2.z - depth2;
            hitIndex = (thickness>=0 && thickness < MAX_THICKNESS) ? (i+2) : hitIndex;
        }
        {
            float thickness = rayPosInTS1.z - depth1;
            hitIndex = (thickness>=0 && thickness < MAX_THICKNESS) ? (i+1) : hitIndex;
        }
        {
            float thickness = rayPosInTS0.z - depth0;
            hitIndex = (thickness>=0 && thickness < MAX_THICKNESS) ? (i+0) : hitIndex;
        }

        if(hitIndex != -1) break;

        rayPosInTS = rayPosInTS3 + vRayDirInTS;
    }

    bool intersected = hitIndex >= 0;
    intersection = rayStartPos.xyz + vRayDirInTS.xyz * hitIndex;
	
    float intensity = intersected ? 1 : 0;
	
    return intensity;
}
```

```
float3 vReflectionEndPosInTS = samplePosInTS + vReflDirInTS * maxTraceDistance;
    
    float3 dp = vReflectionEndPosInTS.xyz - samplePosInTS.xyz;
    int2 sampleScreenPos = int2(samplePosInTS.xy * sceneInfo.ViewSize.xy);
    int2 endPosScreenPos = int2(vReflectionEndPosInTS.xy * sceneInfo.ViewSize.xy);
    int2 dp2 = endPosScreenPos - sampleScreenPos;
    const int max_dist = max(abs(dp2.x), abs(dp2.y));
    dp /= max_dist;
```

在这部分中，目标是计算向量'dp'。这个向量将当前光线移动到下一个位置，通过将其添加到光线的当前位置来实现。在此过程中，需要避免过采样或欠采样。过采样意味着多次对相同的样本进行采样，这将浪费内存带宽和计算单元资源。欠采样意味着在光线方向上跳过一些样本，可能导致遗漏交点。我们都不希望发生这些情况。

```
    float4 rayPosInTS = float4(samplePosInTS.xyz + dp, 0);
    float4 vRayDirInTS = float4(dp.xyz, 0);
	float4 rayStartPos = rayPosInTS;
```


现在我们已经计算出了'dp'，我们通过设置光线的位置和方向来初始化追踪。将起点设置为样本位置加上'dp'，这有效地将起点移动到下一个位置。这是因为我们想避免自相交。

接下来的部分是我们运行一个循环来沿着光线前进。现在，为了更好地理解这部分内容，我们先看看最简单的版本，即版本0，而不是看最终版本，因为最终版本由于优化可能有些复杂。

```
    // Version 0.
    int32_t hitIndex = -1;
    for(int i = 0;i<max_dist && i<MAX_ITERATION;i ++)
    {
	float depth = tex_depth.sample(pointSampler, rayPosInTS.xy).x;
	float thickness = rayPosInTS.z - depth;
	if( thickness>=0 && thickness< MAX_THICKNESS)
	{
	        hitIndex = i;
		break;
	}
		
        rayPosInTS += vRayDirInTS;
    }
```


非常简单。首先，它在光线的位置从深度纹理中采样深度。然后，通过从光线深度中减去采样的深度来计算厚度。如果厚度大于零，这意味着光线距离相机的采样深度更远。因此，在这种情况下，我们可以说我们找到了一个交点。

但是，当厚度太大，大于阈值MAX_THICKNESS时，我们可以将其视为光线穿过物体的背面，而不是相交。这只是缓解SSR技术的局限性的一种方法。

程序完成循环后，我们可以检查hitIndex。如果hitIndex为-1，这意味着我们找不到交点。如果hitIndex大于或等于0，则意味着我们有一个交点。在这种情况下，我们可以使用hitIndex来获取交点的位置。

下表显示了在两台计算机上使用此代码运行SSR着色器的循环所需的GPU时间。

这个循环是SSR着色器花费大部分GPU时间的地方，因此值得尽可能优化它。首先，优化的一个方法是批量采样。我们可以在每次迭代中采样4个深度，而不是只采样1个深度。这样可以使硬件纹理单元更高效地运行。版本1是这个代码的优化版本。

```
int32_t hitIndex = -1;
    for(int i = 0;i<=max_dist && i<MAX_ITERATION;i += 4)
    {
        float depth0 = 0;
        float depth1 = 0;
        float depth2 = 0;
        float depth3 = 0;

        float4 rayPosInTS0 = rayPosInTS+vRayDirInTS*0;
        float4 rayPosInTS1 = rayPosInTS+vRayDirInTS*1;
        float4 rayPosInTS2 = rayPosInTS+vRayDirInTS*2;
        float4 rayPosInTS3 = rayPosInTS+vRayDirInTS*3;

        depth3 = tex_depth.sample(pointSampler, rayPosInTS3.xy).x;
        depth2 = tex_depth.sample(pointSampler, rayPosInTS2.xy).x;
        depth1 = tex_depth.sample(pointSampler, rayPosInTS1.xy).x;
        depth0 = tex_depth.sample(pointSampler, rayPosInTS0.xy).x;

        {
            float thickness = rayPosInTS0.z - depth0;
	    if(thickness>=0 && thickness < MAX_THICKNESS)
	    {
		hitIndex = i+0;
		break;
	    }
        }
	{
	    float thickness = rayPosInTS1.z - depth1;
	    if(thickness>=0 && thickness < MAX_THICKNESS)
		{
			hitIndex = i+1;
			break;
		}
	}
	{
		float thickness = rayPosInTS2.z - depth2;
		if(thickness>=0 && thickness < MAX_THICKNESS)
		{
			hitIndex = i+2;
			break;
		}
	}
	{
		float thickness = rayPosInTS3.z - depth3;
		if(thickness>=0 && thickness < MAX_THICKNESS)
		{
			hitIndex = i+3;
			break;
		}
	}

        if(hitIndex != -1) break;

        rayPosInTS = rayPosInTS3 + vRayDirInTS;
    }
```

优化后性能的提升大约在两台机器上都达到了5%，虽然这并不是非常显著，但还是有所改进。

也许会有人问，如果批量采样4个样本可以提高性能，为什么我们不使用更大的批量大小以获得更多的增益呢？这可能在某些GPU硬件上行不通。至少，在我使用的所有机器上都不行。原因在于，当您想使用更大的批量大小时，您将需要更多的寄存器来存储深度样本。什么是寄存器？寄存器是GPU使用的一种内存，速度非常快，但容量非常有限。着色器代码中的局部变量最终会使用这些寄存器。因此，像 'depth0'、'depth1' 这样的局部变量需要使用寄存器。而且要使用更大的批量大小，您将需要更多的 'depthN' 变量，这将增加寄存器内存的使用量。由于寄存器内存的大小非常有限，着色器使用的寄存器越多，GPU硬件上同时执行的线程数就越少，从而导致性能下降。这就是为什么增加批量大小并不总是有效的原因。

好的。接下来，我想处理的是循环中的条件分支。我希望将条件分支替换为条件赋值。但是，为什么呢？

使用条件分支时，只有代码块的一侧会被执行。例如，对于以下代码，只有在x被评估为true时才会执行'var = (a+b)'，而在x被评估为false时只会执行'var = (a-b)'。

```
int var = 0;
if(x)
{
	var = a+b;
}
else
{
	var = a-b;
}
```

```
int var = x ? (a+b) : (a-b);
```


与条件分支不同，使用条件赋值时，代码块的两侧都会被执行。因此，(a+b) 和 (a-b) 都会被执行。因此，看起来使用条件赋值比使用条件分支时，GPU要做更多的工作。那么，我怎么能说条件赋值比条件分支更高效呢？

这是因为执行条件分支本身的成本非常高。执行条件分支涉及到根据评估结果激活/停用SIMD组中的GPU线程。对于x被评估为true的线程，它运行代码块'var = a+b'，但然后它会被停用，直到其他x被评估为false的线程完成运行'var = a-b'。这是一种昂贵的操作。

| Steps | Threads with x = true | Threads with x = false |
| ----- | --------------------- | ---------------------- |
| 1     | evaluate x -> true    | evaluate x -> false    |
| 2     |                       | deactivate thread      |
| 3     | var = a+b             | de-activated           |
| 4     | deactivate thread     | activate thread        |
| 5     | de-activated          | var = a-b              |
| 6     | activate tread        |                        |

![image-20230802210412500](C:\Users\dc\AppData\Roaming\Typora\typora-user-images\image-20230802210412500.png)


因此，这就是为什么条件赋值比条件分支更高效。然而，当代码块内部的工作较为繁重时，条件分支可能比条件赋值更高效。

下面是带有条件赋值改变的代码。这也是循环的最终版本。

在这篇文章中，我解释了如何使用线性追踪方法来实现SSR，并介绍了一些优化技巧。希望这对您有所帮助。

还有其他方法可以进一步提高性能。例如，我们可以使用场景深度纹理的降采样版本，或者减小最大行进距离。在追踪光线时，我们还可以使用更大的步长。所有这些技术都能提高性能，尽管可能会降低视觉质量。

在下一篇文章中，我将介绍另一种追踪方法，称为“HI-Z追踪”，它在视觉质量上没有任何妥协的情况下大大优于线性追踪方法。这个方法稍微复杂一些，但性能提升很显著。

HI-Z追踪最初是在书籍“GPU PRO 5”中引入的。尽管书中对方法的工作原理有很好的解释，但未提供完整的源代码，这让人有些失望，因为仅仅通过阅读书籍中的说明和不完整的代码片段来实现这个方法并不那么简单。同时，在实现过程中，我遇到了几个问题，但最终找到了合适的解决方案。在下一篇文章中，我将分享完整的源代码，并清楚地解释如何实现这个方法。