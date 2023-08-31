## Screen Space Ambient Occlusion (SSAO)


SSAO是那些你以前不知道需要却一旦拥有就无法离开的效果之一。它可以将一个场景从平庸变为惊艳！对于相对静态的场景，你可以将环境遮挡烘焙到纹理中，但对于更动态的场景，你将需要一个着色器。SSAO是较为复杂的着色技术之一，但一旦成功实现，你会感觉自己成了着色器大师。

通过仅使用少量纹理，SSAO可以近似计算场景的环境遮挡。这比尝试通过遍历整个场景的几何图形来计算环境遮挡要快。这些少量纹理都源自屏幕空间，因此称为屏幕空间环境遮挡。

### Inputs


SSAO着色器将需要以下输入：

1. 视图空间中的顶点位置向量。
2. 视图空间中的顶点法线向量。
3. 切线空间中的采样向量。
4. 切线空间中的噪声向量。
5. 相机镜头的投影矩阵。

### Vertex Positions

![Panda3D Vertex Positions](https://i.imgur.com/gr7IxKv.png)

将顶点位置存储到帧缓冲纹理中并不是必需的。您可以从相机的深度缓冲重新创建它们。由于这是初学者指南，我将避免这种优化，保持简单明了。不过，如果您的实现需要，也可以随意使用深度缓冲。

如果您决定使用深度缓冲，以下是如何在Panda3D中进行设置的示例：

```
PT(Texture) depthTexture =
  new Texture("depthTexture");
depthTexture->set_format
  ( Texture::Format::F_depth_component32
  );

PT(GraphicsOutput) depthBuffer =
  graphicsOutput->make_texture_buffer
    ( "depthBuffer"
    , 0
    , 0
    , depthTexture
    );
depthBuffer->set_clear_color
  ( LVecBase4f(0, 0, 0, 0)
  );

NodePath depthCameraNP =
  window->make_camera();
DCAST(Camera, depthCameraNP.node())->set_lens
  ( window->get_camera(0)->get_lens()
  );
PT(DisplayRegion) depthBufferRegion =
  depthBuffer->make_display_region
    ( 0
    , 1
    , 0
    , 1
    );
depthBufferRegion->set_camera(depthCameraNP);
```

```
in vec4 vertexPosition;

out vec4 fragColor;

void main() {
  fragColor = vertexPosition;
}
```


这是用于将视图空间顶点位置渲染到帧缓冲纹理的简单着色器。更复杂的工作是设置帧缓冲纹理，使其接收的片段向量分量不会被截断为 [0, 1] 范围，并且每个分量都具有足够高的精度（足够多的位数）。例如，如果特定的插值顶点位置是 <-139.444444566, 0.00000034343, 2.5>，您不希望将其存储到纹理中变成 <0.0, 0.0, 1.0>。

```
  FrameBufferProperties fbp = FrameBufferProperties::get_default();

  // ...

  fbp.set_rgba_bits(32, 32, 32, 32);
  fbp.set_rgb_color(true);
  fbp.set_float_color(true);
```

这里是示例代码如何设置帧缓冲纹理以存储顶点位置。它希望每个红色、绿色、蓝色和透明度分量都有32位，并禁止将值夹紧到 [0, 1] 范围。`set_rgba_bits(32, 32, 32, 32)` 的调用设置了位数并且也禁止了夹紧。

```
  glTexImage2D
    ( GL_TEXTURE_2D
    , 0
    , GL_RGB32F
    , 1200
    , 900
    , 0
    , GL_RGB
    , GL_FLOAT
    , nullptr
    );
```

以下是等效的OpenGL调用。`GL_RGB32F`设置了位数并且也禁止了夹紧。

如果颜色缓冲区是定点数，对于无符号标准化或有符号标准化的颜色缓冲区，源值和目标值的分量以及混合因子都会在计算混合方程之前分别被夹紧到 [0, 1] 或 [−1, 1] 范围内。如果颜色缓冲区是浮点数，将不会进行夹紧。

![OpenGL Vertex Positions](https://i.imgur.com/V4nETME.png)


在这里，您可以看到顶点位置，其中 y 被视为上向量。

请注意，Panda3D将 z 设置为上向量，但OpenGL使用 y 作为上向量。由于Panda3D配置为使用默认的 gl-coordinate-system，位置着色器输出的顶点位置是将 z 作为上向量的。

### Vertex Normals

![Panda3d Vertex Normals](https://i.imgur.com/ilnbkzq.gif)

您将需要顶点法线来正确定向在SSAO着色器中采样的样本。示例代码生成了在半球中分布的多个采样向量，但您也可以使用球体，并且完全不需要法线。

```
in vec3 vertexNormal;

out vec4 fragColor;

void main() {
  vec3 normal = normalize(vertexNormal);

  fragColor = vec4(normal, 1);
}
```

就像位置着色器一样，法线着色器也很简单。请确保对顶点法线进行归一化，并记住它们处于视图空间中。

![OpenGL Vertex Normals](https://i.imgur.com/ucdx9Kp.gif)


在这里，您可以看到顶点法线，其中 y 被视为上向量。

请注意，Panda3D将 z 设置为上向量，但OpenGL使用 y 作为上向量。由于Panda3D配置为使用默认的 gl-coordinate-system，法线着色器输出的顶点法线是将 z 作为上向量的。

![SSAO using the normal maps.](https://i.imgur.com/fiHXBex.gif)

在这里，您可以看到使用法线贴图而不是顶点法线的SSAO。这增加了额外的细节级别，并且将与法线贴图的光照效果很好地配合。

```
    normal =
      normalize
        ( normalTex.rgb
        * 2.0
        - 1.0
        );
    normal =
      normalize
        ( mat3
            ( tangent
            , binormal
            , vertexNormal
            )
        * normal
        );

    // ...
```

要使用法线贴图，您需要将法线贴图中的法线从切线空间转换到视图空间，就像在光照计算中所做的那样。

### Samples

要确定任何特定片段的环境遮挡量，您需要对周围区域进行采样。使用的样本数量越多，近似的质量越好，但性能会受到影响。

```
  for (int i = 0; i < numberOfSamples; ++i) {
    LVecBase3f sample =
      LVecBase3f
        ( randomFloats(generator) * 2.0 - 1.0
        , randomFloats(generator) * 2.0 - 1.0
        , randomFloats(generator)
        ).normalized();

    float rand = randomFloats(generator);
    sample[0] *= rand;
    sample[1] *= rand;
    sample[2] *= rand;

    float scale = (float) i / (float) numberOfSamples;
    scale = lerp(0.1, 1.0, scale * scale);
    sample[0] *= scale;
    sample[1] *= scale;
    sample[2] *= scale;

    ssaoSamples.push_back(sample);
  }
```

示例代码生成了在半球中分布的多个随机样本。这些`ssaoSamples`将被发送到SSAO着色器中。

```
    LVecBase3f sample =
      LVecBase3f
        ( randomFloats(generator) * 2.0 - 1.0
        , randomFloats(generator) * 2.0 - 1.0
        , randomFloats(generator) * 2.0 - 1.0
        ).normalized();
```

如果您希望将样本分布在一个球体中，将随机z分量更改为范围从负一到正一。

### Noise

```
  for (int i = 0; i < numberOfNoise; ++i) {
    LVecBase3f noise =
      LVecBase3f
        ( randomFloats(generator) * 2.0 - 1.0
        , randomFloats(generator) * 2.0 - 1.0
        , 0.0
        );

    ssaoNoise.push_back(noise);
  }
```

为了获得对采样区域的很好覆盖，您需要生成一些噪声向量。这些噪声向量将会随机地使半球围绕当前片段倾斜。

### Ambient Occlusion

![SSAO Texture](https://i.imgur.com/KKt74VE.gif)

SSAO通过对片段周围的视图空间进行采样来工作。在一个表面下方的样本越多，片段的颜色越暗。这些样本位于片段位置，并指向顶点法线的大致方向。每个样本用于在位置帧缓冲纹理中查找一个位置。返回的位置与样本进行比较。如果样本距离摄像机的距离大于位置，则样本对片段被遮挡的计算有所贡献。

![SSAO Sampling](https://i.imgur.com/Nm4CJDN.gif)

在这里，您可以看到对表面上方的空间进行采样以获取遮挡信息。

```
  float radius    = 1;
  float bias      = 0.01;
  float magnitude = 1.5;
  float contrast  = 1.5;
```

与其他一些技术类似，SSAO着色器也有一些可以微调的控制参数，以实现您想要的确切效果。偏差会增加样本距离摄像机的距离。您可以使用偏差来对抗"粉刺"效应。半径增加或减少了样本空间的覆盖范围。幅度会使遮挡地图变亮或变暗。对比度会使遮挡地图的明暗程度增加或减少。

```
  // ...

  vec4 position =           texture(positionTexture, texCoord);
  vec3 normal   = normalize(texture(normalTexture,   texCoord).xyz);

  int  noiseX = int(gl_FragCoord.x - 0.5) % 4;
  int  noiseY = int(gl_FragCoord.y - 0.5) % 4;
  vec3 random = noise[noiseX + (noiseY * 4)];

  // ...
```

获取位置、法线和随机向量以备后用。请回忆一下，示例代码创建了一组预定数量的随机向量。随机向量是根据当前片段的屏幕位置选择的。

```
  // ...

  vec3 tangent  = normalize(random - normal * dot(random, normal));
  vec3 binormal = cross(normal, tangent);
  mat3 tbn      = mat3(tangent, binormal, normal);

  // ...
```

使用随机向量和法线向量，组装切线、副切线和法线矩阵。您将需要这个矩阵将样本向量从切线空间转换到视图空间。

```
  // ...

  float occlusion = NUM_SAMPLES;

  for (int i = 0; i < NUM_SAMPLES; ++i) {
    // ...
  }

  // ...
```

有了矩阵，着色器现在可以循环遍历样本，计算没有被遮挡的数量。

```
    vec3 samplePosition = tbn * samples[i];
         samplePosition = position.xyz + samplePosition * radius;
```

使用矩阵，将样本定位在顶点/片段位置附近，并将其按照半径进行缩放。

```
    vec4 offsetUV      = vec4(samplePosition, 1.0);
         offsetUV      = lensProjection * offsetUV;
         offsetUV.xyz /= offsetUV.w;
         offsetUV.xy   = offsetUV.xy * 0.5 + 0.5;
```

使用样本在视图空间中的位置，将其从视图空间转换到裁剪空间，然后再转换到UV空间。

```
-1 * 0.5 + 0.5 = 0
 1 * 0.5 + 0.5 = 1
```

请记住，裁剪空间的分量范围是从负一到正一，UV坐标的范围是从零到一。要将裁剪空间坐标转换为UV坐标，乘以一半并加上一半。

```
    vec4 offsetPosition = texture(positionTexture, offsetUV.xy);

    float occluded = 0;
    if (samplePosition.y + bias <= offsetPosition.y) { occluded = 0; } else { occluded = 1; }
```

使用通过将3D样本投影到2D位置纹理上创建的偏移UV坐标，找到相应的位置向量。这将使您从视图空间转换到裁剪空间再到UV空间，然后再回到视图空间。着色器采用这一往返过程来判断某些几何图形是在这个样本后面、在其位置上还是在其前面。如果样本在某些几何图形前面或在其位置上，这个样本不会计入片段的遮挡。如果样本在某些几何图形后面，这个样本会计入片段的遮挡。

```
  float intensity =
      smoothstep
        ( 0.0
        , 1.0
        ,   radius
          / abs(position.y - offsetPosition.y)
        );
    occluded *= intensity;

    occlusion -= occluded;
```

现在根据样本在半径内或半径外的距离加权。最后，从遮挡因子中减去这个样本，因为它假设在循环之前所有样本都被遮挡。

```
    occlusion /= NUM_SAMPLES;

    // ...

    fragColor = vec4(vec3(occlusion), position.a);
```

将遮挡的计数除以样本数，以将遮挡因子从 [0, NUM_SAMPLES] 缩放到 [0, 1]。零表示完全遮挡，一表示没有遮挡。现在将遮挡因子分配给片段的颜色，完成操作。

```
    fragColor = vec4(vec3(occlusion), position.a);
```

为了演示目的，示例代码将 alpha 通道设置为位置帧缓冲纹理的 alpha 通道，以避免遮挡背景。

### Blurring

![SSAO Blur Texture](https://i.imgur.com/QsqOhFR.gif)

SSAO帧缓冲纹理的噪声比较明显。您会希望对其进行模糊处理以消除噪声。可以参考模糊处理的部分。为了获得最佳结果，可以使用中值或Kuwahara滤波器来保留清晰的边缘。

### Ambient Color

```
  vec2 ssaoBlurTexSize  = textureSize(ssaoBlurTexture, 0).xy;
  vec2 ssaoBlurTexCoord = gl_FragCoord.xy / ssaoBlurTexSize;
  float ssao            = texture(ssaoBlurTexture, ssaoBlurTexCoord).r;

  vec4 ambient = p3d_Material.ambient * p3d_LightModel.ambient * diffuseTex * ssao;
```

SSAO的最后一步是回到光照计算中。在这里，您可以看到遮挡因子在SSAO帧缓冲纹理中进行查找，然后包含在环境光照计算中。