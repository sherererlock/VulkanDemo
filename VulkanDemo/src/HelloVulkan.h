#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include<vector>

struct QueueFamilyIndices
{
    int graphicsFamily = -1;

    bool isComplete() {
        return graphicsFamily >= 0;
    }
};

class HelloVulkan
{
public:
    void Init();
	void Run();
    void Cleanup();

private:
    void InitWindow();
    void InitVulkan();
    void MainLoop();
    void CreateInstance();

    bool CheckExtensionSupport(const char** extension);
    bool checkValidationLayerSupport();

    void setupDebugCallback();
    VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);
    void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);    

    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    std::vector<const char*> getRequiredExtensions();
private:
    GLFWwindow* window;
    VkInstance instance;
    VkDebugReportCallbackEXT callback;

    const int width = 1280;
    const int height = 720;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
};

