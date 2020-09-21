#version 430

in vec3 vertPos;
in vec3 vertNormal;
in vec3 vertTangent;
in vec2 vertUV;
in vec4 vertColor;

out vec4 fragColor;

layout(location=4) uniform vec3 CameraPos;
layout(location=5) uniform vec3 CameraDir;
layout(location=6) uniform vec3 AmbientColor;
layout(location=7) uniform vec3 DiffuseColor;
layout(location=8) uniform vec3 SpecularColor;
layout(location=9) uniform float SpecularExponent;
layout(location=11) uniform bool RenderNormals;
layout(location=12) uniform samplerCube GarageDiffuse;
layout(location=13) uniform float Reflectivity;
layout(location=14) uniform samplerCubeShadow ShadowMap;
layout(location=20) uniform vec3 LightPos;
layout(location=21) uniform float FarPlane;

const float ShadowBias = 0.05;
const float ShadowSampleDist = 0.02;
const vec3 ShadowSampleOffsets[20] = vec3[](
	vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
	vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
	vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
	vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
	vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1));

vec3 calcPointLight(vec3 normal, vec3 diffuseColor) {
	vec3 lightToFrag = vertPos - LightPos;
	float distToLight = length(lightToFrag);
	float shadowRef = (distToLight - ShadowBias) / FarPlane;

	float lightFactor = 0;
	for (int i = 0; i < ShadowSampleOffsets.length(); ++i) {
		lightFactor += texture(ShadowMap, vec4(lightToFrag + ShadowSampleDist * ShadowSampleOffsets[i], shadowRef)).r;
	}
	
	lightFactor /= ShadowSampleOffsets.length();
	lightFactor *= 20 / (1 + 0.25 * distToLight * distToLight);

	vec3 lightD = normalize(LightPos - vertPos);
	vec3 cameraD = normalize(CameraPos - vertPos);
	vec3 halfwayLightCamera = normalize(lightD + cameraD);

	float diffuse = max(0, dot(normal, lightD));
	float specular = pow(max(0, dot(normal, halfwayLightCamera)), SpecularExponent);

	return lightFactor * (diffuse * DiffuseColor + specular * SpecularColor);	
}

vec3 calcSpotlight(vec3 normal, vec3 diffuseColor) {
	vec3 spotlight = vec3(0);
	
	vec3 fragToLight = CameraPos - vertPos;
	float distToLight = length(fragToLight);
	vec3 lightD = fragToLight / distToLight;
	float cosTheta = dot(lightD, -normalize(CameraDir));

	const float outerCutoff = cos(radians(17.5));
	const float innerCutoff = cos(radians(12.5));

	float intensity = min((cosTheta - outerCutoff) / (innerCutoff - outerCutoff), 1);
	if (intensity > 0) {
		vec3 cameraD = normalize(CameraPos - vertPos);
		vec3 halfwayLightCamera = normalize(lightD + cameraD);
		float diffuse = max(0, dot(normal, lightD));
		float specular = pow(max(0, dot(normal, halfwayLightCamera)), 32);
		float attenutation = 1 + 0.25 * distToLight * distToLight;
		spotlight = 20 * intensity * (diffuse * diffuseColor + specular * vec3(1)) / attenutation;
	}

	return spotlight;
}

void main() {

	vec3 normal = normalize(vertNormal);
	
	if (RenderNormals)
		fragColor = vec4(0.5 * (1 + normal), 1);
	else
	{
		vec3 cameraD = normalize(CameraPos - vertPos);
		vec3 lighting = calcPointLight(normal, DiffuseColor);
		lighting += calcSpotlight(normal, DiffuseColor);

		vec3 reflectDir = reflect(-cameraD, normal);
		vec3 reflection = texture(GarageDiffuse, reflectDir).rgb;

		vec3 color = mix(lighting, reflection, Reflectivity);
		fragColor = vec4(color, 1);
	}
}