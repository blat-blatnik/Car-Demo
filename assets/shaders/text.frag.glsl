#version 430

in vec2 vertUV;
in vec4 vertColor;

out vec4 fragColor;

uniform sampler2D Atlas;

void main() {
	float alpha = texture(Atlas, vertUV).r;
	fragColor = vertColor * vec4(1.0, 1.0, 1.0, alpha);
}