#include "graphics.h"
#include "system.h"

#pragma warning(push)
#pragma warning(disable: 4365)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#define STBI_ASSERT(x) assert(x)
#include "lib/stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#define STBTT_assert(x) assert(x)
#define STBTT_malloc(x,u) malloc(x)
#define STBTT_free(x,u) free(x)
#include "lib/stb_truetype.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "lib/tiny_obj_loader.h"
#pragma warning(pop)
#include <vector>

static constexpr int getCharIndex(const Font *font, char character)
{
	if (character >= font->firstChar && character < font->firstChar + font->numChars)
		return character - font->firstChar;
	else
		return font->numChars;
}
static constexpr float getKerning(const Font *font, char char1, char char2)
{
	int index1 = getCharIndex(font, char1);
	int index2 = getCharIndex(font, char2);
	return font->kerningScale * font->xKerning[index1][index2];
}

static GLuint createShader(GLenum type, const char *source)
{
	GLuint shader = glCreateShader(type);
	if (shader)
	{
		glShaderSource(shader, 1, &source, NULL);
		glCompileShader(shader);

		GLint shaderOk;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderOk);
		if (!shaderOk)
		{
			GLint logLength;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
			char *log = (char *)malloc((size_t)logLength);
			glGetShaderInfoLog(shader, logLength, NULL, (GLchar *)log);
			fprintf(stderr, "GLSL error: %s\n", log);
			free(log);

			glDeleteShader(shader);
			shader = 0;
		}
	}
	else
		fprintf(stderr, "OpenGL failed to allocate a shader");

	glCheckErrors();
	return shader;
}
static vec2 getStringSize(const Font *font, const char *string)
{
	vec2 size = vec2(0);
	float x = 0;
	float y = 0;

	if (string != NULL && string[0] != 0)
	{
		float minx = +Inf, miny = +Inf;
		float maxx = -Inf, maxy = -Inf;

		//OPTIMIZATION: we really only need to calculate x0, y0 for the first character and x1, y1 for the last character
		for (int i = 0; string[i] != 0; ++i)
		{
			char c0 = string[i + 0];
			char c1 = string[i + 1];
			int index = getCharIndex(font, c0);

			float x0 = x + font->xOffset[index];
			float y0 = y + font->yOffset[index];
			float x1 = x0 + font->w[index];
			float y1 = y0 + font->h[index];

			minx = min(minx, x0);
			miny = min(miny, y0);
			maxx = max(maxx, x1);
			maxy = max(maxy, y1);

			x += font->xAdvance[index] + getKerning(font, string[i], string[i + 1]);
		}

		size.x = maxx - minx;
		size.y = maxy - miny;
	}

	return size;
}

void convertObjToModel(const char *objFilename, const char *outFilename)
{
	// see loadModel() for a description of the .model file format.

	struct V
	{
		vec3 pos = vec3(0);
		vec3 normal = vec3(0);

		inline constexpr bool operator ==(V other) const
		{
			return all(pos == other.pos) && all(normal == other.normal);
		}

		struct Hash
		{
			inline size_t operator()(V v) const
			{
				return hashBytes(&v, sizeof(V));
			}
		};
	};

	struct M
	{
		vec3 ambient = vec3(0);
		vec3 diffuse = vec3(1);
		vec3 specular = vec3(1);
		float specularExponent = 1;
		float alpha = 1;
	};

	struct O
	{
		uint materialIndex;
		std::vector<uint> vertexIndices;
	};

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string error;
	tinyobj::LoadObj(&attrib, &shapes, &materials, &error, objFilename, "assets/models/", true);

	std::vector<V> outVertices;
	std::vector<M> outMaterials;
	std::vector<O> outObjects;
	vec3 minAABB = vec3(+Inf);
	vec3 maxAABB = vec3(-Inf);

	std::unordered_map<V, uint, V::Hash> vertexMap;
	std::unordered_map<int, int> materialMap;

	for (const auto &shape : shapes)
	{
		std::unordered_map<int, O> materialToObjectMap;

		for (int i = 0; i < (int)shape.mesh.indices.size(); ++i)
		{
			const tinyobj::index_t &index = shape.mesh.indices[i];
			
			int matIdx = shape.mesh.material_ids[i / 3];
			if (matIdx < 0)
				matIdx = -1;

			if (materialMap.find(matIdx) == materialMap.end())
			{
				M material;

				if (matIdx >= 0)
				{
					material.ambient = vec3(
						materials[matIdx].ambient[0],
						materials[matIdx].ambient[1],
						materials[matIdx].ambient[2]);
					material.diffuse = vec3(
						materials[matIdx].diffuse[0],
						materials[matIdx].diffuse[1],
						materials[matIdx].diffuse[2]);
					material.specular = vec3(
						materials[matIdx].specular[0],
						materials[matIdx].specular[1],
						materials[matIdx].specular[2]);
					material.specularExponent = materials[matIdx].shininess;
					material.alpha = materials[matIdx].dissolve;
				}
				else
				{
					material.ambient = vec3(0);
					material.diffuse = vec3(1);
					material.specular = vec3(1);
					material.specularExponent = 1;
					material.alpha = 1;
				}

				materialMap[matIdx] = (int)outMaterials.size();
				outMaterials.push_back(material);
			}

			V v;

			if (index.vertex_index >= 0)
				v.pos = vec3(
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]);

			if (index.normal_index >= 0)
				v.normal = vec3(
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]);

			if (vertexMap.find(v) == vertexMap.end())
			{
				vertexMap[v] = (uint)outVertices.size();
				outVertices.push_back(v);
				minAABB = min(minAABB, v.pos);
				maxAABB = max(maxAABB, v.pos);
			}

			O &object = materialToObjectMap[matIdx];
			object.materialIndex = materialMap[matIdx];
			object.vertexIndices.push_back(vertexMap[v]);
		}

		for (const auto &matObj : materialToObjectMap)
		{
			outObjects.push_back(matObj.second);
		}
	}

	FILE *f = fopen(outFilename, "wb");
	uint flags = 3;
	uint numVertices = (uint)outVertices.size();
	uint numMaterials = (uint)outMaterials.size();
	uint numObjects = (uint)outObjects.size();

	fwrite(&flags, sizeof(flags), 1, f);
	fwrite(&numVertices, sizeof(numVertices), 1, f);
	fwrite(&numMaterials, sizeof(numMaterials), 1, f);
	fwrite(&numObjects, sizeof(numObjects), 1, f);
	fwrite(&minAABB, sizeof(minAABB), 1, f);
	fwrite(&maxAABB, sizeof(maxAABB), 1, f);
	for (const auto &v : outVertices)
	{
		fwrite(&v.pos, sizeof(v.pos), 1, f);
		fwrite(&v.normal, sizeof(v.normal), 1, f);
	}
	for (const auto &m : outMaterials)
	{
		fwrite(&m.ambient, sizeof(m.ambient), 1, f);
		fwrite(&m.diffuse, sizeof(m.diffuse), 1, f);
		fwrite(&m.specular, sizeof(m.specular), 1, f);
		fwrite(&m.specularExponent, sizeof(m.specularExponent), 1, f);
		fwrite(&m.alpha, sizeof(m.alpha), 1, f);
	}
	for (const auto &o : outObjects)
	{
		vec3 minAABB = vec3(+Inf);
		vec3 maxAABB = vec3(-Inf);
		for (auto i : o.vertexIndices)
		{
			vec3 pos = outVertices[i].pos;
			minAABB = min(minAABB, pos);
			maxAABB = max(maxAABB, pos);
		}

		uint numIndices = (uint)o.vertexIndices.size();
		fwrite(&o.materialIndex, sizeof(o.materialIndex), 1, f);
		fwrite(&minAABB, sizeof(minAABB), 1, f);
		fwrite(&maxAABB, sizeof(maxAABB), 1, f);
		fwrite(&numIndices, sizeof(numIndices), 1, f);
		fwrite(&o.vertexIndices[0], sizeof(o.vertexIndices[0]), numIndices, f);
	}
	fclose(f);
}

CompositeModel *loadModel(const char *filename)
{
	/*
	.model custom file format
	This was made so that the car model can be loaded very quickly because .obj models were taking 10-20 sec in debug mode.
	These files have the following structure:

		uint     = flags
		uint     = number-of-vertices
		uint     = number-of-materials
		uint     = number-of-objects
		float[3] = minAABB
		float[3] = maxAABB
		
		for 1 ... number-of-vertices {
			if flags & HAS_POS
				float[3] = pos
			if flags & HAS_NORMAL
				float[3] = normal
			if flags & HAS_UV
				float[2] = uv
			if flags & HAS_COLOR
				float[4] = color
		}
		
		for 1 ... number-of-materials {
			float[3] = ambient
			float[3] = diffuse
			float[3] = specular
			float    = specular-exponent
			float    = alpha
		}
		
		for 1 ... number-of-objects {
			int      = material-index
			float[3] = minAABB
			float[3] = maxAABB
			uint     = number-of-indices
			for 1 ... number-of-indices {
				uint = vertex-index
			}
		}

	*/

	FILE *f = fopen(filename, "rb");
	if (f)
	{
		CompositeModel *model = (CompositeModel *)malloc(sizeof(CompositeModel));
		uint flags, numVertices, numMaterials, numObjects;
		fread(&flags, sizeof(flags), 1, f);
		fread(&numVertices, sizeof(numVertices), 1, f);
		fread(&numMaterials, sizeof(numMaterials), 1, f);
		fread(&numObjects, sizeof(numObjects), 1, f);
		fread(&model->minAABB, sizeof(model->minAABB), 1, f);
		fread(&model->maxAABB, sizeof(model->maxAABB), 1, f);

		model->numMaterials = (int)numMaterials;
		model->materials = (Material *)malloc(numMaterials * sizeof(Material));
		model->numModels = (int)numObjects;
		model->meshes = (Mesh *)malloc(numObjects * sizeof(Mesh));
		model->localTransforms = (Transform *)malloc(numObjects * sizeof(Transform));
		model->materialIndices = (int *)malloc(numObjects * sizeof(int));
		model->transform = Transform();
		model->minAABBs = (vec3 *)malloc(numObjects * sizeof(vec3));
		model->maxAABBs = (vec3 *)malloc(numObjects * sizeof(vec3));

		Vertex *vertices = (Vertex *)malloc(numVertices * sizeof(*vertices));
		for (uint i = 0; i < numVertices; ++i)
		{
			vec3 pos, normal;
			fread(&pos, sizeof(pos), 1, f);
			fread(&normal, sizeof(normal), 1, f);
			
			vertices[i].pos = pos;
			vertices[i].normal = normal;
			vertices[i].tangent = vec3(0);
			vertices[i].uv = vec2(0);
			vertices[i].color = vec4(1);
		}
		GpuBuffer vertexBuffer = createGpuBuffer(vertices, numVertices * sizeof(*vertices));
		free(vertices);

		for (uint i = 0; i < numMaterials; ++i)
		{
			vec3 ambient, diffuse, specular;
			float specularExponent, alpha;
			fread(&ambient, sizeof(ambient), 1, f);
			fread(&diffuse, sizeof(diffuse), 1, f);
			fread(&specular, sizeof(specular), 1, f);
			fread(&specularExponent, sizeof(specularExponent), 1, f);
			fread(&alpha, sizeof(alpha), 1, f);

			model->materials[i].ambientColor = ambient;
			model->materials[i].diffuseColor = diffuse;
			model->materials[i].specularColor = specular;
			model->materials[i].specularExponent = specularExponent;
			model->materials[i].alpha = alpha;
		}

		for (uint i = 0; i < numObjects; ++i)
		{
			uint materialIndex;
			uint numIndices;
			vec3 minAABB, maxAABB;
			fread(&materialIndex, sizeof(materialIndex), 1, f);
			fread(&minAABB, sizeof(minAABB), 1, f);
			fread(&maxAABB, sizeof(maxAABB), 1, f);
			fread(&numIndices, sizeof(numIndices), 1, f);
			
			uint *indices = (uint *)malloc(numIndices * sizeof(indices));
			fread(indices, sizeof(*indices), numIndices, f);
			model->meshes[i] = createMesh(vertexBuffer, numVertices, indices, numIndices);
			model->materialIndices[i] = materialIndex;
			model->localTransforms[i] = Transform();
			model->localTransforms[i].parent = &model->transform;
			model->minAABBs[i] = minAABB;
			model->maxAABBs[i] = maxAABB;
			free(indices);
		}

		fclose(f);
		return model;
	}
	else
		return NULL;
}

CompositeModel *loadModelObj(const char *objFilename)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string error;
	bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &error, objFilename, "assets/models/", true);

	for (const auto &shape : shapes)
	{
		int matIdx = shape.mesh.material_ids[0];
		for (auto id : shape.mesh.material_ids)
			if (id != matIdx)
			{
				printf("mesh %s doesnt have the same materials\n", shape.name.c_str());
				break;
			}
	}
	
	if (error.length() > 0)
		fprintf(stderr, "tinyobj error '%s'\n", error.c_str());

	if (success)
	{
		CompositeModel *model = (CompositeModel *)malloc(sizeof(CompositeModel));
		model->numMaterials = (int)shapes.size();
		model->numModels = (int)shapes.size();
		model->meshes = (Mesh *)malloc(shapes.size() * sizeof(Mesh));
		model->materialIndices = (int *)malloc(shapes.size() * sizeof(int));
		model->localTransforms = (Transform *)malloc(shapes.size() * sizeof(Transform));
		model->materials = (Material *)malloc(shapes.size() * sizeof(Material));
		model->transform = Transform();

		for (int meshIdx = 0; meshIdx < model->numModels; ++meshIdx)
		{
			model->localTransforms[meshIdx] = Transform();
			model->localTransforms[meshIdx].parent = &model->transform;
			
			tinyobj::mesh_t data = shapes[meshIdx].mesh;
			uint *indices = (uint *)malloc(data.indices.size() * sizeof(uint));
			Vertex *vertices = (Vertex *)malloc(data.indices.size() * sizeof(Vertex));

			Material material;
			if (data.material_ids[0] >= 0)
			{
				tinyobj::material_t materialData = materials[data.material_ids[0]];
				material.ambientColor = vec3(
					materialData.ambient[0], 
					materialData.ambient[1], 
					materialData.ambient[2]);
				material.diffuseColor = vec3(
					materialData.diffuse[0],
					materialData.diffuse[1],
					materialData.diffuse[2]);
				material.specularColor = vec3(
					materialData.specular[0],
					materialData.specular[1],
					materialData.specular[2]);
				material.specularExponent = materialData.shininess;
				material.alpha = materialData.dissolve;
			}
			model->materials[meshIdx] = material;
			model->materialIndices[meshIdx] = meshIdx;

			for (int i = 0; i < (int)data.indices.size(); ++i)
			{
				int posIndex = 3 * data.indices[i].vertex_index;
				int normIndex = 3 * data.indices[i].normal_index;
				int texIndex = 2 * data.indices[i].texcoord_index;
				int materialIndex = data.material_ids[i / 3];
				
				Vertex v;

				if (posIndex >= 0)
				{
					v.pos = vec3(
						attrib.vertices[posIndex + 0],
						attrib.vertices[posIndex + 1],
						attrib.vertices[posIndex + 2]);
				}

				if (normIndex >= 0)
				{
					v.normal = vec3(
						attrib.normals[normIndex + 0],
						attrib.normals[normIndex + 1],
						attrib.normals[normIndex + 2]);
				}

				if (texIndex >= 0)
				{
					v.uv = vec2(
						attrib.texcoords[texIndex + 0],
						attrib.texcoords[texIndex + 1]);
				}

				if (materialIndex >= 0)
				{
					tinyobj::material_t &matData = materials[materialIndex];
					v.color = rgb(matData.diffuse[0], matData.diffuse[1], matData.diffuse[2]);
				} 

				indices[i] = (int)i;
				vertices[i] = v;
			}

			model->meshes[meshIdx] = createMesh(vertices, (int)data.indices.size(), indices, (int)data.indices.size());
			free(indices);
			free(vertices);
		}

		return model;
	}
	else
		return NULL;
}

CompositeModel *copyModel(const CompositeModel *model)
{
	const CompositeModel *original = model;
	CompositeModel *copy = (CompositeModel *)malloc(sizeof(CompositeModel));
	memcpy(copy, original, sizeof(CompositeModel));
	
	copy->localTransforms = (Transform *)malloc(original->numModels * sizeof(Transform));
	memcpy(copy->localTransforms, original->localTransforms, original->numModels * sizeof(Transform));
	for (int i = 0; i < copy->numModels; ++i)
	{
		if (copy->localTransforms[i].parent == &original->transform)
			copy->localTransforms[i].parent = &copy->transform;
	}

	return copy;
}

Model createModel(const Vertex *vertices, int numVertices, const uint *indices, int numIndices)
{
	Model model;
	model.mesh = createMesh(vertices, numVertices, indices, numIndices);
	model.material = Material();
	model.transform = Transform();

	model.minAABB = vec3(+Inf);
	model.maxAABB = vec3(-Inf);
	for (int i = 0; i < numVertices; ++i)
	{
		model.minAABB = min(model.minAABB, vertices[i].pos);
		model.maxAABB = max(model.maxAABB, vertices[i].pos);
	}

	return model;
}

void drawMesh(Mesh mesh)
{
	glBindVertexArray(mesh.vertexSpecification);
	glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, NULL);
	glCheckErrors();
}

Mesh createMesh(
	const Vertex *vertices,
	int numVertices,
	const uint *indices,
	int numIndices)
{
	GpuBuffer vertexBuffer = createGpuBuffer(vertices, numVertices * sizeof(*vertices));
	return createMesh(vertexBuffer, numVertices, indices, numIndices);
}

Mesh createMesh(
	GpuBuffer vertexBuffer,
	int numVertices,
	const uint *indices,
	int numIndices)
{
	Mesh mesh;
	mesh.vertexBuffer = vertexBuffer;
	mesh.indexBuffer = createGpuBuffer(indices, numIndices * sizeof(*indices));
	mesh.numVertices = numVertices;
	mesh.numIndices = numIndices;

	glGenVertexArrays(1, &mesh.vertexSpecification);
	if (mesh.vertexSpecification != 0)
	{
		glBindVertexArray(mesh.vertexSpecification);
		{
			glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBuffer);
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glEnableVertexAttribArray(3);
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, pos));
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, tangent));
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));
			glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color));
		}
		glBindVertexArray(0);
	}
	else
		fprintf(stderr, "OpenGL failed to allocate a vertex array object\n");

	return mesh;
}

Texture loadTexture(
	const char *filename,
	GLenum internalFormat,
	GLenum minFilter,
	GLenum magFilter,
	GLenum wrapS,
	GLenum wrapT,
	bool genMipmaps)
{
	Texture texture = 0;

	int width, height, comp;
	stbi_set_flip_vertically_on_load(1);
	void *image = stbi_load(filename, &width, &height, &comp, STBI_rgb_alpha);

	if (image == NULL)
		fprintf(stderr, "couldn't read texture file '%s' because %s\n", filename, stbi_failure_reason());
	else
		texture = createTexture(image, width, height, GL_RGBA, internalFormat, minFilter, magFilter, wrapS, wrapT, genMipmaps);

	free(image);
	return texture;
}

Texture createTexture(
	const void *pixels,
	int width,
	int height,
	GLenum pixelFormat,
	GLenum internalFormat,
	GLenum minFilter,
	GLenum magFilter,
	GLenum wrapS,
	GLenum wrapT,
	bool genMipmaps)
{
	Texture texture;
	glGenTextures(1, &texture);

	if (texture)
	{
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

		glTexImage2D(
			GL_TEXTURE_2D,		   // Target texture type 
			0,					   // Mipmap level - ALWAYS 0
			(GLint)internalFormat, // Internal format
			(GLsizei)width,	       // Image width
			(GLsizei)height,       // Image height
			0,				       // Border? - always 0 aparently.
			pixelFormat,		   // Color components - important not to mess up
			GL_UNSIGNED_BYTE,      // Component format
			pixels                 // Image data
		);
	}
	else
		fprintf(stderr, "OpenGL failed to allocate %d x %d texture\n", width, height);

	glCheckErrors();
	return texture;
}

CubeMap loadCubeMap(
	const char *leftFilename,
	const char *rightFilename,
	const char *upFilename,
	const char *downFilename,
	const char *frontFilename,
	const char *backFilename,
	GLenum internalFormat,
	GLenum minFilter,
	GLenum magFilter,
	bool genMipmaps)
{
	CubeMap cubeMap;
	glGenTextures(1, &cubeMap);

	if (cubeMap)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, magFilter);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		const char *filenames[] = {
			rightFilename,
			leftFilename,
			upFilename,
			downFilename,
			backFilename,
			frontFilename
		};

		stbi_set_flip_vertically_on_load(1);
		for (int i = 0; i < 6; ++i)
		{
			int width, height, comp;
			void *pixels = stbi_load(filenames[i], &width, &height, &comp, STBI_rgb_alpha);

			if (pixels == NULL)
				fprintf(stderr, "couldn't read cube map texture file '%s' because %s\n", filenames[i], stbi_failure_reason());

			glTexImage2D(
				GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i),
				0,					   // Mipmap level - ALWAYS 0
				(GLint)internalFormat, // Internal format
				(GLsizei)width,	       // Image width
				(GLsizei)height,       // Image height
				0,				       // Border? - always 0 aparently.
				GL_RGBA,		       // Color components - important not to mess up
				GL_UNSIGNED_BYTE,      // Component format
				pixels                 // Image data
			);

			stbi_image_free(pixels);
		}
	}
	else
		fprintf(stderr, "OpenGL failed to allocate cube map\n");

	glCheckErrors();
	return cubeMap;
}

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
	GLenum pixelFormat,
	GLenum internalFormat,
	GLenum minFilter,
	GLenum magFilter,
	bool genMipmaps)
{
	CubeMap cubeMap;
	glGenTextures(1, &cubeMap);

	if (cubeMap)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, magFilter);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		int widths[6] = {
			rightWidth,
			leftWidth,
			upWidth,
			downWidth,
			backWidth,
			frontWidth
		};

		int heights[6] = {
			rightHeight,
			leftHeight,
			upHeight,
			downHeight,
			backHeight,
			frontHeight
		};

		const void *pixels[6] = {
			rightPixels,
			leftPixels,
			upPixels,
			downPixels,
			backPixels,
			frontPixels
		};

		if (internalFormat == GL_DEPTH_COMPONENT)
			pixelFormat = GL_DEPTH_COMPONENT;

		for (int i = 0; i < 6; ++i)
		{
			glTexImage2D(
				GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i),
				0,					   // Mipmap level - ALWAYS 0
				(GLint)internalFormat, // Internal format
				(GLsizei)widths[i],	   // Image width
				(GLsizei)heights[i],   // Image height
				0,				       // Border? - always 0 aparently.
				pixelFormat,		   // Color components - important not to mess up
				GL_UNSIGNED_BYTE,      // Component format
				pixels[i]              // Image data
			);
		}
	}
	else
		fprintf(stderr, "OpenGL failed to allocate cube map\n");

	glCheckErrors();
	return cubeMap;
}

Framebuffer createFramebuffer(Texture colorAttachment, Texture depthAttachment)
{
	Framebuffer framebuffer;
	glGenFramebuffers(1, &framebuffer);
	
	if (framebuffer != 0)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		if (colorAttachment != 0)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachment, 0);
		if (depthAttachment != 0)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachment, 0);
	}
	else 
		fprintf(stderr, "OpenGL failed to allocate framebuffer\n");
	
	glCheckErrors();
	return framebuffer;
}

LightProbe createReflectionProbe(
	int faceWidth,
	int faceHeight,
	GLenum internalFormat)
{
	LightProbe probe;
	probe.colorMap = createCubeMap(faceWidth, faceHeight, internalFormat);
	probe.depthMap = createCubeMap(faceWidth, faceHeight, GL_DEPTH_COMPONENT);

	for (int i = 0; i < 6; ++i)
	{
		probe.framebuffers[i] = createFramebuffer();
		glBindFramebuffer(GL_FRAMEBUFFER, probe.framebuffers[i]);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0,
			GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i),
			probe.colorMap, 0);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_DEPTH_ATTACHMENT,
			GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i),
			probe.depthMap, 0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glCheckErrors();
	return probe;
}

LightProbe createShadowProbe(
	int faceWidth,
	int faceHeight)
{
	LightProbe probe;
	probe.colorMap = 0;
	probe.depthMap = createCubeMap(faceWidth, faceHeight, GL_DEPTH_COMPONENT);

	glBindTexture(GL_TEXTURE_CUBE_MAP, probe.depthMap);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	for (int i = 0; i < 6; ++i)
	{
		probe.framebuffers[i] = createFramebuffer();
		glBindFramebuffer(GL_FRAMEBUFFER, probe.framebuffers[i]);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_DEPTH_ATTACHMENT,
			GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i),
			probe.depthMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glCheckErrors();
	return probe;
}

Spotlight createSpotlight(
	vec3 pos,
	vec3 dir,
	vec3 color,
	int shadowMapResolution)
{
	auto angleBetween = [](vec3 a, vec3 b)
	{
		return acos(dot(a, b) / sqrt(lengthSq(a) * lengthSq(b)));
	};

	Spotlight spotlight;
	//float rotY = angleBetween(vec3(1, 0, 0), vec3(dir))
	return spotlight;
}

ShaderProgram loadShaderProgram(const char *vertFilename, const char *fragFilename)
{
	char *vertSrc = readWholeFile(vertFilename);
	char *fragSrc = readWholeFile(fragFilename);
	ShaderProgram shader = 0;

	if (vertSrc == NULL)
		fprintf(stderr, "couldn't read vertex shader file '%s'\n", vertFilename);
	else if (fragSrc == NULL)
		fprintf(stderr, "couldn't read fragment shader file '%s'\n", fragFilename);
	else
	{
		GLenum types[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
		const char *sources[] = { vertSrc, fragSrc };
		shader = createShaderProgram(types, sources, countof(sources));
	}

	free(vertSrc);
	free(fragSrc);
	return shader;
}

ShaderProgram createShaderProgram(
	const GLenum *types,
	const char *const *sources,
	int numSources)
{
	ShaderProgram program = glCreateProgram();
	if (program != 0)
	{
		// I need a workaround for Intel GPUs

		GLuint *shaders = (GLuint *)malloc(numSources * sizeof(GLuint));
		bool allComponentsCompiledOk = true;
		for (int i = 0; i < numSources; ++i)
		{
			shaders[i] = createShader(types[i], sources[i]);
			if (!shaders[i])
				allComponentsCompiledOk = false;
		}

		GLint linkOk = 0;
		if (allComponentsCompiledOk)
		{
			for (int i = 0; i < numSources; ++i)
				glAttachShader(program, shaders[i]);

			//NOTE: if youre getting a segfault here on intel, check to make sure your
			// shader code is 100% correct. Run it through glslang or something. Intel
			// drivers like to crash when the shader code is incorrect.
			glLinkProgram(program);

			glGetProgramiv(program, GL_LINK_STATUS, &linkOk);
			if (!linkOk)
			{
				GLint logLength;
				glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
				char *log = (char *)malloc((size_t)logLength);
				glGetProgramInfoLog(program, logLength, NULL, (GLchar *)log);
				fprintf(stderr, "GLSLC: %s", log);
				free(log);
			}

			for (int i = 0; i < numSources; ++i)
			{
				glDetachShader(program, shaders[i]);
				glDeleteShader(shaders[i]);
			}
		}

		free(shaders);

		if (!linkOk)
			glClearErrors();
	} 
	else
		fprintf(stderr, "OpenGL failed to allocate shader program");

	glCheckErrors();
	return program;
}

GpuBuffer createGpuBuffer(
	const void *data,
	size_t numBytes,
	GLenum usage)
{
	GpuBuffer buffer;
	glGenBuffers(1, &buffer);

	if (buffer)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)numBytes, data, usage);
	}
	else
		fprintf(stderr, "OpenGL failed to allocate a %zu byte buffer", numBytes);

	glCheckErrors();
	return buffer;
}

void drawString(
	const Font *font,
	const char *string,
	vec2 position,
	bool center,
	vec2 scale,
	vec4 color,
	float rotationRadians)
{
	struct TextVertex
	{
		vec2 pos;
		vec2 uv;
		vec4 color;
	};
	
	constexpr int TextBufferSize = 512;
	static TextVertex vertices[TextBufferSize * 4];
	static uint16_t indices[TextBufferSize * 6];
	static GpuBuffer vertexBuffer = createGpuBuffer(NULL, sizeof(vertices), GL_STREAM_DRAW);
	static GpuBuffer indexBuffer = createGpuBuffer(NULL, sizeof(indices), GL_STREAM_DRAW);
	static ShaderProgram shader = loadShaderProgram("assets/shaders/text.vert.glsl", "assets/shaders/text.frag.glsl");
	static VertexSpecification vertexSpec = 0;

	if (vertexSpec == 0)
	{
		glGenVertexArrays(1, &vertexSpec);
		if (vertexSpec != 0)
		{
			glBindVertexArray(vertexSpec);
			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void *)offsetof(TextVertex, pos));
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void *)offsetof(TextVertex, uv));
			glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void *)offsetof(TextVertex, color));
			glBindVertexArray(0);
		}
		else
			fprintf(stderr, "OpenGL failed to allocate vertex array object\n");
	}

	int length = (int)strlen(string);
	assert(length < TextBufferSize);

	vec2 origin = vec2(0);
	if (center)
	{
		vec2 size = scale * getStringSize(font, string);
		origin.x -= size.x / 2;
		origin.y += size.y / 2;
	}

	vec2 pos = origin;
	int nextVertex = 0;
	for (int i = 0; i < length; ++i)
	{
		int c = getCharIndex(font, string[i]);

		float x0 = pos.x + scale.x * font->xOffset[c];
		float x1 = x0 + scale.x * font->w[c];
		float y0 = pos.y + scale.y * font->yOffset[c];
		float y1 = y0 + scale.y * font->h[c];
		float u0 = font->x[c] / (float)font->atlasWidth;
		float v0 = font->y[c] / (float)font->atlasHeight;
		float u1 = (font->x[c] + font->w[c]) / (float)font->atlasWidth;
		float v1 = (font->y[c] + font->h[c]) / (float)font->atlasHeight;

		vertices[nextVertex++] = { vec2(x0, y0), vec2(u0, v0), color };
		vertices[nextVertex++] = { vec2(x1, y0), vec2(u1, v0), color };
		vertices[nextVertex++] = { vec2(x1, y1), vec2(u1, v1), color };
		vertices[nextVertex++] = { vec2(x0, y1), vec2(u0, v1), color };

		pos.x += scale.x * (font->xAdvance[c] + getKerning(font, string[i], string[i + 1]));
	}

	int nextIndex = 0;
	for (int i = 0; i < length; ++i)
	{
		int base = i * 4;
		indices[nextIndex++] = uint16_t(base + 0);
		indices[nextIndex++] = uint16_t(base + 1);
		indices[nextIndex++] = uint16_t(base + 2);
		indices[nextIndex++] = uint16_t(base + 2);
		indices[nextIndex++] = uint16_t(base + 3);
		indices[nextIndex++] = uint16_t(base + 0);
	}

	int numVertices = nextVertex;
	int numIndices = nextIndex;

	mat4 projection = orthoMatLH(0.0f, (float)windowWidth, (float)windowHeight, 0.0f, -1.0f, 1.0f);
	mat4 model = translationMat(vec3(position, 0)) * rotationMat(vec3(0, 0, 1), rotationRadians);
	mat4 mvp = projection * model;
	glUseProgram(shader);
	glUniformMatrix4fv(0, 1, GL_FALSE, (GLfloat *)&mvp);

	glBindVertexArray(vertexSpec);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, font->atlas);
	glDrawElements(GL_TRIANGLES, (GLsizei)numIndices, GL_UNSIGNED_SHORT, NULL);

	glCheckErrors();
}

Font *loadFont(const char *filename, float sizeInPixels)
{
	char *fontData = readWholeFile(filename);
	if (fontData)
	{
		stbtt_fontinfo info;
		if (stbtt_InitFont(&info, (uint8_t *)fontData, 0))
		{
			constexpr int FirstChar = 32;
			constexpr int NumChars = 96;
			constexpr int AtlasWidth = 512;
			constexpr int AtlasHeight = 512;

			stbtt_packedchar charInfo[NumChars];
			uint8_t *atlasPixels = (uint8_t *)malloc(AtlasWidth * AtlasHeight);

			stbtt_pack_context pack;
			stbtt_PackBegin(&pack, atlasPixels, AtlasWidth, AtlasHeight, 0, 1, NULL);
			stbtt_PackSetOversampling(&pack, 1, 1);
			int success = stbtt_PackFontRange(&pack, (uint8_t *)fontData, 0, STBTT_POINT_SIZE(sizeInPixels), FirstChar, NumChars, charInfo);
			stbtt_PackEnd(&pack);

			if (!success)
				fprintf(stderr, "couldn't properly pack font '%s'", filename);

			Font *font = (Font *)malloc(sizeof(Font));
			font->atlas = createTexture(atlasPixels, AtlasWidth, AtlasHeight, GL_RED, GL_RED);
			free(atlasPixels);

			font->firstChar = FirstChar;
			font->numChars = NumChars - 1;
			font->atlasWidth = AtlasWidth;
			font->atlasHeight = AtlasHeight;

			float scale = stbtt_ScaleForPixelHeight(&info, sizeInPixels);
			font->kerningScale = scale;

			int ascent, descent, lineGap;
			stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
			font->ascent = scale * ascent;
			font->descent = scale * descent;
			font->lineGap = scale * lineGap;

			for (int i = 0; i < NumChars; ++i)
			{
				font->x[i] = charInfo[i].x0;
				font->y[i] = charInfo[i].y0;
				font->w[i] = uint16_t(charInfo[i].x1 - charInfo[i].x0);
				font->h[i] = uint16_t(charInfo[i].y1 - charInfo[i].y0);
				font->xOffset[i] = charInfo[i].xoff;
				font->yOffset[i] = charInfo[i].yoff;
				font->xAdvance[i] = charInfo[i].xadvance;
			}

			int glyphs[NumChars];
			for (int i = 0; i < NumChars; ++i)
				glyphs[i] = stbtt_FindGlyphIndex(&info, FirstChar + i);

			int kerningLength = stbtt_GetKerningTableLength(&info);
			memset(&font->xKerning, 0, sizeof(font->xKerning));
			if (kerningLength > 0)
			{
				stbtt_kerningentry *kerning = (stbtt_kerningentry *)malloc(kerningLength * sizeof(stbtt_kerningentry));
				stbtt_GetKerningTable(&info, kerning, kerningLength);

				// Even though this looks like it would be terribly slow it actually isnt that bad.. ~10% of the total font load time
				for (int charIdx1 = 0; charIdx1 < NumChars; ++charIdx1)
				{
					int glyph1 = glyphs[charIdx1];
					for (int kernIdx1 = 0; kernIdx1 < kerningLength; ++kernIdx1)
					{
						if (kerning[kernIdx1].glyph1 == glyph1)
						{
							for (int kernIdx2 = kernIdx1; kernIdx2 < kerningLength && kerning[kernIdx2].glyph1 == glyph1; ++kernIdx2)
							{
								int glyph2 = kerning[kernIdx2].glyph2;
								for (int charIdx2 = 0; charIdx2 < NumChars; ++charIdx2)
								{
									if (glyph2 == glyphs[charIdx2])
									{
										font->xKerning[charIdx1][charIdx2] = int16_t(kerning[kernIdx2].advance);
										break;
									}
								}
							}
						}

						if (kerning[kernIdx1].glyph1 >= glyph1)
							break;
					}
				}

				free(kerning);
			}

			free(fontData);
			return font;
		}
		else
		{
			free(fontData);
			fprintf(stderr, "couldn't load font from '%s'", filename);
			return NULL;
		}
	}
	else
	{
		fprintf(stderr, "couldn't read font file '%s'", filename);
		return NULL;
	}
}

bool shaderIsValid(ShaderProgram program)
{
	GLint status;
	glValidateProgram(program);
	glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
	return status == GL_TRUE;
}