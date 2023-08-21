## Temporal Antialiasing Starter Pack

https://alextardif.com/TAA.html

### Update

在许多方面，这篇帖子现在是一篇关于Emilio López关于TAA的最新视觉教程的子集，您可以在这里找到：https://www.elopezr.com/temporal-aa-and-the-quest-for-the-holy-trail/ 如果您想要全面了解TAA，并且不仅仅是实现参考，我强烈推荐您从那里开始，然后再将这个页面作为额外的资源。

### Overview


最近，我发现自己在赶上TAA方面的进展，与我之前关于尝试避免主渲染通道时的主动态抗锯齿的其他帖子有一定的关联。在浏览关于TAA的最近（或近似的）演示文稿时，我有这样的印象，即关于实现TAA的可靠文章不多（至少我没有找到），特别是对于第一次学习它并且可能觉得它很令人生畏，或者可能对如何将这些演示文稿和论文付诸实践感到困惑的人。事实是，对于任何对学习TAA感兴趣并且不愿付出大量时间投入的人来说，一个不错的TAA实现都是可以实现的。

我不打算深入探讨TAA的理论和背景，而是在移步到我认为是一种实用的通用TAA实现的基本部分之前，会提供比我更了解这方面的人的作品链接。我在这里做的一切都不是新的或原创的，只是将各种在线资源汇集到一个地方。

### Background

在这里，我将提供我认为对于我们将要实现的TAA类型至关重要的背景阅读/演示文稿。

These are worth reading before going any further in this post:

- http://advances.realtimerendering.com/s2014/#_HIGH-QUALITY_TEMPORAL_SUPERSAMPLING
- https://www.gdcvault.com/play/1022970/Temporal-Reprojection-Anti-Aliasing-in
- https://developer.download.nvidia.com/gameworks/events/GDC2016/msalvi_temporal_supersampling.pdf
- http://behindthepixels.io/assets/files/TemporalAA.pdf

### Velocity Buffer

首先要明确的是，我们需要填充一个运动向量目标，有时也称为速度缓冲区。初学时，您可能希使用RG16F格式，以确保具有足够的浮点精度，并且不需要将其编码为更优化的格式。每帧都将其清零。如果您正在进行完整的Z预通道，您可以在那里完成此操作；否则，您可以在主通道（正向通道、G缓冲通道等）中进行。为了填充目标，我们需要一些额外的常数 - 无论您在哪里传递相机的视图+投影矩阵，都要确保还存储了前一帧的矩阵，然后也将其提供给着色器。同样，对于要渲染的每个对象，除了当前的**世界矩阵**外，还要传递前一帧的世界矩阵。从长远来看，对于静态对象，最后一步是不必要的（稍后会详细讨论），但让我们保持简单，为所有对象都执行此操作。**对于蒙皮对象，您要么需要传递前一帧的骨骼变换矩阵，要么提供一个包含了前一帧变换过的顶点位置的缓冲区**。

From here the logic in the vertex shader is simple:

```c
o.currentPos = mul(float4(i.position, 1), worldMatrix);
o.currentPos = mul(o.currentPos, viewProjectionMatrix);
o.previousPos = mul(float4(i.position, 1), previousWorldMatrix);
o.previousPos = mul(o.previousPos, previousViewProjectionMatrix);
```


对于蒙皮对象，您还需要采用同样的蒙皮路径，使用前一帧的骨骼变换，或者如果您已经有了，也可以从某些存储缓冲区中读取先前变换的位置。

像素着色器的工作也很简单，但这一步骤也容易出现简单错误，而这些错误可能会在后续阶段带来麻烦，正如我们即将看到的那样

```
float3 currentPosNDC = i.currentPos.xyz / i.currentPos.w;
float3 previousPosNDC = i.previousPos.xyz / i.previousPos.w;
float2 velocity = currentPosNDC.xy - previousPosNDC.xy;
```

我认识的很少有人实际上将其输出为标准化设备坐标（NDC）。我在这篇文章中更喜欢这样做，因为我发现这样做可以更容易地避免后续步骤出错。熟悉TAA（时域抗锯齿）的人可能会想知道这一点是否有抖动 - 我有意在最后才引入抖动。我们将首先确保投影和采样是正确的，然后再引入额外的抖动复杂性。

### Resolve

现在是有趣的部分！TAA（时域抗锯齿）解算是我们要将之前从上述链接中学到的所有内容（以及更多内容）整合在一起的地方。在光照通道之后，但在任何后期处理之前，我们将插入TAA解算通道。这意味着我们将在未经色调映射的HDR目标上操作，正如上述链接中所描述的。解算着色器所需的输入包括：**源颜色**（当前帧的图像迄今为止）、**历史颜色**（**上一帧的TAA结果**）、**刚刚填充的运动向量目标以及深度缓冲区**。由于第一帧没有累积，对于第0帧的一个合理的默认操作是将源颜色直接复制到历史颜色中，然后跳过一帧的解算，或者在第一帧的解算中忽略历史颜色。考虑到这一点，让我们开始解算。为了简单起见，我们将使用一个**全屏三角形的像素着色器**来进行解算。

第一部分是邻域采样循环，它将为我们完成许多任务，我们将逐步介绍每个步骤。

```
float3 sourceSampleTotal = float3(0, 0, 0);
float sourceSampleWeight = 0.0;
float3 neighborhoodMin = 10000;
float3 neighborhoodMax = -10000;
float3 m1 = float3(0, 0, 0);
float3 m2 = float3(0, 0, 0);
float closestDepth = 0.0;
int2 closestDepthPixelPosition = int2(0, 0);
 
for (int x = -1; x <= 1; x++)
{
    for (int y = -1; y <= 1; y++)
    {
        int2 pixelPosition = i.position.xy + int2(x, y);
        pixelPosition = clamp(pixelPosition, 0, sourceDimensions.xy - 1);  
 
        float3 neighbor = max(0, SourceColor[pixelPosition].rgb);
        float subSampleDistance = length(float2(x, y));
        float subSampleWeight = Mitchell(subSampleDistance);
 
        sourceSampleTotal += neighbor * subSampleWeight;
        sourceSampleWeight += subSampleWeight;
 
        neighborhoodMin = min(neighborhoodMin, neighbor);
        neighborhoodMax = max(neighborhoodMax, neighbor);
 
        m1 += neighbor;
        m2 += neighbor * neighbor;
 
        float currentDepth = DepthBuffer[pixelPosition].r;
        if (currentDepth > closestDepth)
        {
            closestDepth = currentDepth;
            closestDepthPixelPosition = pixelPosition;
        }
    }
}
```


前两行是设置采样位置，确保不超过纹理范围。在这里，我传递了一个常数，您可以这样做，或者在源颜色输入中使用GetDimensions()。接下来，我们实际上从这一帧中对源颜色进行采样。max(0, ...) 这里的作用是确保我们不会从帧前面传播任何垃圾值，如果您曾经见过TAA在消耗整个图像之前"渗出"一些坏像素，那么这就是为什么我们这样做的原因！下面两行的目的与Karis的演示中描述的方法有关，但直到我在Tomasz Stachowiak的推特中找到了他的解释之前，我还没有完全理解幻灯片中的内容（非常感谢Tomasz）。他在这条推文和前一条推文中说了什么：

"顺便说一句，有一个方法**稍微有助于减少噪点/抖动**，即在TAA解算着色器内部对图像进行去抖动处理。不要在当前像素位置进行点采样/获取新帧，而是在局部邻域内进行小型滤波。Brian Karis在他的TAA演讲中详细介绍了这一点；他使用了Blackman-Harris滤波，但我发现Mitchell-Netravali滤波效果更好一些。您实际上在像素中心重建图像，将新帧视为一组子样本。这消除了抖动并稳定了图像。"

事实上，它确实完全按照他（和Karis）的描述进行操作。根据我的个人经验，我同意Tomasz的偏好是Mitchell滤波器。如果您以前没有实现过这样的滤波器，我强烈推荐查看Matt Pettineo的github滤波项目，其中包含了他在《The Order: 1886》的SIGGRAPH演讲中介绍的许多技术，这值得一读。

因此，现在我们正在为当前帧累积经过滤的样本信息，但我们还没有完成！接下来，我们需要获取邻域夹紧和方差剪切的信息。对于前者，我们收集neighborhoodMin和neighborhoodMax，对于后者，我们按照Salvi的演示中描述的方式收集第一和第二色彩矩。我们将在后面使用这些信息。最后，我们找到了邻域中最接近深度的样本位置，我们将用于采样速度缓冲区。请注意这里的大于号，我使用了一个反向深度缓冲区（您也应该这样做），如果您没有使用反向深度缓冲区，您将需要翻转closestDepth的符号和默认值。人们在决定在给定像素上最佳采样速度缓冲区的位置时会做出其他选择，例如有些人使用最大速度，但我更喜欢在最接近深度处使用速度（开头的链接也在某种程度上涵盖了这一点）。接着继续！

```
float2 motionVector = VelocityBuffer[closestDepthPixelPosition].xy * float2(0.5, -0.5);
float2 historyTexCoord = i.texCoord.xy - motionVector;
float3 sourceSample = sourceSampleTotal / sourceSampleWeight;
 
if(any(historyTexCoord != saturate(historyTexCoord)))
{
    return float4(sourceSample, 1);
}
 
float3 historySample = SampleTextureCatmullRom(HistoryColor, LinearSampler, historyTexCoord, float2(historyDimensions.xy)).rgb;
```

首先，我们获取我们的运动向量，这是通过采样速度缓冲区并将NDC向量转换为屏幕空间纹理坐标偏移来完成的。然后，我们使用从全屏三角形顶点着色器获得的当前纹理坐标，并减去运动向量偏移，以得到历史样本的纹理坐标。请注意，我在这里使用减法，是因为在填充速度缓冲区时进行减法的顺序。个人而言，我在思考正在进行的操作时更容易理解这个过程 - 通过减去运动向量以得到历史的纹理坐标。如果您不喜欢这样做，您可以在这里添加运动向量，并交换前面步骤中的减法。

接下来，我们计算新的（经过滤波的）源样本，并对历史纹理坐标进行简单检查，以查看是否超出了范围（0-1）。如果超出了范围，我们在这里停止并返回经过滤波的源样本。这是一个不完美的解决方案，但对于没有历史记录可供提取的情况来说，这是一个不错的起点。

最后，我们从上一帧中采样历史颜色，使用一个优化过的Catmull-Rom滤波器（再次感谢Matt！）由Matt Pettineo提供，向其中传入了历史颜色纹理、线性夹紧采样器、我们计算得到的历史样本位置以及通过常数或通过GetDimensions()传递的历史颜色纹理大小。

```
float oneDividedBySampleCount = 1.0 / 9.0;
float gamma = 1.0;
float3 mu = m1 * oneDividedBySampleCount;
float3 sigma = sqrt(abs((m2 * oneDividedBySampleCount) - (mu * mu)));
float3 minc = mu - gamma * sigma;
float3 maxc = mu + gamma * sigma;

historySample = clip_aabb(minc, maxc, clamp(historySample, neighborhoodMin, neighborhoodMax));
```

我们已经到达了方差剪切的计算部分，直接从Salvi的演示中引用而来。另外，我们还根据论文中的描述将历史样本夹紧在邻域最小/最大值之间，并将方差边界传递给Playdead在链接的INSIDE演示中提供的剪切函数。

```
float sourceWeight = 0.05;
float historyWeight = 1.0 - sourceWeight;
float3 compressedSource = sourceSample * rcp(max(max(sourceSample.r, sourceSample.g), sourceSample.b) + 1.0);
float3 compressedHistory = historySample * rcp(max(max(historySample.r, historySample.g), historySample.b) + 1.0);
float luminanceSource = Luminance(compressedSource);
float luminanceHistory = Luminance(compressedHistory); 
 
sourceWeight *= 1.0 / (1.0 + luminanceSource);
historyWeight *= 1.0 / (1.0 + luminanceHistory);
 
float3 result = (sourceSample * sourceWeight + historySample * historyWeight) / max(sourceWeight + historyWeight, 0.00001);
 
return float4(result, 1);

```

我们已经到达了最后！我们将使用一个相当不错的默认值来融合源样本的比例（0.05），并应用一些所谓的“抗闪烁”（在链接中有描述），以减少可能遇到的因抖动而引起的高频细节闪烁的可能性（我们很快会添加抖动）。这里的亮度函数只是与float3(0.2127, 0.7152, 0.0722)的点乘。这不会消除闪烁，事实上，要更好地做到这一点，您可能还想对源样本和历史样本采样应用亮度滤波。即使如此，您仍然会遇到闪烁问题，这涉及在其他通道中应用额外的缓解措施 - 镜面抗锯齿、预过滤、使诸如泛光在时间上具有感知等。至少这一步提供了一个可以为了说明目的而执行的缓解示例。亮度滤波本身有很多缺陷，包括在这种形式下不考虑不同颜色的感知亮度差异，但实际上这总比什么都不做要好。

好了，我们有了TAA解算！当您的相机移动时，您应该会得到一个漂亮的抗锯齿效果，并且幽灵效应很少，尤其是与更简单的TAA实现相比（这些滤波选择非常重要）。但是，当您让相机保持静止时，图像看起来非常锐化！是什么原因？在没有任何运动的情况下，我们只是一遍又一遍地对相同位置进行采样，没有什么需要混合的内容！于是，进入抖动。

### Jitter


通过在投影矩阵中应用子像素抖动，我们可以解决相机低/无运动的锯齿问题，其思想是通过足够的采样次数，您将在一个抗锯齿的图像上达到收敛和稳定。在游戏中，由于实时性的限制，这种收敛不会真正实现，但它足以给您提供看起来很好的效果。在深入讨论以上内容之前，我想从一个我非常欣赏的人的角度提出一些有趣的思考：

"有时候，我喜欢对此进行一些讽刺，认为抖动只对在相机和世界完全静止时拍摄的时髦照片有用。我的意思是，你可以使用任何你想要的花哨模式进行抖动，然后基于这些偏移应用重建滤波器，但一旦相机移动，一切都不适用了。并不是图像中的每个元素都会完全以像素大小的增量沿X/Y轴平移，实际上，你的着色点将在所有几何体上"滑动"。当然，这也是为什么在重新投影后对前一帧纹理进行采样时，滤波器的选择如此重要，因为在当前帧的精确着色点并不会很好地位于前一帧的像素中心。它总是处于中间位置，因此更清晰的重建将使图像不会变得太模糊和模糊。根据游戏的情况，当然还有一些时候，尽管相机在移动，但屏幕某些部分的有效运动基本为0，但反过来，如果某些东西不动，也许您应该将采样点保持不变，以获得稳定的图像，而不是从抖动模式中引入可能的闪烁问题。"

这些都是要牢记的好思考，现在我们将继续将标准的抖动添加到我们已经实现的内容中。首先是生成抖动偏移量，当前流行的选项是使用(2, 3)的Halton序列来生成(x, y)的偏移。以下是从维基页面上的伪代码快速实现的内容。

```
float Halton(uint32_t i, uint32_t b)
{
    float f = 1.0f;
    float r = 0.0f;
 
    while (i > 0)
    {
        f /= static_cast<float>(b);
        r = r + f * static_cast<float>(i % b);
        i = static_cast<uint32_t>(floorf(static_cast<float>(i) / static_cast<float>(b)));
    }
 
    return r;
}
```

现在我们将使用它来为每一帧生成抖动偏移量。

```
float haltonX = 2.0f * Halton(jitterIndex + 1, 2) - 1.0f;
float haltonY = 2.0f * Halton(jitterIndex + 1, 3) - 1.0f;
float jitterX = (haltonX / dimensions.x);
float jitterY = (haltonY / dimensions.y);
```

在这里，变量jitterIndex每帧递增，直到达到所需的采样次数，dimensions是您的渲染目标尺寸。一个不错的起点是使用8（因此jitterIndex++; jitterIndex = jitterIndex % 8;）来进行实验，看看哪个对您的应用程序有效。请注意在Halton函数中的"+ 1"输入，以避免第一个索引返回0。要应用它，您可以将这个jitterX/Y添加到投影矩阵的[2][0]和[2][1]，或者[0][2]和[1][2]（取决于行优先还是列优先），或者更明确一点，您可以构造一个带有jitter的平移矩阵，然后将其与投影矩阵相乘。您还需要将这个抖动存储在一个常数中，就像我们在处理视图和投影矩阵时所做的那样，我们会跟踪上一帧的抖动并将其作为一个常数传递。我们需要在下一步中对速度缓冲生成进行修改时使用这些值。

```
float2 velocity = (currentPosNDC.xy - jitter) - (previousPosNDC.xy - previousJitter);
```

在处理静态世界/相机时，这一步实际上非常重要，因为如果我们不移除抖动，我们将在我们的预期重建区域之外进行采样，从而创建出我们不想要的模糊结果。更简单地说，我们希望在没有运动时，运动向量为零！这样，抖动的投影将按预期工作。虽然可能不太明显，但也很重要的是，在源颜色样本滤波过程中，将当前帧的抖动纳入subSampleDistance中，这很可能仍然有益处。

### Sweeteners


除了像我们在这里实现的TAA组件之外，人们还有很多其他的技巧来改进他们的TAA实现。大多数情况下，我将这些技巧归类为"附加特性"，也就是说，这些特性很可能是针对您所使用的引擎、游戏中的内容类型或两者的组合而设计的。我认为从再投影、裁剪、夹紧等方面将这些特性与其他特性区分开是很重要的，因为往往其他特性并不一定能够在不同情境之间很好地转化，因此当人们尝试这些特性时，它们可能无法按预期工作。

当您开始涉及基于深度和基于速度的样本接受/拒绝，或者使用模板遮罩您的TAA时，您可能正在涉及更多的游戏特定内容。我认为使用YCoCg进行裁剪可能也属于这个类别。至少从传统实现的角度来看，根据我所见，它在某些情况下可能效果更好，但在其他情况下可能效果不佳。我对感知亮度的有限探索使我对使用YCoCg并认为这样就完成了感到有些怀疑。在实践中，很可能需要进行更多的调整，即使这样，我也觉得可能还有其他的颜色空间可供使用。不过，我绝对同意导致在TAA中使用YCoCg的思考过程，我认为有很多潜力可以进一步扩展它。我也计划这样做，无论成功与否，我都会分享我的发现。不过不要只听我的意见，自己尝试一下，看看您的想法如何。

### Optimization

我上面的实现并不是经过优化的。考虑到所涉及的采样，通过将其转换为计算着色器并利用组共享内存，可以在很大程度上提高性能。另一个重要的优化是在填充速度/运动的过程中，**不要为静态对象导出速度/运动**。对于静态对象，除了相机运动之外，运动向量将为0，因此一个明显的改进是不要为静态对象写入它们，而是在该过程之后运行一个计算着色器，通过从深度缓冲区中的值进行重新投影来应用相机运动，并将其写入速度缓冲区。

### Challenges

开始使用TAA的初步困难在于第一次将其正确地启动和运行起来。微小的错误很容易破坏您的结果，一个遗漏的乘法或不正确的转换可能意味着在大量幽灵效应和相对干净的图像之间存在差异。更糟糕的是，微小的错误可能导致大多数情况下不易察觉的细微问题，这些问题可能难以追踪。这就是为什么我上面的方法着重于许多基础知识，并逐步处理它们。从简单开始，然后逐渐添加功能，测试，验证，再次测试。

一旦您完成了初步的实现，您在处理不太常见的情况时，可能会遇到一些问题，比如透明度、粒子特效或带有UV滚动效果的任何内容等。请查看我在顶部链接的两个示例，这些示例说明了您在生产环境中可能遇到的挑战。没有哪个TAA实现能够免受这些问题的影响，每个实现都需要在项目的整个生命周期中进行维护和调整。这也许是为什么我们会对特定的实现产生如此强烈的情感，因为就像养育宠物一样，我们花费了多年的时间来培养它们 :-)

唐·威廉姆森的一个“有趣”例子：“我曾经有一个客户，将对象放置在场景图层次结构中，将它们作为根的子对象。然后他们会取消父子关系，将其传递给图形处理部分，但在图形处理管道的两个部分中分别使用了初始矩阵和取消父子关系后的矩阵，结果产生了微妙的再投影漂移。追踪这个问题花了一些时间！”

### A Plea For Accessibility

我会尽量保持简短，但这是我一直强调到筋疲力尽的事情：如果您要发布一款使用TAA的游戏，请考虑一下TAA的可访问性因素。我完全意识到现在大多数渲染器依赖TAA来实现足够好的性能和质量，以支持许多现代技术，但另一方面，这种对TAA的依赖作为渲染管线的一部分留下了对其不利因素敏感的人，比如幽灵效应和抖动引起的晕动症。如果有一款游戏强制使用TAA，我敢保证游戏社区肯定已经制作了可以禁用TAA的模组，因为对于某些玩家，这意味着能否玩游戏的问题。至少我们可以做出最小的努力，在菜单中提供一个勾选框，用于禁用TAA、抖动，或两者都禁用，并附带有关图像质量的警告，或者更进一步，还可以提供一个滑块，用于控制剪切的强度，以让玩家可以选择在降低幽灵效应的情况下接受更多的闪烁和锯齿效果。我个人希望看到更强大的解决方案，但我也认识到在生产时间表内能做的事情有限。总比什么都不做要好，让我们做得更多些。

### Other TAA Implementations