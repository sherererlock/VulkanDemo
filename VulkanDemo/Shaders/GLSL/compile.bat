@REM glslangValidator.exe -V Shaders\GLSL\shader.vert	-o Shaders\GLSL\spv\shader.vert.spv
@REM glslangValidator.exe -V Shaders\GLSL\shader.frag -o Shaders\GLSL\spv\shader.frag.spv
@REM glslangValidator.exe -V Shaders\GLSL\shader_rsm.frag -o Shaders\GLSL\spv\shader_rsm.frag.spv
@REM glslangValidator.exe -V Shaders\GLSL\shader_ibl.frag -o Shaders\GLSL\spv\shader_ibl.frag.spv

@REM glslangValidator.exe -V Shaders\GLSL\shaderCascaded.vert	-o Shaders\GLSL\spv\shaderCascaded.vert.spv
@REM glslangValidator.exe -V Shaders\GLSL\shaderCascaded.frag -o Shaders\GLSL\spv\shaderCascaded.frag.spv

@REM glslangValidator.exe -V Shaders\GLSL\shadow.vert -o Shaders\GLSL\spv\shadow.vert.spv
@REM glslangValidator.exe -V Shaders\GLSL\shadow.frag -o Shaders\GLSL\spv\shadow.frag.spv

@REM glslangValidator.exe -V Shaders\GLSL\quad.vert -o Shaders\GLSL\spv\quad.vert.spv
@REM glslangValidator.exe -V Shaders\GLSL\quad.frag -o Shaders\GLSL\spv\quad.frag.spv

@REM glslangValidator.exe -V Shaders\GLSL\skybox.vert -o Shaders\GLSL\spv\skybox.vert.spv
@REM glslangValidator.exe -V Shaders\GLSL\skybox.frag -o Shaders\GLSL\spv\skybox.frag.spv

@REM glslangValidator.exe -V Shaders\GLSL\filtercube.vert -o Shaders\GLSL\spv\filtercube.vert.spv

@REM glslangValidator.exe -V Shaders\GLSL\irradiancecube.frag -o Shaders\GLSL\spv\irradiancecube.frag.spv
@REM glslangValidator.exe -V Shaders\GLSL\prefilterLight.frag -o Shaders\GLSL\spv\prefilterLight.frag.spvd

@REM glslangValidator.exe -V Shaders\GLSL\genBRDFLut.vert -o Shaders\GLSL\spv\genBRDFLut.vert.spv
@REM glslangValidator.exe -V Shaders\GLSL\genBRDFLut.frag -o Shaders\GLSL\spv\genBRDFLut.frag.spv

@REM glslangValidator.exe -V Shaders\GLSL\rsmgbuffer.vert -o Shaders\GLSL\spv\rsmgbuffer.vert.spv
@REM glslangValidator.exe -V Shaders\GLSL\rsmgbuffer.frag -o Shaders\GLSL\spv\rsmgbuffer.frag.spv

@REM glslangValidator.exe -V Shaders\GLSL\ssaogbuffer.vert -o Shaders\GLSL\spv\ssaogbuffer.vert.spv
@REM glslangValidator.exe -V Shaders\GLSL\ssaogbuffer.frag -o Shaders\GLSL\spv\ssaogbuffer.frag.spv
@REM glslangValidator.exe -V Shaders\GLSL\ssao.frag -o Shaders\GLSL\spv\ssao.frag.spv
@REM glslangValidator.exe -V Shaders\GLSL\shader_ssao.frag -o Shaders\GLSL\spv\shader_ssao.frag.spv

glslangValidator.exe -V Shaders\GLSL\ssrgbuffer.vert -o Shaders\GLSL\spv\ssrgbuffer.vert.spv
glslangValidator.exe -V Shaders\GLSL\ssrgbuffer.frag -o Shaders\GLSL\spv\ssrgbuffer.frag.spv
glslangValidator.exe -V Shaders\GLSL\ssr.frag -o Shaders\GLSL\spv\ssr.frag.spv

pause