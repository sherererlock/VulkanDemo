## Screen Space Reflection (SSR)

添加反射可以真正使您的场景更加真实。湿润和有光泽的物体会栩栩如生，因为没有什么比反射更能使物体看起来湿润或有光泽。有了反射，您可以更好地展现水和金属物体的幻象。

在光照部分，您模拟了光源的反射、镜子般的图像。这个过程被称为渲染镜面反射。记得镜面光是通过计算反射光线的方向来获得的。类似地，使用屏幕空间反射（Screen Space Reflection，SSR），您可以模拟场景中其他物体的反射，而不仅限于光源。光线不再来自光源，然后反射进入相机，而是来自场景中的某个物体，然后反射进入相机。

SSR的工作原理是将屏幕图像通过自身反射到自身。这与立方体贴图（cube mapping）不同，后者使用六个屏幕或纹理。在立方体贴图中，您将光线从场景中的某一点反射到包围场景的立方体内的某一点。而在SSR中，您将光线从屏幕上的某一点反射到屏幕上的另一点。通过将屏幕映射到自身，您可以创造出反射的幻象。尽管这个幻象在大部分情况下都是有效的，但SSR在某些情况下可能会出现问题，正如您将看到的。

### Ray Marching

屏幕空间反射使用一种称为"射线步进"的技术来确定每个片段的反射效果。射线步进是一种通过迭代延长或缩短某个向量的长度或大小，以便在某个空间中探测或采样信息的过程。在屏幕空间反射中，射线是关于法线的位置向量。

直观地说，光线在场景中某个点发生碰撞，然后反射，沿着反射位置向量的相反方向前进，再次反射，沿着位置向量的相反方向前进，并最终碰撞到相机镜头。这样，您就可以在当前片段中看到场景中某个点的颜色被反射出来。屏幕空间反射是沿着光线的路径进行反向追踪的过程。它试图找到光线反射的点，即与当前片段相交的点。在每次迭代中，算法沿着反射射线对场景的位置或深度进行采样，并询问每次光线是否与场景的几何体相交。如果有相交，那么该场景中的位置可能成为当前片段反射的候选位置。

理想情况下，能够准确地确定第一个交点的解析方法是存在的。这个第一个交点是唯一有效的反射点，可以用于当前片段的反射。但是，这种方法更像是战舰游戏（battleship）。您无法看到交点（如果有的话），因此您从反射射线的起点开始，在向反射方向前进的过程中报出坐标。每次报出后，您会得到一个回答，告诉您是否碰撞到了某些物体。如果碰撞到物体，您会尝试在该区域附近尝试其他点，希望找到精确的交点。

在这里，您可以看到射线步进被用于计算每个片段的反射点。顶点法线是明亮绿色箭头，位置向量是明亮蓝色箭头，而明亮红色向量则是通过视图空间进行射线步进的反射射线。

![SSR Ray Marching](https://i.imgur.com/wnAC7NI.gif)

### Vertex Positions

view space

To compute the reflections, you'll need the vertex normals in view space. Referrer back to [SSAO](https://lettier.github.io/3d-game-shaders-for-beginners/ssao.html#vertex-normals) for details.

### Position Transformations

正如SSAO（屏幕空间环境光遮蔽）一样，SSR也在屏幕空间和视图空间之间来回转换。您需要相机镜头的投影矩阵，将视图空间中的点转换到裁剪空间。从裁剪空间，您需要再次将点转换到UV空间。一旦在UV空间中，您可以从场景中采样一个顶点/片段位置，该位置将是场景中距离您采样点最近的位置。这是屏幕空间反射中的屏幕空间部分，因为"屏幕"是一个通过UV映射在屏幕形状矩形上的纹理。

### Reflected UV Coordinates

有几种方法可以实现屏幕空间反射（SSR）。示例代码通过为每个屏幕片段计算反射的UV坐标来开始反射过程。您也可以跳过这一步，直接使用场景的最终渲染结果来计算反射的颜色。

请回忆一下，UV坐标的范围是从0到1，对于U和V轴都是如此。屏幕可以看作是一个2D纹理，通过UV映射到屏幕大小的矩形上。基于这一点，示例代码实际上不需要场景的最终渲染结果来计算反射。它可以预测每个屏幕像素最终会使用的UV坐标。然后，这些计算得到的UV坐标可以保存到帧缓冲纹理中，并在场景渲染完成后使用。

这种方法是高效的，因为它避免了仅用于SSR的额外渲染过程。相反，它预测了反射信息，并在最终渲染过程中使用，降低了总体计算成本。

```
uniform mat4 lensProjection;

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;

  float maxDistance = 15;
  float resolution  = 0.3;
  int   steps       = 10;
  float thickness   = 0.5;
  
   vec2 texSize  = textureSize(positionTexture, 0).xy;
  vec2 texCoord = gl_FragCoord.xy / texSize;

  vec4 positionFrom     = texture(positionTexture, texCoord);
  vec3 unitPositionFrom = normalize(positionFrom.xyz);
  vec3 normal           = normalize(texture(normalTexture, texCoord).xyz);
  vec3 pivot            = normalize(reflect(unitPositionFrom, normal)); 
```


像其他效果一样，屏幕空间反射（SSR）有一些可以调整的参数。根据场景的复杂性，可能需要一些时间来找到合适的设置。在反射复杂几何体时，使屏幕空间反射看起来完美通常比较困难。

maxDistance参数控制片段可以反射的最大距离。换句话说，它控制反射射线的最大长度或大小。

resolution参数控制在第一次pass时在沿着反射射线的方向上跳过多少片段。这第一次pass是为了找到射线方向上的一个点，该点在场景中进入或穿过一些几何体。可以将这第一次pass视为粗略的过程。注意，resolution的取值范围是从零到一。零将导致没有反射，而一将在反射射线的方向上逐个片段地进行跟踪。resolution为一可能会明显降低您的帧率，尤其在maxDistance较大时。

steps参数控制在第二次pass时进行多少次迭代。这第二次pass是为了找到沿着反射射线的方向，射线立即与场景中的一些几何体相交的确切点。可以将这第二次pass视为细化的过程。

thickness参数控制哪些地方被视为可能的反射点，哪些不被视为反射点。理想情况下，希望射线立即在场景中的某个相机捕捉到的位置或深度处停止。这将是光线反射、碰撞到当前片段并反射进入相机的确切点。不幸的是，计算并不总是那么精确，因此thickness提供了一些余地或容差。希望thickness尽可能小——刚好超出采样位置或深度的短距离。

收集当前片段的位置、法线和关于法线的反射向量。positionFrom是从相机位置指向当前片段位置的向量。normal是指向当前片段的插值顶点法线方向的向量。pivot是反射射线或者说关于positionFrom向量反射方向的向量。它目前的长度或大小为1。

```
  vec4 startView = vec4(positionFrom.xyz + (pivot *           0), 1);
  vec4 endView   = vec4(positionFrom.xyz + (pivot * maxDistance), 1);
  
   vec4 startFrag      = startView;
       // Project to screen space.
       startFrag      = lensProjection * startFrag;
       // Perform the perspective divide.
       startFrag.xyz /= startFrag.w;
       // Convert the screen-space XY coordinates to UV coordinates.
       startFrag.xy   = startFrag.xy * 0.5 + 0.5;
       // Convert the UV coordinates to fragment/pixel coordnates.
       startFrag.xy  *= texSize;

  vec4 endFrag      = endView;
       endFrag      = lensProjection * endFrag;
       endFrag.xyz /= endFrag.w;
       endFrag.xy   = endFrag.xy * 0.5 + 0.5;
       endFrag.xy  *= texSize; 
```

将这些起始点和结束点从视图空间投影或转换到屏幕空间。这些点现在是片段位置，对应于屏幕上的像素位置。既然您知道射线在屏幕上的起始点和结束点，您可以在屏幕空间中沿着其方向移动或行进。将这个射线看作是在屏幕上画的一条线。您将沿着这条线行进，并使用它来采样存储在位置帧缓冲纹理中的片段位置。

![Screen space versus view space.](D:\Games\VulkanDemo\doc\MpBR225.png)

请注意，您可以在视图空间中沿着射线行进，但这可能会对位置帧缓冲纹理中的场景位置进行欠采样或过采样。请回忆一下，位置帧缓冲纹理的大小和形状与屏幕相同。每个屏幕片段或像素对应于相机捕捉到的某个位置。一个反射射线在视图空间中可能会行进很长的距离，但在屏幕空间中，它可能只会穿过几个像素。您只能对屏幕的像素位置进行采样，因此在视图空间中行进时可能会导致重复采样相同的像素，效率较低。通过在屏幕空间中行进，您将更有效地采样射线实际上所占据或覆盖的片段或像素。

```
  vec2 frag  = startFrag.xy;
       uv.xy = frag / texSize;
```

第一次pass将从反射射线的起始片段位置开始。通过将片段位置的坐标除以位置纹理的尺寸，将片段位置转换为UV坐标。

```
  float deltaX    = endFrag.x - startFrag.x;
  float deltaY    = endFrag.y - startFrag.y;
```

计算结束片段和起始片段的X和Y坐标之间的差异（delta）。这将告诉您射线在屏幕的X和Y维度上占据了多少像素。

![The reflection ray in screen space.](D:\Games\VulkanDemo\doc\Um4dzgL.png)

```
  float useX      = abs(deltaX) >= abs(deltaY) ? 1 : 0;
  float delta     = mix(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0, 1);
```


为了处理射线可能采取的各种不同方向（垂直、水平、对角线等），您需要跟踪并使用较大的差异。较大的差异将帮助您确定每次迭代在X和Y方向上要前进多少，需要多少次迭代才能行进整个线，并且当前位置表示线的百分比。

useX要么是1，要么是0。它用于根据哪个差异更大来选择X或Y维度。delta是两个X和Y差异中较大的那个。它用于确定每次迭代时在每个维度上前进多少，并在第一次pass期间需要多少次迭代。

```
  vec2  increment = vec2(deltaX, deltaY) / max(delta, 0.001);
```

通过使用两个差异中较大的那个来计算X和Y位置要增加多少。如果两个差异相同，每次迭代它们都会增加1。如果一个差异大于另一个，较大的差异将增加1，而较小的差异将增加小于1的值。这假设resolution为1。如果resolution小于1，算法将跳过一些片段。

```
startFrag  = ( 1,  4)
endFrag    = (10, 14)

deltaX     = (10 - 1) = 9
deltaY     = (14 - 4) = 10

resolution = 0.5

delta      = 10 * 0.5 = 5

increment  = (deltaX, deltaY) / delta
           = (     9,     10) / 5
           = ( 9 / 5,      2)
```

For example, say the `resolution` is 0.5. The larger dimension will increment by two fragments instead of one.

```
  float search0 = 0;
  float search1 = 0;
current position x = (start x) * (1 - search1) + (end x) * search1;
current position y = (start y) * (1 - search1) + (end y) * search1;
 int hit0 = 0;
  int hit1 = 0;
```

To move from the start fragment to the end fragment, the algorithm uses linear interpolation.


search1的取值范围是从零到一。当search1为零时，当前位置是起始片段。当search1为一时，当前位置是结束片段。对于任何其他值，当前位置介于起始片段和结束片段之间。

search0用于记住射线未命中或未与任何几何体相交的线上的上一个位置。在第二次pass中，算法将使用search0来帮助细化射线与场景几何体接触的点。

hit0指示在第一次pass中有交点（即射线在第一次pass中与场景发生了相交）。hit1指示在第二次pass中有交点（即射线在第二次pass中与场景发生了相交）。

```
  float viewDistance = startView.y;
  float depth        = thickness;
```

viewDistance的值表示射线上当前点距离相机有多远。请回忆在Panda3D中，Y维度在视图空间中表示屏幕的前后方向。而在其他系统中，Z维度在视图空间中表示屏幕的前后方向。无论如何，viewDistance表示射线当前距离相机有多远。请注意，如果您使用深度缓冲而不是视图空间中的顶点位置，viewDistance将等于Z深度。

请确保不要将viewDistance的值与射线在屏幕上横跨的Y维度混淆。viewDistance从相机向场景内部延伸，而射线在屏幕上的Y维度是沿着屏幕向上或向下移动。

depth是当前射线点与场景位置之间的视图距离差异。它告诉您射线当前在场景的后面还是前面有多远。请记住，场景位置是存储在位置帧缓冲纹理中的插值顶点位置。

```
for (i = 0; i < int(delta); ++i) {
    frag      += increment;
    uv.xy      = frag / texSize;
    positionTo = texture(positionTexture, uv.xy);
       search1 =
      mix
        ( (frag.y - startFrag.y) / deltaY
        , (frag.x - startFrag.x) / deltaX
        , useX
        ); 
```

现在您可以开始第一次pass。第一次pass在i小于delta值时运行。当i等于delta时，算法已经沿着整条线行进了。请记住，delta是X和Y两个差异中较大的那个。

![Screen Space Transformations](D:\Games\VulkanDemo\doc\Qnsvkc0.gif)

```c
/*
  (C) 2019 David Lettier
  lettier.com
*/

#version 150

uniform mat4 lensProjection;

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D maskTexture;

uniform vec2 enabled;

out vec4 fragColor;

void main() {
  float maxDistance = 8;
  float resolution  = 0.3;
  int   steps       = 5;
  float thickness   = 0.5;

  vec2 texSize  = textureSize(positionTexture, 0).xy;
  vec2 texCoord = gl_FragCoord.xy / texSize;

  vec4 uv = vec4(0.0);

  vec4 positionFrom = texture(positionTexture, texCoord);
  vec4 mask         = texture(maskTexture,     texCoord);

  if (  positionFrom.w <= 0.0
     || enabled.x      != 1.0
     || mask.r         <= 0.0
     ) { fragColor = uv; return; }

  vec3 unitPositionFrom = normalize(positionFrom.xyz);
  vec3 normal           = normalize(texture(normalTexture, texCoord).xyz);
  vec3 pivot            = normalize(reflect(unitPositionFrom, normal));

  vec4 positionTo = positionFrom;

  vec4 startView = vec4(positionFrom.xyz + (pivot *         0.0), 1.0);
  vec4 endView   = vec4(positionFrom.xyz + (pivot * maxDistance), 1.0);

  vec4 startFrag      = startView;
       startFrag      = lensProjection * startFrag;
       startFrag.xyz /= startFrag.w;
       startFrag.xy   = startFrag.xy * 0.5 + 0.5;
       startFrag.xy  *= texSize;

  vec4 endFrag      = endView;
       endFrag      = lensProjection * endFrag;
       endFrag.xyz /= endFrag.w;
       endFrag.xy   = endFrag.xy * 0.5 + 0.5;
       endFrag.xy  *= texSize;

  vec2 frag  = startFrag.xy;
       uv.xy = frag / texSize;

  float deltaX    = endFrag.x - startFrag.x;
  float deltaY    = endFrag.y - startFrag.y;
  float useX      = abs(deltaX) >= abs(deltaY) ? 1.0 : 0.0;
  float delta     = mix(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0.0, 1.0);
  vec2  increment = vec2(deltaX, deltaY) / max(delta, 0.001);

  float search0 = 0;
  float search1 = 0;

  int hit0 = 0;
  int hit1 = 0;

  float viewDistance = startView.y;
  float depth        = thickness;

  float i = 0;

  for (i = 0; i < int(delta); ++i) {
    frag      += increment;
    uv.xy      = frag / texSize;
    positionTo = texture(positionTexture, uv.xy);

    search1 =
      mix
        ( (frag.y - startFrag.y) / deltaY
        , (frag.x - startFrag.x) / deltaX
        , useX
        );

    search1 = clamp(search1, 0.0, 1.0);

    viewDistance = (startView.y * endView.y) / mix(endView.y, startView.y, search1);
    depth        = viewDistance - positionTo.y;

    if (depth > 0 && depth < thickness) {
      hit0 = 1;
      break;
    } else {
      search0 = search1;
    }
  }

  search1 = search0 + ((search1 - search0) / 2.0);

  steps *= hit0;

  for (i = 0; i < steps; ++i) {
    frag       = mix(startFrag.xy, endFrag.xy, search1);
    uv.xy      = frag / texSize;
    positionTo = texture(positionTexture, uv.xy);

    viewDistance = (startView.y * endView.y) / mix(endView.y, startView.y, search1);
    depth        = viewDistance - positionTo.y;

    if (depth > 0 && depth < thickness) {
      hit1 = 1;
      search1 = search0 + ((search1 - search0) / 2);
    } else {
      float temp = search1;
      search1 = search1 + ((search1 - search0) / 2);
      search0 = temp;
    }
  }

  float visibility =
      hit1
    * positionTo.w
    * ( 1 - max ( dot(-unitPositionFrom, pivot), 0))
    * ( 1 - clamp( depth / thickness, 0, 1))
    * ( 1 - clamp( length(positionTo - positionFrom) / maxDistance, 0, 1))
    * (uv.x < 0 || uv.x > 1 ? 0 : 1)
    * (uv.y < 0 || uv.y > 1 ? 0 : 1);

  visibility = clamp(visibility, 0, 1);

  uv.ba = vec2(visibility);

  fragColor = uv;
}

```


在这段描述中，主要是计算当前片段在直线上所占的百分比或部分。如果useX为零，则使用直线的Y维度。如果useX为一，则使用直线的X维度。

当frag等于startFrag时，search1等于零，因为frag - startFrag为零。当frag等于endFrag时，search1等于一，因为frag - startFrag等于delta。

search1表示当前位置在直线上所代表的百分比或部分。您将需要使用这个百分比在相机的视空间中在射线的起始和终止距离之间进行插值。

使用search1，在反射射线上的当前位置进行视距插值（即相机在视空间中到当前位置的距离）。

您可能会尝试简单地在起始和结束的视空间位置之间进行视距插值，但这样会导致反射射线上当前位置的视距不正确。相反，您需要进行透视校正插值，如下所示。

计算在这一点上射线的视距与场景在这一点上采样视距之间的差异。

如果差异在零和厚度之间，那么这是一个击中（hit）。将hit0设置为1并退出第一遍扫描。如果差异不在零和厚度之间，那么这是一个未击中（miss）。将search0设置为等于search1，以记住这个位置作为最后已知的未击中位置。继续沿着射线朝结束片段前进。

现在您已经完成了第二次也是最后一次扫描，但在输出反射UV坐标之前，您需要计算反射的可见性（visibility）。可见性的范围从零到一。如果第二次扫描没有击中（hit），则可见性为零。

如果反射场景位置的alpha或w分量为零，则可见性为零。请注意，如果w为零，表示该点处没有场景位置。

![Reflection ray pointing towards the camera position.](D:\Games\VulkanDemo\doc\7e2cOdZ.gif)


屏幕空间反射可能失败的一种情况是反射射线指向相机的大致方向。如果反射射线指向相机并且击中了某个物体，很可能是击中了面向相机背面的物体。

为了处理这种失败情况，您需要根据反射向量与相机位置之间的夹角逐渐淡化反射效果。如果反射向量指向位置向量的完全相反方向，则可见性为零。其他任何方向都会导致可见性大于零。

在计算点乘时，请记得对两个向量进行归一化。unitPositionFrom是经过归一化的位置向量，它的长度或大小为一。

在对反射射线进行采样时，您希望找到反射射线与场景几何体首次相交的确切点。然而，您可能找不到这个特定的点。在找到的相交点附近，逐渐淡化反射效果。

根据反射点距离初始起始点的远近逐渐淡化反射效果。这样可以让反射逐渐变淡，而不是在达到最大距离时突然结束。