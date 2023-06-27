#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm/gtx/hash.hpp>

#include<vector>
#include <array>
#include <string>
#include "gltfModel.h"
#include "camera.hpp"
#include "Shadow.h"
#include "Debug.h"

struct QueueFamilyIndices
{
    int graphicsFamily = -1;
    int presentFamily = -1;

    bool isComplete() {
        return graphicsFamily >= 0 && presentFamily >= 0;
    }
};

struct SwapChainSupportDetails 
{
    VkSurfaceCapabilitiesKHR capabilities ={};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 depthVP;
    glm::vec4 viewPos;
};

struct UBOParams {
	glm::vec4 lights[4];

};

struct PipelineCreateInfo
{
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineColorBlendStateCreateInfo colorBlending;
    VkPipelineDynamicStateCreateInfo dynamicState;
    VkPipelineDepthStencilStateCreateInfo depthStencil;

    PipelineCreateInfo() : vertexInputInfo({}), inputAssembly({}), viewportState({}), rasterizer({}),
        multisampling({}), colorBlending({}), dynamicState({}), depthStencil({})
    {        
    }
};

class HelloVulkan
{
public:
    HelloVulkan();

    static HelloVulkan* GetHelloVulkan()
    {
        return helloVulkan;
    }

    void Init();
	void Run();
    void Cleanup();

    void InitWindow();
    void InitVulkan();
    void MainLoop();

    void CreateInstance();
    void CreateDevice();
    void CreateSurface();
    void createSwapChain();
    void createImageViews();

    void createRenderPass();
    PipelineCreateInfo CreatePipelineCreateInfo();
    void createGraphicsPipeline();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    std::array<VkPipelineShaderStageCreateInfo, 2> CreaterShader(std::string vertexFile, std::string fragmentFile);

    void createFrameBuffer();

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createUniformBuffer();
    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createDescriptorSet();
    void updateDescriptorSet(int colorIdx, int normalIdx, int roughnessIdx);

    void createCommandPool();
    void createCommandBuffers();
    void buildCommandBuffers();
    void createSemaphores();

    void createTextureImage();
    void generateMipmaps(VkImage image, int32_t texWidth, VkFormat imageFormat,int32_t texHeight, uint32_t mipLevels);
    void createTextureImageView();
    void createTextureSampler(VkSampler& sampler, VkFilter magFilter, VkFilter minFilter, uint32_t mipLevels);
    void createTextureSampler();
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t miplevels, VkSampleCountFlagBits  numSamples);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t miplevels);

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t miplevels);

    void updateUniformBuffer();
    void updateSceneUniformBuffer(float frameTimer);
    void drawFrame();

    void recreateSwapChain();
    void cleanupSwapChain();

    void createDepthResources();

    void createColorResources();

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    bool checkValidationLayerSupport();
    bool checkDeviceExtensionSupport(VkPhysicalDevice phydevice);

    VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);
    void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void setupDebugMessenger();

    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice phyDevice);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    std::vector<const char*> getRequiredExtensions();

    VkSampleCountFlagBits getMaxUsableSampleCount();

    static void onWindowResized(GLFWwindow* window, int width, int height);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void MouseButtonCallback(GLFWwindow* window, int key, int action, int mods);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    void MouseCallback(double xpos, double ypos);

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    VkFormat findDepthFormat();

    bool hasStencilComponent(VkFormat format);

    void loadModel();

    void loadgltfModel(std::string filename);
    void AddLight(std::vector<uint32_t>& indexBuffer, std::vector<Vertex1>& vertexBuffer);

    float debugtimer = 0.5f;
    bool isOrth = true;
    void UpdateDebug();
    void UpdateProjectionMatrix();

    VkDevice GetDevice()
    {
        return device;
    }

private:
    GLFWwindow* window;
    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    VkRenderPass renderPass;

    VkDescriptorPool descriptorPool;

    VkDescriptorSetLayout descriptorSetLayoutM;
    VkDescriptorSet descriptorSetM;

    VkDescriptorSetLayout descriptorSetLayoutS;
    VkDescriptorSet descriptorSetS;

    VkDescriptorSetLayout descriptorSetLayoutMa;

    VkPushConstantRange pushConstantRange;
    VkPipelineLayout pipelineLayout;

    VkPipeline graphicsPipeline;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;

    VkBuffer uniformBufferL;
    VkDeviceMemory uniformBufferMemoryL;

    VkSampler textureSampler;
    uint32_t mipLevels;
    VkImage textureImage;
    VkImageView textureImageView;
    VkDeviceMemory textureImageMemory;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    int width = 1920;
    int height = 1080;

    const std::string MODEL_PATH = "D:/Games/VulkanDemo/VulkanDemo/models/buster_drone/busterDrone.gltf";
    const std::string TEXTURE_PATH = "textures/image_512.jpg";

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    size_t currentFrame = 0;
    const int Max_Frames_In_Fight = 2;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    static HelloVulkan* helloVulkan;

    // Model

    gltfModel gltfModel;

    float frameTimer = 1.0f;

	// Defines a frame rate independent timer value clamped from -1.0...1.0
	// For use in animations, rotations, etc.
	float timer = 0.0f;
	// Multiplier for speeding up (or slowing down) the global timer
	float timerSpeed = 0.25f;

    Camera camera;
    glm::vec2 mousePos;
    bool viewUpdated = false;

    struct {
		bool left = false;
		bool right = false;
		bool middle = false;
	} mouseButtons;


    //glm::vec4 lightPos = {0.0f, 40.0f, -4.0f, 1.0f};

    glm::vec4 lightPos;
    Node* lightNode;

    float zNear = 1.0f;
    float zFar = 96.0f;
    Shadow shadow;
    Debug debug;
    bool isDebug = true;
};

