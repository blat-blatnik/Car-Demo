#version 430

layout(location=0) in vec3 pos;

out vec3 vertPos;

layout(location=0) uniform mat4 Model;
layout(location=1) uniform mat4 MVP;

void main() {
	vertPos = (Model * vec4(pos, 1)).xyz;
	gl_Position = MVP * vec4(pos, 1);
}