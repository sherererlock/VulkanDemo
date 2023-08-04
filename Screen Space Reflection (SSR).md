## Screen Space Reflection (SSR)

添加反射可以真正使您的场景更加真实。湿润和有光泽的物体会栩栩如生，因为没有什么比反射更能使物体看起来湿润或有光泽。有了反射，您可以更好地展现水和金属物体的幻象。

在光照部分，您模拟了光源的反射、镜子般的图像。这个过程被称为渲染镜面反射。记得镜面光是通过计算反射光线的方向来获得的。类似地，使用屏幕空间反射（Screen Space Reflection，SSR），您可以模拟场景中其他物体的反射，而不仅限于光源。光线不再来自光源，然后反射进入相机，而是来自场景中的某个物体，然后反射进入相机。

SSR的工作原理是将屏幕图像通过自身反射到自身。这与立方体贴图（cube mapping）不同，后者使用六个屏幕或纹理。在立方体贴图中，您将光线从场景中的某一点反射到包围场景的立方体内的某一点。而在SSR中，您将光线从屏幕上的某一点反射到屏幕上的另一点。通过将屏幕映射到自身，您可以创造出反射的幻象。尽管这个幻象在大部分情况下都是有效的，但SSR在某些情况下可能会出现问题，正如您将看到的。

### Ray Marching

屏幕空间反射使用一种称为"射线步进"的技术来确定每个片段的反射效果。射线步进是一种通过迭代延长或缩短某个向量的长度或大小，以便在某个空间中探测或采样信息的过程。在屏幕空间反射中，射线是关于法线的位置向量。

直观地说，光线在场景中某个点发生碰撞，然后反射，沿着反射位置向量的相反方向前进，再次反射，沿着位置向量的相反方向前进，并最终碰撞到相机镜头。这样，您就可以在当前片段中看到场景中某个点的颜色被反射出来。屏幕空间反射是沿着光线的路径进行反向追踪的过程。它试图找到光线反射的点，即与当前片段相交的点。在每次迭代中，算法沿着反射射线对场景的位置或深度进行采样，并询问每次光线是否与场景的几何体相交。如果有相交，那么该场景中的位置可能成为当前片段反射的候选位置。

理想情况下，能够准确地确定第一个交点的解析方法是存在的。这个第一个交点是唯一有效的反射点，可以用于当前片段的反射。但是，这种方法更像是战舰游戏（battleship）。您无法看到交点（如果有的话），因此您从反射射线的起点开始，在向反射方向前进的过程中报出坐标。每次报出后，您会得到一个回答，告诉您是否碰撞到了某些物体。如果碰撞到物体，您会尝试在该区域附近尝试其他点，希望找到精确的交点。

在这里，您可以看到射线步进被用于计算每个片段的反射点。顶点法线是明亮绿色箭头，位置向量是明亮蓝色箭头，而明亮红色向量则是通过视图空间进行射线步进的反射射线。

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

resolution参数控制在第一次通过时在沿着反射射线的方向上跳过多少片段。这第一次通过是为了找到射线方向上的一个点，该点在场景中进入或穿过一些几何体。可以将这第一次通过视为粗略的过程。注意，resolution的取值范围是从零到一。零将导致没有反射，而一将在反射射线的方向上逐个片段地进行跟踪。resolution为一可能会明显降低您的帧率，尤其在maxDistance较大时。

steps参数控制在第二次通过时进行多少次迭代。这第二次通过是为了找到沿着反射射线的方向，射线立即与场景中的一些几何体相交的确切点。可以将这第二次通过视为细化的过程。

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

第一次通过将从反射射线的起始片段位置开始。通过将片段位置的坐标除以位置纹理的尺寸，将片段位置转换为UV坐标。

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

useX要么是1，要么是0。它用于根据哪个差异更大来选择X或Y维度。delta是两个X和Y差异中较大的那个。它用于确定每次迭代时在每个维度上前进多少，并在第一次通过期间需要多少次迭代。

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

search0用于记住射线未命中或未与任何几何体相交的线上的上一个位置。在第二次通过中，算法将使用search0来帮助细化射线与场景几何体接触的点。

hit0指示在第一次通过中有交点（即射线在第一次通过中与场景发生了相交）。hit1指示在第二次通过中有交点（即射线在第二次通过中与场景发生了相交）。

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

现在您可以开始第一次通过。第一次通过在i小于delta值时运行。当i等于delta时，算法已经沿着整条线行进了。请记住，delta是X和Y两个差异中较大的那个。