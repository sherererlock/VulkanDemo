#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm/glm.hpp>

class HelloVulkan;
class Input
{
private:
	HelloVulkan* vulkanAPP;

	glm::vec2 mousePos;
	struct {
		bool left = false;
		bool right = false;
		bool middle = false;
	} mouseButtons;

public:
	void Init(HelloVulkan* vulkan)
	{
		vulkanAPP = vulkan;
	}

	static void onWindowResized(GLFWwindow* window, int width, int height);
	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
	static void MouseButtonCallback(GLFWwindow* window, int key, int action, int mods);
	static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void MouseCallback(double xpos, double ypos);
};

