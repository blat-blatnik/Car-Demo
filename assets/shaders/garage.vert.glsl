#version 430

layout(location=0) in vec3 pos;
layout(location=1) in vec3 normal;
layout(location=2) in vec3 tangent;
layout(location=3) in vec2 uv;
layout(location=4) in vec4 color;

out vec3 vertPos;
out vec3 vertNormal;
out vec3 vertTangent;
out vec3 vertTexDir;
out vec4 vertColor;

layout(location=0) uniform mat4 Model;
layout(location=1) uniform mat4 MVP;

void main() {
	gl_Position = MVP * vec4(pos, 1);

	mat3 m = mat3(Model);
	vertPos = (Model * vec4(pos, 1)).xyz;
	vertNormal = normalize(m * normal);
	vertTangent = normalize(m * tangent);
	vertTexDir = pos;
	vertColor = color;
}