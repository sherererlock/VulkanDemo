#pragma once

#include<vector>
#include <array>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm/gtx/hash.hpp>

#include "gltfModel.h"
#include "camera.hpp"
#include "Debug.h"
#include "Input.h"
#include "macros.h"
#include "SkyboxRenderer.h"
#include "PBRLighting.h"
#include "Shadow.h"

class Shadow;
class ReflectiveShadowMap;
class SSAO;
class SSRGBufferRenderer;
class GenHierarchicalDepth;
class SSR;
class PBRTest;
class BasePass;
class LightingPass;
class TAA;
struct FrameBufferAttachment;
class PostProcess;

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
    glm::vec4 viewPos;
};

struct UBOParams {
	glm::vec4 lights[4];
	float exposure = 4.5f;
	float gamma = 2.2f;
};

class HelloVulkan
{
private:
	
    struct IBLEnviromentLight
    {
        TextureCubeMap irradianceCube;
        TextureCubeMap prefilteredMap;
        Texture2D BRDFLutMap;
        inline void Cleanup()
        {
            irradianceCube.destroy();
            prefilteredMap.destroy();
            BRDFLutMap.destroy();
        }
    };

public:
    HelloVulkan();

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
    void createUniformBuffer();
    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createDescriptorSet();

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void createCommandPool();
    void createCommandBuffers();
    void buildCommandBuffers();
    void createSemaphores();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void generateMipmaps(VkImage image, int32_t texWidth, VkFormat imageFormat,int32_t texHeight, uint32_t mipLevels);
    void createTextureSampler(VkSampler& sampler, VkFilter magFilter, VkFilter minFilter, uint32_t mipLevels, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t miplevels, VkSampleCountFlagBits  numSamples, uint32_t layers = 1, VkImageCreateFlags flag = 0);
    void createImageView(VkImageView& view, VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t miplevels, VkImageViewType viewtype = VK_IMAGE_VIEW_TYPE_2D, uint32_t layers = 1);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t miplevels);
	void transitionImageLayout(
		VkCommandBuffer cmdBuffer,
        VkImage image,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkImageSubresourceRange subresourceRange,
		VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    void updateUniformBuffer(float frameTimer);
    void updateLight(float frameTimer);
    void updateSceneUniformBuffer(float frameTimer);
    void drawFrame();

    void recreateSwapChain();
    void cleanupSwapChain();
    void createDepthResources();
    void createColorResources();
    void createEmptyTexture();

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    bool checkValidationLayerSupport();
    bool checkDeviceExtensionSupport(VkPhysicalDevice phydevice);
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice phyDevice);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    std::vector<const char*> getRequiredExtensions();
    VkSampleCountFlagBits getMaxUsableSampleCount();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);

    void loadgltfModel(std::string filename, gltfModel& gltfmodel);
    Node* AddLight(std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);

	bool isDebug = false;
	float accTime = 0.0f;
    float debugtimer = 0.5f;
    bool isOrth = false;
    bool move = false;
    void UpdateDebug();
    void UpdateDebug(int key);
    void UpdateProjectionMatrix();
	void UpdateShadowIndex(int indx = -1);
	void UpdateShadowFilterSize();

	static HelloVulkan* GetHelloVulkan() { return helloVulkan; }
	inline float GetNear() const { return zNear; }
	inline float GetFar() const { return zFar; }
    inline VkDevice GetDevice() { return device; }
    inline Camera& GetCamera() { return camera; }
    inline Input& GetInput() { return input; }
    inline void ViewUpdated() { viewUpdated = true; }
    inline VkDescriptorSetLayout GetMaterialDescriptorSetLayout() const { return pbrLighting->GetDescriptorSetLayout(); }
    inline const gltfModel& GetSkybox() const { return skyboxRenderer->GetSkybox(); }
    inline std::vector<VkDescriptorSetLayout> GetDescriptorSetLayouts() const { return { descriptorSetLayoutM, descriptorSetLayoutS }; }
    inline std::array<VkDescriptorSet, 2> GetDescriptorSets() const { return { descriptorSetM, descriptorSetS }; }
    inline VkFormat GetFormat() const { return swapChainImageFormat; }

    inline const Texture2D& GetEmptyTexture() const { return emptyTexture; }
    inline VkSampleCountFlagBits GetSampleCountFlag() const { return msaaSamples; }
    inline const Texture2D& GetEmu() const {return EmuMap; }
    inline const Texture2D& GetEavg() const {return EavgMap; }

    inline SSRGBufferRenderer* GetSSRGBuffer() const { return ssrGBuffer; }
    inline GenHierarchicalDepth* GetHierarchicalDepth() const { return hierarchicalDepth;  }
    inline Shadow* GetShadow() const { return shadow; }
    inline BasePass* GetBasePass() const { return basePass; }
    inline SkyboxRenderer* GetSkyboxRenderer() const { return skyboxRenderer; }
    inline int GetShadowIndex() const { return shadow->shadowIndex; }

    const FrameBufferAttachment* GetCurrentRenderTarget() const;
    const VkDescriptorImageInfo GetLastImageInfo() const;

    const std::vector<glm::vec4>& GetHaltonSequence() const { return haltonSequence;  }



private:
	static HelloVulkan* helloVulkan;

	float zNear;
	float zFar;
	int width = 1280;
	int height = 720;
    GLFWwindow* window;
    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physicalDevice;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;
	VkCommandPool commandPool;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    VkRenderPass renderPass;
    VkDescriptorPool descriptorPool;

    VkDescriptorSetLayout descriptorSetLayoutM;
    VkDescriptorSetLayout descriptorSetLayoutS;

    VkDescriptorSet descriptorSetM;
    VkDescriptorSet descriptorSetS;

    IBLEnviromentLight envLight;
    Texture2D EmuMap;
    Texture2D EavgMap;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;

    VkBuffer uniformBufferL;
    VkDeviceMemory uniformBufferMemoryL;

    uint32_t mipLevels;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    Texture2D emptyTexture;

    bool msaa = false;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    // for msaa
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;

    size_t currentFrame = 0;
    const int Max_Frames_In_Fight = 2;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    // Model

    gltfModel gltfmodel;

    float frameTimer = 1.0f;
	float timer = 0.0f;
	float timerSpeed = 0.25f;

    Camera camera;
    bool viewUpdated = false;

    glm::vec4 lightPos;
    glm::vec4 debugPos;

    Node* lightNode;
    SkyboxRenderer* skyboxRenderer;
    PBRLighting* pbrLighting;
    PBRTest* pbrTest;

    Shadow* shadow;
    Debug debug;
    Input input;

    ReflectiveShadowMap* rsm;
    SSAO* ssao;
    SSRGBufferRenderer* ssrGBuffer;
    GenHierarchicalDepth* hierarchicalDepth;
    SSR* ssr;
    BasePass* basePass;
    LightingPass* lightingPass;

    bool aa = false;
    TAA* taa;

    PostProcess* postProcess;

    std::vector<Renderer*> renderers;

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    std::vector<glm::vec4> haltonSequence;
    bool rotateCamera = false;
};

