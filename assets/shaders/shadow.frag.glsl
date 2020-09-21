#version 430

in vec3 vertPos;

layout(location=20) uniform vec3 LightPos;
layout(location=21) uniform float FarPlane;

void main() {
	float dist = distance(vertPos, LightPos) / FarPlane;
	gl_FragDepth = dist;
} 