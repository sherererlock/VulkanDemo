glslangValidator.exe -V Shaders\GLSL\shader.vert	-o Shaders\GLSL\spv\shader.vert.spv
glslangValidator.exe -V Shaders\GLSL\shader.frag -o Shaders\GLSL\spv\shader.frag.spv

glslangValidator.exe -V Shaders\GLSL\shaderCascaded.vert	-o Shaders\GLSL\spv\shaderCascaded.vert.spv
glslangValidator.exe -V Shaders\GLSL\shaderCascaded.frag -o Shaders\GLSL\spv\shaderCascaded.frag.spv

glslangValidator.exe -V Shaders\GLSL\shadow.vert -o Shaders\GLSL\spv\shadow.vert.spv
glslangValidator.exe -V Shaders\GLSL\shadow.frag -o Shaders\GLSL\spv\shadow.frag.spv

glslangValidator.exe -V Shaders\GLSL\quad.vert -o Shaders\GLSL\spv\quad.vert.spv
glslangValidator.exe -V Shaders\GLSL\quad.frag -o Shaders\GLSL\spv\quad.frag.spv
pause