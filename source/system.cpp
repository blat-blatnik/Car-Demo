#include "system.h"

/* request a dedicated GPU if avaliable https://stackoverflow.com/a/39047129 */
#ifdef _MSC_VER
extern "C" __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

GLFWwindow *window;
int windowWidth;
int windowHeight;
int mouseX;
int mouseY;
int mouseDeltaX;
int mouseDeltaY;
int mouseWheelDelta;
RenderMode renderMode = RenderDefault;

static double timerPeriod;

static void onGlfwError(int code, const char *desc)
{
	printf("GLFW error 0x%X: %s\n", code, desc);
}
static void onGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	const char *severityMessage =
		severity == GL_DEBUG_SEVERITY_HIGH ? "error" :
		severity == GL_DEBUG_SEVERITY_MEDIUM ? "warning" :
		severity == GL_DEBUG_SEVERITY_LOW ? "warning" :
		severity == GL_DEBUG_SEVERITY_NOTIFICATION ? "info" :
		"unknown";
	const char *sourceMessage =
		source == GL_DEBUG_SOURCE_SHADER_COMPILER ? "glslc" :
		source == GL_DEBUG_SOURCE_API ? "API" :
		source == GL_DEBUG_SOURCE_WINDOW_SYSTEM ? "windows API" :
		source == GL_DEBUG_SOURCE_APPLICATION ? "application" :
		source == GL_DEBUG_SOURCE_THIRD_PARTY ? "third party" :
		"unknown";
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
		fprintf(stderr, "OpenGL %s 0x%X: %s (source: %s)\n", severityMessage, (int)id, message, sourceMessage);
}

// Window Events
// -------------
static void onFramebufferResized(GLFWwindow *window, int newWidth, int newHeight)
{
	windowWidth = newWidth;
	windowHeight = newHeight;
}
static void onKey(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS)
		return;

	switch (key)
	{
		case GLFW_KEY_ESCAPE: /* close the window when ESC is pressed */
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		case GLFW_KEY_F3:
			renderMode = RenderMode((int(renderMode) + 1) % (1 + RenderNormals));
			break;
		case GLFW_KEY_F:
		{
			GLFWmonitor *monitor = glfwGetPrimaryMonitor();
			int monX, monY, monW, monH;
			glfwGetMonitorWorkarea(monitor, &monX, &monY, &monW, &monH);

			if (glfwGetWindowMonitor(window) == NULL)
				glfwSetWindowMonitor(window, monitor, monX, monY, monW, monH, GLFW_DONT_CARE);
			else
			{
				int x = monX + (monW - 1280) / 2;
				int y = monY + (monH -  720) / 2;
				glfwSetWindowMonitor(window, NULL, x, y, 1280, 720, GLFW_DONT_CARE);
			}
		} break;
		default: break;
	}
}
static void onMouseButton(GLFWwindow *window, int button, int action, int mods)
{

}
static void onMouseMove(GLFWwindow *window, double newX, double newY)
{
	mouseX = (int)newX;
	mouseY = (int)newY;
}
static void onMouseWheel(GLFWwindow *window, double dX, double dY)
{
	if (dY < 0)
		--mouseWheelDelta;
	else if (dY > 0)
		++mouseWheelDelta;
}

void initSystem()
{
	glfwSetErrorCallback(onGlfwError);
	int glfwOk = glfwInit();
	if (!glfwOk)
	{
		fprintf(stderr, "ERROR: GLFW failed to initialize .. aborting\n");
		abort();
	}

	/* add additional window hints here ... */
	// I want a 4.3 context because GLSL 430 has explicit uniform locations and Im too lazy not to use those
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_DEPTH_BITS, 24);
	glfwWindowHint(GLFW_STENCIL_BITS, 8);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

	window = glfwCreateWindow(1280, 720, "Car Demo", NULL, NULL);
	if (!window)
	{
		fprintf(stderr, "ERROR: GLFW failed to open window .. aborting\n");
		abort();
	}

	glfwMakeContextCurrent(window);

	int gladOk = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	if (!gladOk)
	{
		fprintf(stderr, "ERROR: GLAD failed to load OpenGL functions .. aborting\n");
		abort();
	}

	printf("using OpenGL %s: %s\n",
		(const char *)glGetString(GL_VERSION),
		(const char *)glGetString(GL_RENDERER));

	if (GLVersion.major < 4 || (GLVersion.major == 4 && GLVersion.minor < 3))
	{
		fprintf(stderr, "ERROR: need at least OpenGL 4.3 to run .. aborting\n");
		abort();
	}

#ifndef NDEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(onGlError, NULL);
#endif

	glfwSetFramebufferSizeCallback(window, onFramebufferResized);
	glfwSetKeyCallback(window, onKey);
	glfwSetMouseButtonCallback(window, onMouseButton);
	glfwSetCursorPosCallback(window, onMouseMove);
	glfwSetScrollCallback(window, onMouseWheel);

	timerPeriod = 1.0 / glfwGetTimerFrequency();
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
	double mx, my;
	glfwGetCursorPos(window, &mx, &my);
}

void startGameLoop(std::function<void(double deltaTime)> frameCallback)
{
	uint64_t time0 = glfwGetTimerValue();

	while (!glfwWindowShouldClose(window))
	{
		int mouseXBefore = mouseX;
		int mouseYBefore = mouseY;
		mouseWheelDelta = 0;
		glfwPollEvents();
		mouseDeltaX = mouseX - mouseXBefore;
		mouseDeltaY = mouseY - mouseYBefore;

		uint64_t time1 = glfwGetTimerValue();
		double deltaTime = getDeltaTime(time0, time1);
		frameCallback(deltaTime);
		time0 = time1;
	}
}

double getDeltaTime(uint64_t time1, uint64_t time2)
{
	return (time2 - time1) * timerPeriod;
}