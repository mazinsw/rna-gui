#include "Input.h"
#include <raylib.h>
#include <stddef.h>

// o raylib embute o GLFW mas não distribui seus headers
extern "C"
{
	typedef struct GLFWwindow GLFWwindow;
	typedef void (*GLFWmousebuttonfun)(GLFWwindow* window, int button, int action, int mods);
	GLFWwindow* glfwGetCurrentContext(void) __attribute__((weak));
	GLFWmousebuttonfun glfwSetMouseButtonCallback(GLFWwindow* window,
		GLFWmousebuttonfun callback) __attribute__((weak));
}

namespace input
{
	static bool installed = false;
	static GLFWmousebuttonfun raylibCallback = NULL;
	static int pendingPresses = 0;
	static int pendingReleases = 0;
	static bool pressed = false;
	static bool released = false;

	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		if(button == MOUSE_BUTTON_LEFT)
		{
			if(action == 1) // GLFW_PRESS
				pendingPresses++;
			else if(action == 0) // GLFW_RELEASE
				pendingReleases++;
		}
		if(raylibCallback != NULL)
			raylibCallback(window, button, action, mods);
	}

	void install()
	{
		// GetWindowHandle() retorna a janela nativa (NSWindow no macOS), não a do GLFW
		if(glfwGetCurrentContext == NULL || glfwSetMouseButtonCallback == NULL)
			return;
		GLFWwindow* window = glfwGetCurrentContext();
		if(window == NULL)
			return;
		raylibCallback = glfwSetMouseButtonCallback(window, mouseButtonCallback);
		installed = true;
	}

	void beginFrame()
	{
		if(!installed)
		{
			pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
			released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
			return;
		}
		pressed = pendingPresses > 0;
		released = pendingReleases > 0;
		pendingPresses = 0;
		pendingReleases = 0;
	}

	bool mousePressed()
	{
		return pressed;
	}

	bool mouseReleased()
	{
		return released;
	}

	bool mouseDown()
	{
		return IsMouseButtonDown(MOUSE_BUTTON_LEFT);
	}
}
