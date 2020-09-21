#pragma once

#include "common.h"
#include "lib/bmath.h"
#include "lib/glad.h"
#include "lib/glfw3.h"
#include <functional>

enum RenderMode
{
	RenderDefault,
	RenderWireframe,
	RenderNormals,
};

extern GLFWwindow *window;
extern int windowWidth;
extern int windowHeight;
extern int mouseX;
extern int mouseY;
extern int mouseDeltaX;
extern int mouseDeltaY;
extern int mouseWheelDelta;
extern RenderMode renderMode;

void initSystem();
void startGameLoop(std::function<void(double deltaTime)>);

double getDeltaTime(uint64_t time1, uint64_t time2);