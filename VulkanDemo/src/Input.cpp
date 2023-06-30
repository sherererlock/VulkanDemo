#include "Input.h"
#include "HelloVulkan.h"

void Input::onWindowResized(GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0) return;

	HelloVulkan* app = reinterpret_cast<HelloVulkan*>(glfwGetWindowUserPointer(window));
	app->recreateSwapChain();
}

void Input::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	HelloVulkan* vulkan = HelloVulkan::GetHelloVulkan();
	Camera& camera = vulkan->GetCamera();
	if (key == GLFW_KEY_W)
	{
		if (action == GLFW_PRESS)
			camera.keys.up = true;
		if (action == GLFW_RELEASE)
			camera.keys.up = false;
	}

	if (key == GLFW_KEY_S)
	{
		if (action == GLFW_PRESS)
			camera.keys.down = true;
		if (action == GLFW_RELEASE)
			camera.keys.down = false;
	}

	if (key == GLFW_KEY_A)
	{
		if (action == GLFW_PRESS)
			camera.keys.left = true;
		if (action == GLFW_RELEASE)
			camera.keys.left = false;
	}

	if (key == GLFW_KEY_D)
	{
		if (action == GLFW_PRESS)
			camera.keys.right = true;
		if (action == GLFW_RELEASE)
			camera.keys.right = false;
	}

	if (key == GLFW_KEY_Q)
	{
		vulkan->UpdateProjectionMatrix();
	}

	if (key == GLFW_KEY_UP)
	{
		vulkan->UpdateShadowIndex();
	}

	if (key == GLFW_KEY_LEFT)
	{
		vulkan->UpdateShadowFilterSize();
	}
}

void Input::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	HelloVulkan* vulkan = HelloVulkan::GetHelloVulkan();
	vulkan->GetInput().MouseCallback(xpos, ypos);
}

void Input::MouseButtonCallback(GLFWwindow* window, int key, int action, int mods)
{
	HelloVulkan* vulkan = HelloVulkan::GetHelloVulkan();
	Input& input = vulkan->GetInput();
	if (key == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			double xposition, yposition;
			glfwGetCursorPos(window, &xposition, &yposition);
			input.mousePos = glm::vec2((float)xposition, (float)yposition);
			input.mouseButtons.left = true;
		}
		else if (action == GLFW_RELEASE)
		{
			input.mouseButtons.left = false;
		}
	}

	if (key == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			double xposition, yposition;
			glfwGetCursorPos(window, &xposition, &yposition);
			input.mousePos = glm::vec2((float)xposition, (float)yposition);
			input.mouseButtons.right = true;
			vulkan->UpdateDebug();
		}
		else if (action == GLFW_RELEASE)
		{
			input.mouseButtons.right = false;
		}
	}

	if (key == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		if (action == GLFW_PRESS)
		{
			double xposition, yposition;
			glfwGetCursorPos(window, &xposition, &yposition);
			input.mousePos = glm::vec2((float)xposition, (float)yposition);

			input.mouseButtons.middle = true;
		}
		else if (action == GLFW_RELEASE)
		{
			input.mouseButtons.middle = false;
		}
	}
}

void Input::MouseCallback(double x, double y)
{
	int32_t dx = (int32_t)(mousePos.x - x);
	int32_t dy = (int32_t)(mousePos.y - y);
	Camera& camera = vulkanAPP->GetCamera();
	if (mouseButtons.left) {
		camera.rotate(glm::vec3(dy * camera.rotationSpeed, -dx * camera.rotationSpeed, 0.0f));
		vulkanAPP->ViewUpdated();

	}
	if (mouseButtons.right) {
		camera.translate(glm::vec3(-0.0f, 0.0f, dy * .005f));
		vulkanAPP->ViewUpdated();
	}
	if (mouseButtons.middle) {
		camera.translate(glm::vec3(-dx * 0.005f, -dy * 0.005f, 0.0f));
		vulkanAPP->ViewUpdated();
	}
	mousePos = glm::vec2((float)x, (float)y);
}

void Input::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	HelloVulkan* vulkan = HelloVulkan::GetHelloVulkan();

	vulkan->GetCamera().translate(glm::vec3(0.0f, 0.0f, (float)yoffset * 0.005f));
	vulkan->ViewUpdated();
}