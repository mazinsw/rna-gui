#include "Input.h"
#include <raylib.h>
#include <math.h>
#include <stddef.h>

// intervalo e distância máximos entre os cliques de um clique duplo
#define DOUBLE_CLICK_TIME 0.5
#define DOUBLE_CLICK_DISTANCE 4

// o raylib usa o GLFW mas não distribui seus headers. Fora do Windows as
// referências são fracas: um raylib dinâmico pode não exportar o GLFW. No
// Windows o GLFW sempre é linkado estático.
#if defined(_WIN32)
#define GLFW_IMPORT
#else
#define GLFW_IMPORT __attribute__((weak))
#endif

extern "C"
{
	typedef struct GLFWwindow GLFWwindow;
	typedef void (*GLFWmousebuttonfun)(GLFWwindow* window, int button, int action, int mods);
	GLFWwindow* glfwGetCurrentContext(void) GLFW_IMPORT;
	GLFWmousebuttonfun glfwSetMouseButtonCallback(GLFWwindow* window,
		GLFWmousebuttonfun callback) GLFW_IMPORT;
}

namespace input
{
	static bool installed = false;
	static GLFWmousebuttonfun raylibCallback = NULL;
	static int pendingPresses = 0;
	static int pendingReleases = 0;
	static bool pendingDoubleClick = false;
	static int pendingRightPresses = 0;
	static bool rightPressed = false;
	static bool pressed = false;
	static bool released = false;
	static bool doubleClicked = false;
	static double lastPressTime = -1;
	static Vector2 lastPressPosition = { 0, 0 };

	/**
	 * Registra um clique e verifica se ele completa um clique duplo
	 */
	static void registerPress()
	{
		double now = GetTime();
		Vector2 position = GetMousePosition();
		bool doubleClick = lastPressTime >= 0 && now - lastPressTime <= DOUBLE_CLICK_TIME &&
			fabsf(position.x - lastPressPosition.x) <= DOUBLE_CLICK_DISTANCE &&
			fabsf(position.y - lastPressPosition.y) <= DOUBLE_CLICK_DISTANCE;
		if(doubleClick)
			pendingDoubleClick = true;
		// o terceiro clique começa outra sequência
		lastPressTime = doubleClick ? -1 : now;
		lastPressPosition = position;
	}

	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		if(button == MOUSE_BUTTON_RIGHT && action == 1) // GLFW_PRESS
			pendingRightPresses++;
		if(button == MOUSE_BUTTON_LEFT)
		{
			if(action == 1) // GLFW_PRESS
			{
				pendingPresses++;
				registerPress();
			}
			else if(action == 0) // GLFW_RELEASE
				pendingReleases++;
		}
		if(raylibCallback != NULL)
			raylibCallback(window, button, action, mods);
	}

	void install()
	{
		// referências fracas ficam NULL quando o símbolo não existe
		GLFWwindow* (*getCurrentContext)(void) = glfwGetCurrentContext;
		GLFWmousebuttonfun (*setMouseButtonCallback)(GLFWwindow*, GLFWmousebuttonfun) =
			glfwSetMouseButtonCallback;
		if(getCurrentContext == NULL || setMouseButtonCallback == NULL)
			return;
		// GetWindowHandle() retorna a janela nativa (NSWindow no macOS), não a do GLFW
		GLFWwindow* window = getCurrentContext();
		if(window == NULL)
			return;
		raylibCallback = setMouseButtonCallback(window, mouseButtonCallback);
		installed = true;
	}

	void beginFrame()
	{
		if(!installed)
		{
			pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
			released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
			rightPressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
			if(pressed)
				registerPress();
		}
		else
		{
			pressed = pendingPresses > 0;
			released = pendingReleases > 0;
			rightPressed = pendingRightPresses > 0;
			pendingPresses = 0;
			pendingReleases = 0;
			pendingRightPresses = 0;
		}
		doubleClicked = pendingDoubleClick;
		pendingDoubleClick = false;
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

	bool mouseDoubleClicked()
	{
		return doubleClicked;
	}

	bool mouseRightPressed()
	{
		return rightPressed;
	}
}
