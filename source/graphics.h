#pragma once

#include "common.h"
#include "lib/bmath.h"
#include "lib/glad.h"

#if !defined(NDEBUG) && !defined(NO_GL_CHECK_ERROR)
#define glClearErrors() do {} while (glGetError() != GL_NO_ERROR) 
#define glCheckErrors()\
	do {\
		GLenum error = glGetError();\
		if (error != GL_NO_ERROR) {\
			const char *errorStr =\
				error == GL_INVALID_ENUM ?      "GL_INVALID_ENUM" :\
				error == GL_INVALID_VALUE ?     "GL_INVALID_VALUE" :\
				error == GL_INVALID_OPERATION ? "GL_INVALID_OPERATION" :\
				error == GL_OUT_OF_MEMORY ?     "GL_OUT_OF_MEMORY" :\
				error == GL_STACK_UNDERFLOW ?   "GL_STACK_UNDERFLOW" :\
				error == GL_STACK_OVERFLOW ?    "GL_STACK_OVERFLOW" :\
				error == GL_INVALID_FRAMEBUFFER_OPERATION ? "GL_INVALID_FRAMEBUFFER_OPERATION" :\
				"unknown OpenGL error";\
			fprintf(stderr, "%s generated\n", errorStr);\
		}\
	} while(0)
#else
#define glClearErrors() do {} while(0)
#define glCheckErrors() do {} while(0)
#endif

constexpr float NearPlane = 0.01f;
constexpr float FarPlane = 100.0f;
constexpr int ReflectionMapResolution = 256;
constexpr int ShadowMapResolution = 512;

typedef GLuint Texture;
typedef GLuint CubeMap;
typedef GLuint GpuBuffer;
typedef GLuint ShaderProgram;
typedef GLuint VertexSpecification;
typedef GLuint Framebuffer;

struct Font // stores all visible ASCII characters
{
	Texture atlas;		// the texture in which all characters are packed
	int atlasWidth;		// width of the atlas in pixels
	int atlasHeight;	// height of the atlas in pixels
	int firstChar;		// the first character codepoint stored in the font
	int numChars;		// the number of characters stored in the font - sequential characters are stored
	float ascent;		// highest glyph position over the baseline in pixels
	float descent;		// lowest glyph position below the baseling in pixels (negative)
	float lineGap;		// extra distance between 2 lines in pixels
	float kerningScale; // scale factor for xKerning to convert it into fractional pixels
	uint16_t x[96];     // [numChars] U coordinate of each character in the atlas - divide by atlasWidth before use!
	uint16_t y[96];     // [numChars] V coordinate of each character in the atlas - divide by atlasHeight before use!
	uint16_t w[96];     // [numChars] width of each character in the atlas in pixels
	uint16_t h[96];     // [numChars] height of each character in the atlas in pixels
	float xOffset[96];  // [numChars] x-position relative to cursor where to begin drawing each character
	float yOffset[96];  // [numChars] y-position above the baseline where to begin drawing each character

	// for monospaced fonts:
	// - xAdvance will always be the same value.
	// - xKerning will always be 0 for any pair.
	float xAdvance[96];       // [numChars] how many fractional pixels to advance the cursor after each character
	int16_t xKerning[96][96]; // [numChars * numChars] additional spacing for each character PAIR - multiply by kerningScale before use!
};

struct Vertex
{
	vec3 pos     = vec3(0);
	vec3 normal  = vec3(0);
	vec3 tangent = vec3(0);
	vec2 uv      = vec2(0);
	vec4 color   = vec4(1);
};

struct Transform
{
	vec3 pos      = vec3(0);
	vec3 scale    = vec3(1);
	quat rotation = quat(0, 0, 0, 1);
	Transform *parent = NULL;

	constexpr inline mat4 getLocalMatrix() const
	{
		return translationMat(pos) * quatToMat(rotation) * scaleMat(scale);
	}

	constexpr inline mat4 getMatrix() const
	{
		mat4 matrix = getLocalMatrix();
		
		if (parent != NULL)
			return parent->getMatrix() * matrix;
		else
			return matrix;
	}

	inline void rotate(vec3 axis, float angleRadians)
	{
		rotation = rotationQuat(axis, angleRadians) * rotation;
	}
};

struct Mesh
{
	VertexSpecification vertexSpecification;
	GpuBuffer vertexBuffer;
	GpuBuffer indexBuffer;
	int numVertices;
	int numIndices;
};

struct Material
{
	vec3 ambientColor      = vec3(0);
	vec3 diffuseColor      = vec3(1);
	vec3 specularColor     = vec3(1);
	float specularExponent = 1;
	float alpha            = 1;
};

struct Model
{
	Mesh mesh;
	Material material;
	Transform transform;
	vec3 minAABB;
	vec3 maxAABB;
};

struct CompositeModel
{
	Transform transform;
	int numMaterials;
	Material *materials;
	int numModels;
	Mesh *meshes;
	int *materialIndices;
	Transform *localTransforms;
	vec3 minAABB;
	vec3 maxAABB;
	vec3 *minAABBs;
	vec3 *maxAABBs;

	inline Material &getMaterial(int modelIndex)
	{
		return materials[materialIndices[modelIndex]];
	}

	inline const Material &getMaterial(int modelIndex) const
	{
		return materials[materialIndices[modelIndex]];
	}

	inline Model getModel(int modelIndex) const
	{
		Model model;
		model.mesh = meshes[modelIndex];
		model.transform = localTransforms[modelIndex];
		model.material = getMaterial(modelIndex);
		model.minAABB = minAABBs[modelIndex];
		model.maxAABB = maxAABBs[modelIndex];
		return model;
	}

	inline vec3 getCenter() const
	{
		return (transform.getMatrix() * vec4(0.5f * (minAABB + maxAABB), 1)).xyz;
	}
};

struct LightProbe
{
	CubeMap colorMap  = 0;
	CubeMap depthMap  = 0;
	
	union
	{
		// same order as GL_CUBE_MAP_POSITIVE_X + 0..5
		Framebuffer framebuffers[6] = { 0, 0, 0, 0, 0, 0 };
		struct
		{
			Framebuffer right;
			Framebuffer left;
			Framebuffer up;
			Framebuffer down;
			Framebuffer front;
			Framebuffer back;
		};
	};
};

struct Spotlight
{
	vec3 pos   = vec3(0);
	vec3 dir   = vec3(1, 0, 0);
	vec3 up    = vec3(0, 1, 0); // this just needs to be something perpendicular to the direction
	vec3 color = vec3(1);
	float constantAttenuation  = 1;
	float linearAttenutation   = 0;
	float quadraticAttenuation = 1;
	float innerCutoffRadians   = Pi / 4;
	float outerCutoffRadians   = Pi / 4;

	Texture shadowMapTex    = 0;
	Framebuffer shadowMapFb = 0;
};

CompositeModel *loadModel(const char *filename);
CompositeModel *loadModelObj(const char *objFilename);
void convertObjToModel(const char *objFilename, const char *outFilename);
CompositeModel *copyModel(const CompositeModel *model);

Model createModel(const Vertex *vertices, int numVertices, const uint *indices, int numIndices);

void drawMesh(Mesh mesh);

Mesh createMesh(
	const Vertex *vertices, 
	int numVertices, 
	const uint *indices, 
	int numIndices);

Mesh createMesh(
	GpuBuffer vertexBuffer,
	int numVertices,
	const uint *indices,
	int numIndices);

void drawString(
	const Font *font,
	const char *string,
	vec2 position,
	bool center = false,
	vec2 scale = vec2(1),
	vec4 color = vec4(1),
	float rotationRadians = 0);

Font *loadFont(const char *filename, float sizeInPixels);

Texture loadTexture(
	const char *filename,
	GLenum internalFormat = GL_RGBA,
	GLenum minFilter = GL_LINEAR,
	GLenum magFilter = GL_LINEAR,
	GLenum wrapS = GL_CLAMP_TO_EDGE,
	GLenum wrapT = GL_CLAMP_TO_EDGE,
	bool genMipmaps = true);

Texture createTexture(
	const void *pixels,
	int width,
	int height,
	GLenum pixelFormat = GL_RGBA,
	GLenum internalFormat = GL_RGBA,
	GLenum minFilter = GL_LINEAR,
	GLenum magFilter = GL_LINEAR,
	GLenum wrapS = GL_CLAMP_TO_EDGE,
	GLenum wrapT = GL_CLAMP_TO_EDGE,
	bool genMipmaps = true);

CubeMap loadCubeMap(
	const char *leftFilename,
	const char *rightFilename,
	const char *upFilename,
	const char *downFilename,
	const char *frontFilename,
	const char *backFilename,
	GLenum internalFormat = GL_RGBA,
	GLenum minFilter = GL_LINEAR,
	GLenum magFilter = GL_LINEAR,
	bool genMipmaps = true);

CubeMap createCubeMap(
	const void *leftPixels,
	const void *rightPixels,
	const void *upPixels,
	const void *downPixels,
	const void *frontPixels,
	const void *backPixels,
	int leftWidth,
	int leftHeight,
	int rightWidth,
	int rightHeight,
	int upWidth,
	int upHeight,
	int downWidth,
	int downHeight,
	int frontWidth,
	int frontHeight,
	int backWidth,
	int backHeight,
	GLenum pixelFormat = GL_RGBA,
	GLenum internalFormat = GL_RGBA,
	GLenum minFilter = GL_LINEAR,
	GLenum magFilter = GL_LINEAR,
	bool genMipmaps = true);

inline CubeMap createCubeMap(
	const void *leftPixels,
	const void *rightPixels,
	const void *upPixels,
	const void *downPixels,
	const void *frontPixels,
	const void *backPixels,
	int width,
	int height,
	GLenum pixelFormat = GL_RGBA,
	GLenum internalFormat = GL_RGBA,
	GLenum minFilter = GL_LINEAR,
	GLenum magFilter = GL_LINEAR,
	bool genMipmaps = true)
{
	return createCubeMap(
		leftPixels,
		rightPixels,
		upPixels,
		downPixels,
		frontPixels,
		backPixels,
		width, height,
		width, height,
		width, height,
		width, height,
		width, height,
		width, height,
		pixelFormat,
		internalFormat,
		minFilter,
		magFilter,
		genMipmaps);
}

inline CubeMap createCubeMap(
	int width,
	int height,
	GLenum internalFormat = GL_RGBA,
	GLenum minFilter = GL_LINEAR,
	GLenum magFilter = GL_LINEAR)
{
	return createCubeMap(
		NULL, NULL, NULL, NULL, NULL, NULL,
		width, height,
		GL_RGBA,
		internalFormat,
		minFilter,
		magFilter);
}

LightProbe createReflectionProbe(
	int faceWidth,
	int faceHeight,
	GLenum internalFormat);

LightProbe createShadowProbe(
	int faceWidth,
	int faceHeight);

Spotlight createSpotlight(
	vec3 pos,
	vec3 dir,
	vec3 color = vec3(1),
	int shadowMapResolution=ShadowMapResolution);

Framebuffer createFramebuffer(Texture colorAttachment = 0, Texture depthAttachment = 0);

ShaderProgram loadShaderProgram(const char *vertFilename, const char *fragFilename);

ShaderProgram createShaderProgram(
	const GLenum *types,
	const char *const *sources,
	int numSources);

GpuBuffer createGpuBuffer(
	const void *data,
	size_t numBytes,
	GLenum usage = GL_STATIC_DRAW);

bool shaderIsValid(ShaderProgram program);

inline void setUniform(GLint location, vec3 value)
{
	glUniform3f(location, value.x, value.y, value.z);
}
inline void setUniform(GLint location, mat4 value)
{
	glUniformMatrix4fv(location, 1, GL_FALSE, (GLfloat *)&value);
}
inline void bindUniformTexture(GLint location, GLint index, GLuint texture)
{
	glUniform1i(location, index);
	glActiveTexture(GLenum(GL_TEXTURE0 + index));
	glBindTexture(GL_TEXTURE_2D, texture);
}
inline void bindUniformCubeMap(GLint location, GLint index, CubeMap cubeMap)
{
	glUniform1i(location, index);
	glActiveTexture(GLenum(GL_TEXTURE0 + index));
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
}

inline constexpr vec4 rgb(float r, float g, float b)
{
	return vec4(r, g, b, 1);
}
inline constexpr vec4 gray(float intensity)
{
	return rgb(intensity, intensity, intensity);
}

// TODO: This doesn't work for all cases! 
// If the clip box is enclosed inside the bounding box this will incorrectly cull.
// But hey, it works for now :)
inline bool frustumCullAABB(vec3 aabbMin, vec3 aabbMax, mat4 mvpMatrix)
{
	vec3 points[8] = {
		vec3(aabbMin.x, aabbMin.y, aabbMin.z),
		vec3(aabbMin.x, aabbMin.y, aabbMax.z),
		vec3(aabbMin.x, aabbMax.y, aabbMin.z),
		vec3(aabbMin.x, aabbMax.y, aabbMax.z),
		vec3(aabbMax.x, aabbMin.y, aabbMin.z),
		vec3(aabbMax.x, aabbMin.y, aabbMax.z),
		vec3(aabbMax.x, aabbMax.y, aabbMin.z),
		vec3(aabbMax.x, aabbMax.y, aabbMax.z),
	};

	for (int i = 0; i < 8; ++i)
	{
		vec4 clip = mvpMatrix * vec4(points[i], 1);
		vec3 p = clip.xyz / clip.w;

		if (p.x >= -1 && p.x <= +1 &&
			p.y >= -1 && p.y <= +1 &&
			p.z >= -1 && p.z <= +1)
		{
			return false;
		}
	}

	return true;
}