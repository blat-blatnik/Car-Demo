/*
	TODO
	---------------------
	[+] make gamma correction work on Intel.. oh intel...
	[ ] finish stage lights
	 ->  [ ] attach spot lights
	 ->  [ ] shadow map the spot lights
	 ->  [ ] animate the spot lights
	[ ] add smoke to the room - billboards? volume?
	 ->  [ ] animate the smoke
	[ ] volumetric shadows - ray cast?
	[ ] find better textures for the garage
	[ ] find better car model
	[ ] fix shadow acne
	[ ] animate car wheels
	[ ] simple car physics
	[ ] instructions?
*/

#include "system.h"
#include "graphics.h"
#include <vector>

constexpr vec3 CubeDirections[6] = {
	vec3(+1, 0, 0),
	vec3(-1, 0, 0),
	vec3(0, +1, 0),
	vec3(0, -1, 0),
	vec3(0, 0, +1),
	vec3(0, 0, -1),
};
constexpr vec3 CubeUpVectors[6] = {
	vec3(0, -1,  0),
	vec3(0, -1,  0),
	vec3(0,  0, +1),
	vec3(0,  0, -1),
	vec3(0, -1,  0),
	vec3(0, -1,  0),
};
constexpr Vertex CubeVertices[] = {
	// bottom
	{ vec3(-1, -1, -1), vec3(0, +1, 0), vec3(+1, 0, 0), vec2(0, 0), vec4(1) },
	{ vec3(+1, -1, -1), vec3(0, +1, 0), vec3(+1, 0, 0), vec2(1, 0), vec4(1) },
	{ vec3(+1, -1, +1), vec3(0, +1, 0), vec3(+1, 0, 0), vec2(1, 1), vec4(1) },
	{ vec3(-1, -1, +1), vec3(0, +1, 0), vec3(+1, 0, 0), vec2(0, 1), vec4(1) },
	// top
	{ vec3(-1, +1, -1), vec3(0, -1, 0), vec3(+1, 0, 0), vec2(0, 0), vec4(1) },
	{ vec3(+1, +1, -1), vec3(0, -1, 0), vec3(+1, 0, 0), vec2(1, 0), vec4(1) },
	{ vec3(+1, +1, +1), vec3(0, -1, 0), vec3(+1, 0, 0), vec2(1, 1), vec4(1) },
	{ vec3(-1, +1, +1), vec3(0, -1, 0), vec3(+1, 0, 0), vec2(0, 1), vec4(1) },
	// right
	{ vec3(+1, -1, +1), vec3(-1, 0, 0), vec3(0, 0, -1), vec2(0, 0), vec4(1) },
	{ vec3(+1, -1, -1), vec3(-1, 0, 0), vec3(0, 0, -1), vec2(1, 0), vec4(1) },
	{ vec3(+1, +1, -1), vec3(-1, 0, 0), vec3(0, 0, -1), vec2(1, 1), vec4(1) },
	{ vec3(+1, +1, +1), vec3(-1, 0, 0), vec3(0, 0, -1), vec2(0, 1), vec4(1) },
	// left
	{ vec3(-1, -1, -1), vec3(+1, 0, 0), vec3(0, 0, +1), vec2(0, 0), vec4(1) },
	{ vec3(-1, -1, +1), vec3(+1, 0, 0), vec3(0, 0, +1), vec2(1, 0), vec4(1) },
	{ vec3(-1, +1, +1), vec3(+1, 0, 0), vec3(0, 0, +1), vec2(1, 1), vec4(1) },
	{ vec3(-1, +1, -1), vec3(+1, 0, 0), vec3(0, 0, +1), vec2(0, 1), vec4(1) },
	// front
	{ vec3(-1, -1, -1), vec3(0, 0, +1), vec3(+1, 0, 0), vec2(0, 0), vec4(1) },
	{ vec3(+1, -1, -1), vec3(0, 0, +1), vec3(+1, 0, 0), vec2(1, 0), vec4(1) },
	{ vec3(+1, +1, -1), vec3(0, 0, +1), vec3(+1, 0, 0), vec2(1, 1), vec4(1) },
	{ vec3(-1, +1, -1), vec3(0, 0, +1), vec3(+1, 0, 0), vec2(0, 1), vec4(1) },
	// back
	{ vec3(-1, -1, +1), vec3(0, 0, -1), vec3(+1, 0, 0), vec2(0, 0), vec4(1) },
	{ vec3(+1, -1, +1), vec3(0, 0, -1), vec3(+1, 0, 0), vec2(1, 0), vec4(1) },
	{ vec3(+1, +1, +1), vec3(0, 0, -1), vec3(+1, 0, 0), vec2(1, 1), vec4(1) },
	{ vec3(-1, +1, +1), vec3(0, 0, -1), vec3(+1, 0, 0), vec2(0, 1), vec4(1) },
};
constexpr uint CubeIndices[] = {
	0, 1, 2, 2, 3, 0, // bottom
	4, 5, 6, 6, 7, 4, // top
	8, 9, 10, 10, 11, 8, // right
	12, 13, 14, 14, 15, 12, // left
	16, 17, 18, 18, 19, 16, // front
	20, 21, 22, 22, 23, 20, // back
};

constexpr vec3 StageLightColor = vec3(0.9, 0.9, 1.0);
const float StageLightCosOuterCutoff = cos(radians(27.5f));
const float StageLightCosInnerCutoff = cos(radians(15.0f));

//NOTE: Dont forget cube maps use right handed coordinates!
const mat4 cubeProjection = perspectiveMatRH(radians(90.0f), 1.0f, NearPlane, FarPlane);

void drawModel(const Model &model, mat4 viewProjection)
{
	mat4 modelMatrix = model.transform.getMatrix();
	setUniform(0, modelMatrix);
	setUniform(1, viewProjection * modelMatrix);
	setUniform(6, model.material.ambientColor);
	setUniform(7, model.material.diffuseColor);
	setUniform(8, model.material.specularColor);
	glUniform1f(9, model.material.specularExponent);
	glUniform1f(10, model.material.alpha);
	glUniform1ui(11, renderMode == RenderNormals);
	drawMesh(model.mesh);
}

void controlPositionAndRotation(Transform *transform, float deltaTime)
{
	bool upPressed = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
	bool downPressed = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
	bool leftPressed = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
	bool rightPressed = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
	bool forwardPressed = glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS;
	bool backwardPressed = glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS;
	bool rotateYForwardPressed = glfwGetKey(window, GLFW_KEY_KP_6) == GLFW_PRESS;
	bool rotateYBackwardPressed = glfwGetKey(window, GLFW_KEY_KP_4) == GLFW_PRESS;
	bool rotateXForwardPressed = glfwGetKey(window, GLFW_KEY_KP_8) == GLFW_PRESS;
	bool rotateXBackwardPressed = glfwGetKey(window, GLFW_KEY_KP_2) == GLFW_PRESS;
	bool speedUpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

	vec3 moveDir = vec3(0);
	if (upPressed)
		moveDir += vec3(0, 1, 0);
	if (downPressed)
		moveDir -= vec3(0, 1, 0);
	if (leftPressed)
		moveDir -= vec3(1, 0, 0);
	if (rightPressed)
		moveDir += vec3(1, 0, 0);
	if (forwardPressed)
		moveDir -= vec3(0, 0, 1);
	if (backwardPressed)
		moveDir += vec3(0, 0, 1);

	float rotateX = 0;
	if (rotateXForwardPressed)
		rotateX += 1;
	if (rotateXBackwardPressed)
		rotateX -= 1;

	float rotateY = 0;
	if (rotateYForwardPressed)
		rotateY += 1;
	if (rotateYBackwardPressed)
		rotateY -= 1;

	float moveSpeed = 0.5f * deltaTime;
	float rotateSpeed = 0.5f * deltaTime;

	if (speedUpPressed)
	{
		moveSpeed *= 8;
		rotateSpeed *= 8;
	}

	transform->pos += moveSpeed * moveDir;
	transform->rotate(vec3(0, 1, 0), rotateSpeed * rotateY);
	transform->rotate(vec3(1, 0, 0), rotateSpeed * rotateX);
}

void drawModelToShadowMap(const CompositeModel *model, mat4 viewProjection)
{
	for (int i = 0; i < model->numModels; ++i)
	{
		Model m = model->getModel(i);
		if (m.material.alpha > 0.95f)
		{
			mat4 modelMatrix = m.transform.getMatrix();
			mat4 modelViewProjection = viewProjection * modelMatrix;
			if (!frustumCullAABB(m.minAABB, m.maxAABB, modelViewProjection))
			{
				setUniform(0, modelMatrix);
				setUniform(1, modelViewProjection);
				drawMesh(m.mesh);
			}
		}
	}
}

int main()
{
	//convertObjToModel("assets/models/stage-light.obj", "assets/models/stage-light.model");

	initSystem();

	Font *segoeUi = loadFont("assets/fonts/segoeui.ttf", 32);

	ShaderProgram carShader = loadShaderProgram("assets/shaders/common.vert.glsl", "assets/shaders/car.frag.glsl");
	//ShaderProgram stagelightShader = loadShaderProgram("assets/shaders/common.vert.glsl", "assets/shaders/stage-light.frag.glsl");
	ShaderProgram shadowShader = loadShaderProgram("assets/shaders/shadow.vert.glsl", "assets/shaders/shadow.frag.glsl");
	ShaderProgram garageShader = loadShaderProgram("assets/shaders/garage.vert.glsl", "assets/shaders/garage.frag.glsl");

	Model garageModel = createModel(CubeVertices, countof(CubeVertices), CubeIndices, countof(CubeIndices));	
	garageModel.transform.scale = vec3(20, 10, 20);
	garageModel.transform.pos.y = garageModel.transform.scale.y;

	CubeMap garageDiffuse = loadCubeMap(
		"assets/textures/brick-diffuse.png",
		"assets/textures/brick-diffuse.png",
		"assets/textures/concrete-diffuse.png",
		"assets/textures/concrete-diffuse.png",
		"assets/textures/brick-diffuse.png",
		"assets/textures/brick-diffuse.png");
	CubeMap garageNormal = loadCubeMap(
		"assets/textures/brick-norm.png",
		"assets/textures/brick-norm.png",
		"assets/textures/concrete-norm.png",
		"assets/textures/concrete-norm.png",
		"assets/textures/brick-norm.png",
		"assets/textures/brick-norm.png");

	LightProbe garageReflection = createReflectionProbe(ReflectionMapResolution, ReflectionMapResolution, GL_RGB);
	LightProbe shadowProbe = createShadowProbe(ShadowMapResolution, ShadowMapResolution);

	CompositeModel *carModel = loadModel("assets/models/car.model");
	//carModel->transform.scale *= 3.0f;
	//carModel->transform.pos.x = 5.0f;

	CompositeModel *stageLight1 = loadModel("assets/models/stage-light.model");
	stageLight1->transform.scale = vec3(5);
	stageLight1->transform.pos = vec3(-8, 9, -10);
	stageLight1->transform.rotation = quat(0, 0, 0, 1);

	CompositeModel *stageLight2 = copyModel(stageLight1);
	stageLight2->transform.pos.x = -stageLight1->transform.pos.x;

	float cameraRotX = radians(45.0f);
	float cameraRotY = radians(180.0f);
	float cameraDist = 10;

	vec3 lightPos = vec3(0, 10, 0);

	struct TransparentModel { Model model; float distToCamera; };
	std::vector<TransparentModel> transparentModels;

	glfwSwapInterval(0); // turn on vsync
	glEnable(GL_MULTISAMPLE);

	startGameLoop([&](double deltaTime)
	{
		cameraDist = clamp(cameraDist - (20.0f * (float)deltaTime) * mouseWheelDelta, 8.0f, 20.0f);
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT))
		{
			cameraRotX += 0.01f * mouseDeltaY;
			cameraRotY += 0.01f * mouseDeltaX;
			cameraRotX = clamp(cameraRotX, 0.05f, +0.499f * Pi);
			//cameraRotX = clamp(cameraRotX, -0.499f * Pi, +0.499f * Pi);
		}

		//controlPositionAndRotation(&stageLight1->transform, (float)deltaTime);
		//stageLight2->transform = stageLight1->transform;
		//stageLight2->transform.pos.x = -stageLight1->transform.pos.x;

		float t = (float)glfwGetTime();
		lightPos.x = 8 * cos(t);
		lightPos.z = 10 * sin(t);
		lightPos.y = 10 + 2 * cos(0.2f * t);

		vec3 carCenter = carModel->getCenter();
		vec3 light1Center = stageLight1->getCenter();
		vec3 light2Center = stageLight2->getCenter();
		vec3 light1Dir = carCenter - light1Center;
		vec3 light2Dir = carCenter - light2Center;
		float light1XRotBase = atan2(light1Dir.y, light1Dir.z);
		float light2XRotBase = atan2(light2Dir.y, light2Dir.z);
		float light1YRotBase = Pi + atan2(light1Dir.x, light1Dir.z);
		float light2YRotBase = Pi + atan2(light2Dir.x, light2Dir.z);
		quat light1XRot = rotationQuat(vec3(1, 0, 0), light1XRotBase);
		quat light2XRot = rotationQuat(vec3(1, 0, 0), light2XRotBase);

		stageLight1->transform.rotation = rotationQuat(vec3(0, 1, 0), light1YRotBase);
		stageLight2->transform.rotation = rotationQuat(vec3(0, 1, 0), light2YRotBase);
		stageLight1->localTransforms[4].rotation = light1XRot;
		stageLight1->localTransforms[5].rotation = light1XRot;
		stageLight2->localTransforms[4].rotation = light2XRot;
		stageLight2->localTransforms[5].rotation = light2XRot;
		
		vec3 stageLight1Dir = rotate(vec3(0, 0, -1), stageLight1->transform.rotation);
		vec3 stageLight2Dir = rotate(vec3(0, 0, -1), stageLight2->transform.rotation);
		stageLight1Dir = normalize(rotate(stageLight1Dir, stageLight1->localTransforms[4].rotation));
		stageLight2Dir = normalize(rotate(stageLight2Dir, stageLight2->localTransforms[4].rotation));

		vec3 cameraPos = vec3(0, 0, -cameraDist);
		cameraPos = rotate(cameraPos, vec3(1, 0, 0), cameraRotX);
		cameraPos = rotate(cameraPos, vec3(0, 1, 0), cameraRotY);
		vec3 cameraDir = normalize(-cameraPos);

		if (renderMode == RenderWireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		//glDisable(GL_FRAMEBUFFER_SRGB);
		
		glUseProgram(shadowShader);
		setUniform(20, lightPos);
		glUniform1f(21, FarPlane);
		glViewport(0, 0, ShadowMapResolution, ShadowMapResolution);

		for (int i = 0; i < 6; ++i)
		{
			mat4 cubeView = lookAtMatRH(lightPos, CubeDirections[i], CubeUpVectors[i]);
			mat4 cubeViewProjection = cubeProjection * cubeView;

			glBindFramebuffer(GL_FRAMEBUFFER, shadowProbe.framebuffers[i]);
			glClear(GL_DEPTH_BUFFER_BIT);
			drawModelToShadowMap(carModel, cubeViewProjection);
			
			//TODO: put these back after you remove the point light
			// right now the stage lights just levitate and cast flying shadows
			// 
			//drawModelToShadowMap(stageLight1, cubeViewProjection);
			//drawModelToShadowMap(stageLight2, cubeViewProjection);

			mat4 modelMatrix = garageModel.transform.getMatrix();
			mat4 modelViewProjection = cubeViewProjection * modelMatrix;
			setUniform(0, modelMatrix);
			setUniform(1, modelViewProjection);
			drawMesh(garageModel.mesh);
		}

		glUseProgram(garageShader);
		mat4 garageModelMatrix = garageModel.transform.getMatrix();
		setUniform(0, garageModelMatrix);
		setUniform(4, cameraPos);
		setUniform(6, garageModel.material.ambientColor);
		glUniform1ui(11, 0);
		setUniform(20, lightPos);
		glUniform1f(21, FarPlane);
		glUniform1i(12, 0);
		glUniform1i(13, 1);
		glUniform1i(14, 2);
		glUniform1i(22, 0);
		glUniform1i(23, 0);
		bindUniformCubeMap(12, 0, garageDiffuse);
		bindUniformCubeMap(13, 1, garageNormal);
		bindUniformCubeMap(14, 2, shadowProbe.depthMap);
		//setUniform(30, stageLight1->transform.pos);
		//setUniform(31, stageLight1Dir);
		//setUniform(32, StageLightColor);

		glViewport(0, 0, ReflectionMapResolution, ReflectionMapResolution);

		for (int i = 0; i < 6; ++i)
		{
			vec3 dir = CubeDirections[i];
			mat4 cubeView = lookAtMatRH(carCenter, dir, CubeUpVectors[i]);
			setUniform(1, cubeProjection * cubeView * garageModelMatrix);
			setUniform(5, dir);
			glBindFramebuffer(GL_FRAMEBUFFER, garageReflection.framebuffers[i]);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			drawMesh(garageModel.mesh);
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glEnable(GL_FRAMEBUFFER_SRGB);
		glViewport(0, 0, windowWidth, windowHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(carShader);
		mat4 view = lookAtMatLH(cameraPos, cameraDir, vec3(0, 1, 0));
		mat4 projection = perspectiveMatLH(radians(60.0f), (float)windowWidth / windowHeight, 0.001f, 1000.0f);
		mat4 viewProjection = projection * view;
		setUniform(4, cameraPos);
		setUniform(5, cameraDir);
		glUniform1f(13, 0.2f);
		setUniform(20, lightPos);
		glUniform1f(21, FarPlane);
		glUniform1i(12, 0);
		glUniform1i(14, 1);
		glUniform1i(23, 1);
		bindUniformCubeMap(12, 0, garageReflection.colorMap);
		bindUniformCubeMap(14, 1, shadowProbe.depthMap);

		transparentModels.clear();

		for (int i = 0; i < carModel->numModels; ++i)
		{
			Model model = carModel->getModel(i);
			if (model.material.alpha > 0.95)
				drawModel(model, viewProjection);
			else
			{
				vec3 center = 0.5f * (model.minAABB + model.maxAABB);
				center = (model.transform.getMatrix() * vec4(center, 1)).xyz;
				float dist = lengthSq(cameraPos - center);
				transparentModels.push_back({ model, dist });
			}
		}

		glUniform1f(13, 0.0f);
		//for (int i = 0; i < stageLight1->numModels; ++i)
		//	drawModel(stageLight1->getModel(i), viewProjection);
		//for (int i = 0; i < stageLight2->numModels; ++i)
		//	drawModel(stageLight2->getModel(i), viewProjection);
		
		glUseProgram(garageShader);
		setUniform(0, garageModelMatrix);
		setUniform(1, viewProjection * garageModelMatrix);
		setUniform(20, lightPos);
		glUniform1f(21, FarPlane);
		setUniform(4, cameraPos);
		setUniform(5, cameraDir);
		setUniform(6, garageModel.material.ambientColor);
		glUniform1ui(11, renderMode == RenderNormals);
		bindUniformCubeMap(12, 0, garageDiffuse);
		bindUniformCubeMap(13, 1, garageNormal);
		bindUniformCubeMap(14, 2, shadowProbe.depthMap);
		glUniform1i(22, 1);
		glUniform1i(23, 1);
		drawMesh(garageModel.mesh);

		glUseProgram(carShader);
		setUniform(4, cameraPos);
		setUniform(5, cameraDir);
		setUniform(20, lightPos);
		glUniform1f(13, 0.2f);
		glUniform1f(21, FarPlane);
		glUniform1i(23, 1);
		bindUniformCubeMap(12, 0, garageReflection.colorMap);
		bindUniformCubeMap(14, 1, shadowProbe.depthMap);
		
		qsort(&transparentModels[0], transparentModels.size(), sizeof(transparentModels[0]), [](const void *left, const void *right)
		{
			const TransparentModel *lm = (const TransparentModel *)left;
			const TransparentModel *rm = (const TransparentModel *)right;
			return lm->distToCamera > rm->distToCamera ? +1 : -1;
		});
		for (const TransparentModel &trans : transparentModels)
			drawModel(trans.model, viewProjection);

		glDisable(GL_DEPTH_TEST);

		char string[256];
		//sprintf(string, "%.1lf fps", 1 / deltaTime);
		//drawString(segoeUi, string, vec2(10, 20), false, vec2(0.5));
		//sprintf(string, "camera = [%.1f %.1f %.1f]", cameraPos.x, cameraPos.y, cameraPos.z);
		//drawString(segoeUi, string, vec2(10, 40), false, vec2(0.5));
		//sprintf(string, "light = [%.1f %.1f %.1f]", lightPos.x, lightPos.y, lightPos.z);
		//drawString(segoeUi, string, vec2(10, 60), false, vec2(0.5));
		//
		//vec3 dir = rotate(vec3(0, 0, 1), stageLight1->transform.rotation);
		//sprintf(string, "pos = [%.4f %.4f %.4f]", stageLight1->transform.pos.x, stageLight1->transform.pos.y, stageLight1->transform.pos.z);
		//drawString(segoeUi, string, vec2(10, 100), false, vec2(0.5));
		//sprintf(string, "dir = [%.4f %.4f %.4f]", dir.x, dir.y, dir.z);
		//drawString(segoeUi, string, vec2(10, 120), false, vec2(0.5));
		//sprintf(string, "rot = [%.4f %.4f %.4f %.4f]", stageLight1->transform.rotation.x, stageLight1->transform.rotation.y, stageLight1->transform.rotation.z, stageLight1->transform.rotation.w);
		//drawString(segoeUi, string, vec2(10, 140), false, vec2(0.5));

		glCheckErrors();
		glfwSwapBuffers(window);
	});

	return 0;
}