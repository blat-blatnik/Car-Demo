#version 430

layout(location=0) in vec2 pos;
layout(location=1) in vec2 uv;
layout(location=2) in vec4 color;

out vec2 vertUV;
out vec4 vertColor;

layout(location=0) uniform mat4 MVP;

void main() {
	gl_Position = MVP * vec4(pos, 0, 1);
	vertUV = uv;
	vertColor = color;
}