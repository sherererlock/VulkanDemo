#version 450

layout (location = 0) out vec2 outUV;

void main() 
{
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUV * 2.0 - 1.0, 0.0, 1.0);
}

/* 
gl_VertexIndex 0
outUV(0, 0)
gl_Position(-1.0, -1.0, 0.0, 1.0)

gl_VertexIndex 1
outUV(1, 0)
gl_Position(1.0, -1.0, 0.0, 1.0)


gl_VertexIndex 2
outUV(0, 1)
gl_Position(-1.0, 1.0, 0.0, 1.0)

gl_VertexIndex 3
outUV(1, 1)
gl_Position(1.0, 1.0, 0.0, 1.0)

*/