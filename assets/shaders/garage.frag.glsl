#version 430

in vec3 vertPos;
in vec3 vertNormal;
in vec3 vertTangent;
in vec3 vertTexDir;
in vec4 vertColor;

out vec4 fragColor;

struct Spotlight {
	vec3 pos;
	vec3 dir;
	vec3 color;
	float cosOuterCutoff;
	float cosInnerCutoff;
};

layout(location=4) uniform vec3 CameraPos;
layout(location=5) uniform vec3 CameraDir;
layout(location=6) uniform vec3 AmbientColor;
layout(location=11) uniform bool RenderNormals;
layout(location=12) uniform samplerCube DiffuseMap;
layout(location=13) uniform samplerCube NormalMap;
layout(location=14) uniform samplerCubeShadow ShadowMap;
layout(location=15) uniform samplerCube DisplacementMap;
layout(location=20) uniform vec3 LightPos;
layout(location=21) uniform float FarPlane;
layout(location=22) uniform bool DoSpotlight;
layout(location=23) uniform bool DoGammaCorrection;

//TODO
//layout(location=30) uniform Spotlight StageLights[2];
//layout(location=31) uniform sampler2DShadow SpotlightShadowMaps[2];

//TODO: some shadow acne is still visible
const float SpecularExponent = 8;
const float ShadowBias = 0.05;
const float ShadowSampleDist = 0.05;
const vec3 ShadowSampleOffsets[20] = vec3[](
	vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
	vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
	vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
	vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
	vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1));

vec3 calcPointLight(vec3 normal, vec3 diffuseColor) {
	vec3 lightD = normalize(LightPos - vertPos);
	vec3 cameraD = normalize(CameraPos - vertPos);
	vec3 halfwayLightCamera = normalize(lightD + cameraD);

	float diffuse = max(0, dot(normal, lightD));
	float specular = pow(max(0, dot(normal, halfwayLightCamera)), SpecularExponent);

	vec3 lightToFrag = vertPos - LightPos;
	float distToLight = length(lightToFrag);
	float shadowRef = (distToLight - ShadowBias) / FarPlane;
			
	// 
	// NOTE: this is much faster but visibly worse quality
	//
	//float lightFactor = texture(shadowMap, vec4(lightToFrag, shadowRef)).r;
	//if (lightFactor < 1 && lightFactor > 0) {
	//	for (int i = 0; i < shadowSampleOffsets.length; ++i) {
	//		lightFactor += texture(shadowMap, vec4(lightToFrag + shadowSampleDist * shadowSampleOffsets[i], shadowRef)).r;
	//	}
	//	lightFactor /= (1 + shadowSampleOffsets.length);
	//}
	float lightFactor = 0;
	for (int i = 0; i < ShadowSampleOffsets.length(); ++i) {
		vec4 shadowCoords = vec4(lightToFrag + ShadowSampleDist * ShadowSampleOffsets[i], shadowRef);
		lightFactor += texture(ShadowMap, shadowCoords).r;
	}

	float attenutation = 1 + distToLight * distToLight;

	lightFactor *= 10 / attenutation / ShadowSampleOffsets.length();
	return lightFactor * (diffuse * diffuseColor + specular * vec3(1));
}

vec3 calcLighting(Spotlight spotlight, vec3 normal, vec3 diffuseColor) {
	vec3 lighting = vec3(0);
	
	vec3 fragToLight = spotlight.pos - vertPos;
	float distToLight = length(fragToLight);
	vec3 lightD = fragToLight / distToLight;
	float cosTheta = dot(lightD, -spotlight.dir);

	float cosOuterCutoff = spotlight.cosOuterCutoff;
	float cosInnerCutoff = spotlight.cosInnerCutoff;

	float intensity = min((cosTheta - cosOuterCutoff) / (cosInnerCutoff - cosOuterCutoff), 1);
	if (intensity > 0) {
		vec3 cameraD = normalize(CameraPos - vertPos);
		vec3 halfwayLightCamera = normalize(lightD + cameraD);
		float diffuse = max(0, dot(normal, lightD));
		float specular = pow(max(0, dot(normal, halfwayLightCamera)), SpecularExponent);
		float attenutation = 1 + 0.25 * distToLight * distToLight;
		lighting = spotlight.color * intensity * (diffuse * diffuseColor + specular * vec3(1)) / attenutation;
	}

	return lighting;
}

void main() {
	vec3 normal = normalize(vertNormal);
	vec3 tangent = normalize(vertTangent);
	tangent = normalize(tangent - dot(tangent, normal) * normal);
	vec3 bitangent = cross(tangent, normal);

	mat3 tbn = mat3(tangent, bitangent, normal);
	normal = texture(NormalMap, vertTexDir).rgb * 2 - 1;
	normal = normalize(tbn * normal);

	if (RenderNormals)
		fragColor = vec4(0.5 * (1 + normal), 1);
	else
	{
		vec3 diffuseColor = texture(DiffuseMap, vertTexDir).rgb;

		vec3 lighting = calcPointLight(normal, diffuseColor);
		Spotlight headlight1;
		headlight1.pos = vec3(-1.9891, 1.8, 4.8);
		headlight1.dir = normalize(vec3(-0.1, 0, 1));
		headlight1.color = vec3(20);
		headlight1.cosOuterCutoff = cos(radians(37.5));
		headlight1.cosInnerCutoff = cos(radians(12.5));
		
		Spotlight headlight2;
		headlight2.pos = vec3(+1.9891, 1.8, 4.8);
		headlight2.dir = normalize(vec3(+0.1, 0, 1));
		headlight2.color = vec3(20);
		headlight2.cosOuterCutoff = cos(radians(37.5));
		headlight2.cosInnerCutoff = cos(radians(12.5));
		
		lighting += calcLighting(headlight1, normal, diffuseColor);
		lighting += calcLighting(headlight2, normal, diffuseColor);
		//lighting += calcLighting(StageLights[0], normal, diffuseColor);
		//lighting += calcLighting(StageLights[1], normal, diffuseColor);
		
		vec3 gamma = vec3(DoGammaCorrection ? 1.0 / 2.2 : 1.0);
		fragColor = vec4(pow(vertColor.rgb * (AmbientColor + lighting), gamma), 1);
	}
}