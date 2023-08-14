#include <stdexcept>
#include <iostream>
#include <set>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <unordered_map>

#define TINYGLTF_NO_STB_IMAGE_WRITE
//#define TINYGLTF_NO_STB_IMAGE

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include "HelloVulkan.h"
#include "Mesh.h"
#include "PBRLighting.h"
#include "SkyboxRenderer.h"
#include "CommonShadow.h"
#include "CascadedShadow.h"
#include "PreProcess.h"
#include "ReflectiveShadowMap.h"
#include "SSAO.h"
#include "SSR.h"
#include "SSRGBufferRenderer.h"
#include "GenHierarchicalDepth.h"
#include "PBRTest.h"

//#define IBLLIGHTING

//#define RSMLIGHTING

//#define SCREENSPACEAO

//#define SCREENSPACEREFLECTION

//#define SKYBOX

#define SHADOW

//#define PBRTESTING

#ifdef  IBLLIGHTING
#define SKYBOX
#endif //  SKYBOX

#ifdef  SCREENSPACEREFLECTION
#define SHADOW
#endif //  SSR


//const std::string MODEL_PATH = "D:/Games/VulkanDemo/VulkanDemo/models/sponza/sponza.gltf";
const std::string MODEL_PATH = "D:/Games/VulkanDemo/VulkanDemo/models/buster_drone/busterDrone.gltf";
//const std::string MODEL_PATH = "D:/Games/VulkanDemo/VulkanDemo/models/vulkanscene_shadow.gltf";
//const std::string MODEL_PATH = "D:/Games/VulkanDemo/VulkanDemo/models/sphere.gltf";

#define SHADOWMAP_SIZE 2048

HelloVulkan* HelloVulkan::helloVulkan = nullptr;

VkCommandBuffer HelloVulkan::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void HelloVulkan::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;

    VkFence fence;
    vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);

    vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
    vkDestroyFence(device, fence, nullptr);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

bool HelloVulkan::hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void HelloVulkan::loadgltfModel(std::string filename, gltfModel& model)
{
    tinygltf::Model glTFInput;
	tinygltf::TinyGLTF gltfContext;
	std::string error, warning;

	bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, filename);

	std::vector<uint32_t> indexBuffer;
	std::vector<Vertex> vertexBuffer;

	size_t pos = filename.find_last_of('/');
	std::string path = filename.substr(0, pos);

	if (fileLoaded) {
        model.logicalDevice = device;
        model.loadImages(glTFInput);
		model.loadTextures(glTFInput);
		model.loadMaterials(glTFInput);
		const tinygltf::Scene& scene = glTFInput.scenes[0];
		for (size_t i = 0; i < scene.nodes.size(); i++) {
			const tinygltf::Node node = glTFInput.nodes[scene.nodes[i]];
			model.loadNode(node, glTFInput, nullptr, indexBuffer, vertexBuffer);
		}
	}
	else {
		throw std::runtime_error("validation layers requested, but not available!");("Could not open the glTF file.\n\nThe file is part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.", -1);
	}

    //lightNode = AddLight(indexBuffer, vertexBuffer);
    //lightNode->matrix = glm::scale(lightNode->matrix, glm::vec3(0.01f));

	size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
	size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
	model.indices.count = static_cast<uint32_t>(indexBuffer.size());

	struct StagingBuffer {
		VkBuffer buffer;
		VkDeviceMemory memory;
	} vertexStaging, indexStaging;

	// Create host visible staging buffers (source)
    createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexStaging.buffer, vertexStaging.memory);

    createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indexStaging.buffer, indexStaging.memory);

    void* data;
    vkMapMemory(device, vertexStaging.memory, 0, vertexBufferSize, 0, &data);
    memcpy(data, vertexBuffer.data(), vertexBufferSize);
    vkUnmapMemory(device, vertexStaging.memory);

    void* data1;
    vkMapMemory(device, indexStaging.memory, 0, indexBufferSize, 0, &data1);
    memcpy(data1, indexBuffer.data(), indexBufferSize);
    vkUnmapMemory(device, indexStaging.memory);

    createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, model.vertices.buffer, model.vertices.memory);

    createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, model.indices.buffer, model.indices.memory);

    copyBuffer(vertexStaging.buffer, model.vertices.buffer, vertexBufferSize);

    copyBuffer(indexStaging.buffer, model.indices.buffer, indexBufferSize);

	vkDestroyBuffer(device, vertexStaging.buffer, nullptr);
	vkFreeMemory(device, vertexStaging.memory, nullptr);
	vkDestroyBuffer(device, indexStaging.buffer, nullptr);
	vkFreeMemory(device, indexStaging.memory, nullptr);
}

Node* HelloVulkan::AddLight(std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer)
{
    CubeMesh mesh;
    const uint32_t indexCount = (uint32_t) mesh.indices.size();

	uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
	uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
   
    for (int i = 0; i < mesh.indices.size(); i++)
        indexBuffer.push_back(mesh.indices[i] + (uint32_t)vertexBuffer.size());

    for (int i = 0; i < mesh.vertex.size(); i++)
        vertexBuffer.push_back(mesh.vertex[i]);
 
    Primitive primitive{};
	primitive.firstIndex = firstIndex;
	primitive.indexCount = indexCount;
	primitive.materialIndex = 0;

	Node* node = new Node{};
    node->matrix = glm::translate(node->matrix, glm::vec3(lightPos.x, lightPos.y, lightPos.z));
	node->mesh.primitives.push_back(primitive);

    gltfmodel.nodes.push_back(node);

    node->matrix = glm::scale(node->matrix, glm::vec3(0.01f));

    return node;
}

HelloVulkan::HelloVulkan()
{
    helloVulkan = this;

    isOrth = true;
	lightPos = glm::vec4(0.0f, 20.0f, 10.0f, 1.0f);
	//lightPos = glm::vec4(0.0f, 5.0f, 0.0f, 1.0f);

	zNear = 0.1f;
	zFar = 250.0f;

	width = 1280;
	height = 720;

	camera.type = Camera::CameraType::firstperson;


#ifdef IBLLIGHTING
	camera.setPosition(glm::vec3(0.0f, 0.0f, -2.1f));
	camera.setRotation(glm::vec3(-25.5f, 363.0f, 0.0f));
#else
	camera.setPosition(glm::vec3(0.0f, 1.0f, 0.0f));
	camera.setRotation(glm::vec3(0.0f, -90.0f, 0.0f));
#endif

	camera.setPosition(glm::vec3(0.0f, 1.0f, 0.0f));
	camera.setRotation(glm::vec3(0.0f, -90.0f, 0.0f));

	camera.setPosition(glm::vec3(0.0f, 0.0f, -2.1f));
	camera.setRotation(glm::vec3(-25.5f, 363.0f, 0.0f));

	//camera.setPosition(glm::vec3(10.0f, 13.0f, 1.8f));
	//camera.setRotation(glm::vec3(-62.5f, 90.0f, 0.0f));

	camera.movementSpeed = 4.0f;
    camera.flipY = true;
	camera.setPerspective(60.0f, (float)width / (float)height, zNear, zFar);
	camera.rotationSpeed = 0.25f;
    viewUpdated = true;

    renderers.reserve(10);

#ifdef  SKYBOX
    skyboxRenderer = new SkyboxRenderer();
    renderers.push_back(skyboxRenderer);
#endif //  SKYBOX

#ifdef RSMLIGHTING
    rsm = new ReflectiveShadowMap();
    renderers.push_back(rsm);
#endif

#ifdef SHADOW
	if (CASCADED_COUNT > 1)
		shadow = new CascadedShadow();
	else
		shadow = new CommonShadow();
#endif


#ifdef SCREENSPACEAO
    ssao = new SSAO();
    renderers.push_back(ssao);
#endif

#ifdef SCREENSPACEREFLECTION
    ssrGBuffer = new SSRGBufferRenderer();
    hierarchicalDepth = new GenHierarchicalDepth();
    ssr = new SSR();

    renderers.push_back(ssrGBuffer);
    renderers.push_back(hierarchicalDepth);
    renderers.push_back(ssr);
#else

#ifdef PBRTESTING
    pbrTest = new PBRTest();
    renderers.push_back(pbrTest);
#else
    pbrLighting = new PBRLighting();
    renderers.push_back(pbrLighting);
#endif

#endif //  SSR

}

void HelloVulkan::Init()
{
	InitWindow();
	InitVulkan();
}

void HelloVulkan::Run()
{
	MainLoop();
}

void HelloVulkan::InitWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetWindowSizeCallback(window, Input::onWindowResized);
    glfwSetKeyCallback(window, Input::KeyCallback);
    glfwSetCursorPosCallback(window, Input::MouseCallback);
    glfwSetMouseButtonCallback(window, Input::MouseButtonCallback);
    glfwSetScrollCallback(window, Input::ScrollCallback);

    input.Init(this);
}

void HelloVulkan::InitVulkan()
{
    CreateInstance();
    debug.setupDebugMessenger(instance);

    CreateSurface();
    pickPhysicalDevice();
    CreateDevice();

    debug.Init(device, this, zNear, zFar);

    for(Renderer* render : renderers)
        render->Init(this, device, width, height);

    #ifdef SHADOW
	shadow->Init(this, device, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
    #endif

    createSwapChain();
    createImageViews();

    createRenderPass();
    
    for (Renderer* render : renderers)
    {
        render->CreateGBuffer();
        render->CreatePass();
    }

    #ifdef SHADOW
    shadow->CreateShadowPass();
    #endif

#ifdef SCREENSPACEREFLECTION
    hierarchicalDepth->CreateMipMap();
#endif //  SSR

    createDescriptorSetLayout();

    debug.CreateDescriptSetLayout();

    for (Renderer* render : renderers)
        render->CreateDescriptSetLayout();

    #ifdef SHADOW
    shadow->CreateDescriptSetLayout();
    #endif

    createGraphicsPipeline();

    createCommandPool();
    createColorResources();
    createDepthResources();
	createEmptyTexture();

    createFrameBuffer();

    for (Renderer* render : renderers)
        render->CreateFrameBuffer();

    #ifdef SHADOW
	shadow->CreateShadowMap();
    shadow->CreateFrameBuffer();
    #endif

    createUniformBuffer();
    debug.CreateUniformBuffer();

    for (Renderer* render : renderers)
        render->CreateUniformBuffer();

    #ifdef SHADOW
    shadow->CreateUniformBuffer();
    #endif

#ifdef  SKYBOX
	skyboxRenderer->LoadSkyBox();
#endif //  SKYBOX

    loadgltfModel(MODEL_PATH, gltfmodel);

#ifdef SCREENSPACEREFLECTION
    ssrGBuffer->AddModel(&gltfmodel);
#else
    if (pbrTest)
        pbrTest->AddModel(&gltfmodel);

    if(pbrLighting)
        pbrLighting->AddModel(&gltfmodel);
#endif //  SSR

#ifdef IBLLIGHTING
	PreProcess::generateIrradianceCube(this, skyboxRenderer->GetCubemap(), envLight.irradianceCube);
	PreProcess::prefilterEnvMap(this, skyboxRenderer->GetCubemap(), envLight.prefilteredMap);
	PreProcess::genBRDFLut(this, envLight.BRDFLutMap);
#endif

    PreProcess::genBRDFMissLut(this, EmuMap, EavgMap);

	createDescriptorPool();
    createDescriptorSet();

    for (Renderer* render : renderers)
        render->SetupDescriptSet(descriptorPool);

    #ifdef SHADOW
    shadow->SetupDescriptSet(descriptorPool);
    #endif

    createCommandBuffers();

    createSemaphores();

    updateSceneUniformBuffer(0.0f);
}

void HelloVulkan::MainLoop()
{
    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();

        auto tStart = std::chrono::high_resolution_clock::now();

        buildCommandBuffers();

        drawFrame();

        auto tEnd = std::chrono::high_resolution_clock::now();

        auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

        frameTimer = (float)tDiff / 1000.0f;
	    camera.update(frameTimer);
	    if (camera.moving())
	    {
		    viewUpdated = true;
	    }

        updateLight(frameTimer);
		updateSceneUniformBuffer(frameTimer);
        //if (viewUpdated)
        {
            updateUniformBuffer(frameTimer);
            viewUpdated = false;
        }

		timer += timerSpeed * frameTimer;
		if (timer > 1.0)
		{
			timer -= 1.0f;
		}

        if (debugtimer > 0.0f)
        {
            debugtimer -= frameTimer;
        }
    }

    vkDeviceWaitIdle(device);
}

void HelloVulkan::Cleanup()
{
    cleanupSwapChain();

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayoutS, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayoutM, nullptr);

    vkDestroyBuffer(device, uniformBuffer, nullptr);
    vkFreeMemory(device, uniformBufferMemory, nullptr);

    vkDestroyBuffer(device, uniformBufferL, nullptr);
    vkFreeMemory(device, uniformBufferMemoryL, nullptr);

    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

    vkDestroyCommandPool(device, commandPool, nullptr);

    gltfmodel.Cleanup();

    for (Renderer* render : renderers)
        render->Cleanup();

#ifdef IBLLIGHTING
    envLight.Cleanup();
#endif

    EmuMap.destroy();
    EavgMap.destroy();

    debug.Cleanup(instance);
    emptyTexture.destroy();

    #ifdef SHADOW
    shadow->Cleanup();
    #endif

    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();

    delete pbrLighting;
    delete skyboxRenderer;
    delete shadow;
    delete rsm;
    delete ssao;
    delete ssrGBuffer;
    delete hierarchicalDepth;
    delete ssr;
    delete pbrTest;
}

void HelloVulkan::CreateInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0,0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0,0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // 对整个程序都有效的扩展和layer
    // VK_KHR_Surface
    // VK_KHR_win32_surface
    // VK_EXT_debug_utils
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) 
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        debug.populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    }
    else 
    {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create instance!");
    }
}

void HelloVulkan::CreateDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;
    deviceFeatures.imageCubeArray = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    createInfo.pEnabledFeatures = &deviceFeatures;

    // 适配于device的extension
    // VK_KHR_swapchain
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
}

void HelloVulkan::CreateSurface()
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
}

void HelloVulkan::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    // surface的format,包括图像的colorspace和format
    // presentmode 三缓冲
    // extent 图像分辨率
    // 图像的数量
    // 队列对于图像的使用：呈现队列和渲染队列

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

    if (indices.graphicsFamily != indices.presentFamily) 
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    // 获取交换链中的vkImage

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void HelloVulkan::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) 
    {
        createImageView(swapChainImageViews[i], swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void HelloVulkan::createRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = msaaSamples; // 采样数
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // 存储下来

    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0; // 引用的附件在数组中的索引
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve = {};
    colorAttachmentResolve.format = swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentResolveRef = {};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // 图像渲染的子流程

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef; // fragment shader使用 location = 0 outcolor,输出
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // 开始之前的pass
    dependency.dstSubpass = 0; // 第一个subpass

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // A pass layout 转换的阶段
    dependency.srcAccessMask = 0; // A pass layout 转换的阶段的操作之后就可以进行转换

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // B pass 等待执行的阶段
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; //B pass 等待执行的操作

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

PipelineCreateInfo HelloVulkan::CreatePipelineCreateInfo()
{
    PipelineCreateInfo pipelineCreateInfo;
    
    pipelineCreateInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineCreateInfo.vertexInputInfo.vertexBindingDescriptionCount = 1;

    pipelineCreateInfo.inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipelineCreateInfo.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineCreateInfo.inputAssembly.primitiveRestartEnable = VK_FALSE;

    pipelineCreateInfo.viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipelineCreateInfo.viewportState.viewportCount = 1;
    pipelineCreateInfo.viewportState.scissorCount = 1;

    pipelineCreateInfo.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineCreateInfo.rasterizer.depthClampEnable = VK_FALSE;
    pipelineCreateInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // VK_POLYGON_MODE_LINE
    //pipelineCreateInfo.rasterizer.polygonMode = VK_POLYGON_MODE_LINE; // VK_POLYGON_MODE_LINE

    pipelineCreateInfo.rasterizer.lineWidth = 1.0f;

    pipelineCreateInfo.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineCreateInfo.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    pipelineCreateInfo.rasterizer.depthBiasEnable = VK_FALSE;
    pipelineCreateInfo.rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    pipelineCreateInfo.rasterizer.depthBiasClamp = 0.0f; // Optional
    pipelineCreateInfo.rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    pipelineCreateInfo.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineCreateInfo.multisampling.sampleShadingEnable = VK_FALSE;
    // pipelineCreateInfo.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineCreateInfo.multisampling.rasterizationSamples = msaaSamples;
    pipelineCreateInfo.multisampling.minSampleShading = 0.0f; // Optional
    pipelineCreateInfo.multisampling.pSampleMask = nullptr; // Optional
    pipelineCreateInfo.multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    pipelineCreateInfo.multisampling.alphaToOneEnable = VK_FALSE; // Optional

    pipelineCreateInfo.colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipelineCreateInfo.colorBlending.logicOpEnable = VK_FALSE;
    pipelineCreateInfo.colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    pipelineCreateInfo.colorBlending.attachmentCount = 1;
    pipelineCreateInfo.colorBlending.blendConstants[0] = 0.0f; // Optional
    pipelineCreateInfo.colorBlending.blendConstants[1] = 0.0f; // Optional
    pipelineCreateInfo.colorBlending.blendConstants[2] = 0.0f; // Optional
    pipelineCreateInfo.colorBlending.blendConstants[3] = 0.0f; // Optional

    pipelineCreateInfo.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineCreateInfo.depthStencil.depthTestEnable = VK_TRUE;
    pipelineCreateInfo.depthStencil.depthWriteEnable = VK_TRUE;
    pipelineCreateInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    return pipelineCreateInfo;
}

void HelloVulkan::createGraphicsPipeline()
{
    std::string vertexFileName = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/shader.vert.spv";
    std::string fragmentFileName = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/shader.frag.spv";

#ifdef IBLLIGHTING
    fragmentFileName = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/shader_ibl.frag.spv";
#endif

#ifdef RSMLIGHTING
    fragmentFileName = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/shader_rsm.frag.spv";
#endif

#ifdef SCREENSPACEAO
    fragmentFileName = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/shader_ssao.frag.spv";
#endif

    if (CASCADED_COUNT > 1)
    {
		vertexFileName = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/shaderCascaded.vert.spv";
		fragmentFileName = "D:/Games/VulkanDemo/VulkanDemo/shaders/GLSL/spv/shaderCascaded.frag.spv";
    }


    PipelineCreateInfo info = CreatePipelineCreateInfo();
    
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    info.dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    info.dynamicState.dynamicStateCount = 2;
    info.dynamicState.pDynamicStates = dynamicStates;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    info.colorBlending.pAttachments = &colorBlendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.renderPass = renderPass;

#ifndef SCREENSPACEREFLECTION
    if(pbrLighting != nullptr)
        pbrLighting->SetShaderFile(vertexFileName, fragmentFileName);
#endif //  SSR

    info.Apply(pipelineInfo);
    debug.CreateDebugPipeline(info, pipelineInfo);

    for(Renderer* renderer : renderers)
        renderer->CreatePipeline(info, pipelineInfo);

    #ifdef SHADOW
    shadow->CreateShadowPipeline(info, pipelineInfo);
    #endif
}

void HelloVulkan::createFrameBuffer()
{
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++)
    {
        std::array<VkImageView, 3> attachments = {
            colorImageView,
            depthImageView,
            swapChainImageViews[i]            
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void HelloVulkan::createUniformBuffer()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformBufferMemory);

    bufferSize = sizeof(UBOParams);
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBufferL, uniformBufferMemoryL);
}

void HelloVulkan::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool!");
    }
}

void HelloVulkan::createCommandBuffers()
{
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }

}

void HelloVulkan::buildCommandBuffers()
{
    PreProcess::genBRDFMissLut(this, EmuMap, EavgMap);
    for (size_t i = 0; i < commandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr; // Optional

        vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

        {
            #ifdef RSMLIGHTING
            rsm->BuildCommandBuffer(commandBuffers[i], gltfmodel);
            #endif

            #ifdef SHADOW
            shadow->BuildCommandBuffer(commandBuffers[i], gltfmodel);
            #endif
        }

        {
            #ifdef SCREENSPACEAO
            ssao->BuildCommandBuffer(commandBuffers[i], gltfmodel);
            #endif
        }

        {
            #ifdef SCREENSPACEREFLECTION
            ssrGBuffer->BuildCommandBuffer(commandBuffers[i], gltfmodel);
            //hierarchicalDepth->BuildCommandBuffer(commandBuffers[i], gltfmodel);
            #endif //  SSR
        }

        {
            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapChainFramebuffers[i];

            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapChainExtent;

            std::array<VkClearValue, 2> clearValues = {};
            clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
            clearValues[1].depthStencil = {1.0f, 0};

            VkViewport viewport = {};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float) swapChainExtent.width;
            viewport.height = (float) swapChainExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor = {};
            scissor.offset = {0, 0};
            scissor.extent = swapChainExtent;

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);
            vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);

            if (isDebug)
            {
                debug.BuildCommandBuffer(commandBuffers[i]);
            }
            else
            {
                #ifdef  SKYBOX
				skyboxRenderer->BuildCommandBuffer(commandBuffers[i], gltfmodel);
                #endif //  SKYBOX

                #ifdef SCREENSPACEREFLECTION
                    ssr->BuildCommandBuffer(commandBuffers[i], gltfmodel);
                #else
                if (pbrLighting != nullptr)
				    pbrLighting->BuildCommandBuffer(commandBuffers[i], gltfmodel);
                #endif //  SSR

                #ifdef PBRTESTING
                pbrTest->BuildCommandBuffer(commandBuffers[i], gltfmodel);
                #endif
            }

            vkCmdEndRenderPass(commandBuffers[i]);
        }

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void HelloVulkan::createSemaphores()
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
    vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create semaphores!");
    }

}

void HelloVulkan::generateMipmaps(VkImage image, int32_t texWidth, VkFormat imageFormat,int32_t texHeight, uint32_t mipLevels) 
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) 
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit = {};

        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    endSingleTimeCommands(commandBuffer);
}

void HelloVulkan::updateUniformBuffer(float frameTimer)
{
    UniformBufferObject ubo = {};

    ubo.view = camera.matrices.view;
    ubo.proj = camera.matrices.perspective;
    ubo.viewPos = camera.viewPos;

    glm::vec3 pos = {lightPos.x, lightPos.y, lightPos.z};
    glm::mat4 view = glm::lookAt(pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	float range = 5.0f;
	glm::mat4 ortho = glm::ortho(-range, range, -range, range, zNear, zFar);
    glm::mat4 pers = glm::perspective(glm::radians(45.0f), 1.0f, zNear, zFar);

    pers[1][1] *= -1; // flip Y
    ortho[1][1] *= -1; // flip Y

    glm::mat4 proj = isOrth ? ortho : pers;

#ifdef SKYBOX
	skyboxRenderer->UpateLightMVP(camera.matrices.view, camera.matrices.perspective, lightPos);
#endif //  SKYBOX

#ifdef RSMLIGHTING
	rsm->UpateLightMVP(view, proj, lightPos, zNear, zFar);
#endif

#ifdef SHADOW
    shadow->UpateLightMVP(view, proj, lightPos);
#endif

#ifdef SCREENSPACEAO
    ssao->UpateLightMVP(camera.matrices.view, camera.matrices.perspective, camera.viewPos, camera.getNearClip(), camera.getFarClip());
#endif

#ifdef SCREENSPACEREFLECTION
    ssr->UpateLightMVP(camera.matrices.view, camera.matrices.perspective, proj * view, camera.viewPos);
#endif //  SSR

#ifdef PBRTESTING
	pbrTest->UpateLightMVP(camera.matrices.view, camera.matrices.perspective, lightPos);
#endif

    void* data;
    vkMapMemory(device, uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
    memcpy(data, &ubo, sizeof(UniformBufferObject));
    vkUnmapMemory(device, uniformBufferMemory);

    //glm::mat4 cameratoworld = glm::inverse(view);
    //lightNode->matrix = cameratoworld;
    //lightNode->matrix = glm::scale(lightNode->matrix, glm::vec3(0.01f));
}

void HelloVulkan::updateLight(float frameTimer)
{
    glm::mat4 rotation;
    glm::vec3 yaxis(0.0f, 1.0f, 0.0f);
    constexpr float speed = glm::radians(35.0f);
    rotation = glm::rotate(rotation, speed * frameTimer, yaxis);

    //lightPos = rotation * lightPos;
}

void HelloVulkan::updateSceneUniformBuffer(float frameTimer)
{
    auto RotateLight = [](float angle) {
        glm::mat4 rotation;
        glm::vec3 yaxis(0.0f, 1.0f, 0.0f);
        float rot = glm::radians(angle);
        rotation = glm::rotate(rotation, rot, yaxis);

        return rotation;
    };

    UBOParams uboparams = {};
	uboparams.lights[0] = lightPos;
    
    uboparams.lights[1] = RotateLight(45.0f) * lightPos;
    uboparams.lights[2] = RotateLight(90.0f) * lightPos;
    uboparams.lights[3] = RotateLight(135.0f) * lightPos;
 
    void* data;
    vkMapMemory(device, uniformBufferMemoryL, 0, sizeof(uboparams), 0, &data);
    memcpy(data, &uboparams, sizeof(uboparams));
    vkUnmapMemory(device, uniformBufferMemoryL);
}

void HelloVulkan::drawFrame()
{
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) 
    {
        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    vkQueueWaitIdle(presentQueue);
}

void HelloVulkan::recreateSwapChain()
{
    int width = 0, height = 0;
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device); // 等待对象都使用完毕

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createColorResources();
    createDepthResources();
    createFrameBuffer();
    createCommandBuffers();
}

void HelloVulkan::cleanupSwapChain()
{
    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);

    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
    {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
    }

    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    vkDestroyRenderPass(device, renderPass, nullptr);

    for (size_t i = 0; i < swapChainImageViews.size(); i++)
    {
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void HelloVulkan::createDepthResources()
{
    VkFormat depthFormat = findDepthFormat();
    //createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory, 1, VK_SAMPLE_COUNT_1_BIT);

    createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory, 1, msaaSamples);

    createImageView(depthImageView, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

void HelloVulkan::createColorResources() {
    VkFormat colorFormat = swapChainImageFormat;

    createImage(swapChainExtent.width, swapChainExtent.height, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory,  1, msaaSamples);

    createImageView(colorImageView, colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    transitionImageLayout(colorImage, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
}

void HelloVulkan::createEmptyTexture()
{
    float color = 0.0f;
	VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    emptyTexture.fromBuffer(this, &color, 4, VK_FORMAT_R8G8B8A8_UNORM, 1, 1, VK_FILTER_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
}

void HelloVulkan::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes = {};

    // TODO: create layout for sky box
    //7: model, scene, skybox(2), shadow, debug rsm(2) ssao(3)
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(gltfmodel.materials.size()) + 11 + 12;

    // ma * 4 + (s * 4) + (s * 4)  + ssao(4)
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(gltfmodel.materials.size()) * 4 + 4 + 12 + 12;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    // skybox:2 helloVulkan:2 shadow: 1 debug 1 rsm:1 ssao:3
    poolInfo.maxSets = static_cast<uint32_t>(gltfmodel.materials.size()) + 10 + 12;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void HelloVulkan::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uniformLayoutBinding = {};

    uniformLayoutBinding.binding = 0;
    uniformLayoutBinding.descriptorCount = 1;
    uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformLayoutBinding.pImmutableSamplers = nullptr;
    uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> uniformLayoutBindings = {uniformLayoutBinding, uniformLayoutBinding};
    uniformLayoutBindings[1].binding = 1;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = (uint32_t) uniformLayoutBindings.size();
    layoutInfo.pBindings = uniformLayoutBindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayoutM) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding imageLayoutBinding = {};
    imageLayoutBinding.binding = 1;
    imageLayoutBinding.descriptorCount = 1;
    imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    imageLayoutBinding.pImmutableSamplers = nullptr;
    imageLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    int binding = 0;
    int bindingcount = 4;
    std::array<VkDescriptorSetLayoutBinding, 4> scenebindings = { uniformLayoutBinding, imageLayoutBinding, imageLayoutBinding, imageLayoutBinding };

#ifdef RSMLIGHTING
    std::array<VkDescriptorSetLayoutBinding, 6> scenebindings = { uniformLayoutBinding, imageLayoutBinding, imageLayoutBinding , imageLayoutBinding , imageLayoutBinding, uniformLayoutBinding };
    bindingcount = 6;
#else

#ifdef SCREENSPACEAO
    std::array<VkDescriptorSetLayoutBinding, 6> scenebindings = { uniformLayoutBinding, imageLayoutBinding, imageLayoutBinding , imageLayoutBinding , imageLayoutBinding, imageLayoutBinding };
    bindingcount = 6;
#endif

#ifdef IBLLIGHTING
    std::array<VkDescriptorSetLayoutBinding, 5> scenebindings = { uniformLayoutBinding, imageLayoutBinding, imageLayoutBinding , imageLayoutBinding , imageLayoutBinding };
    bindingcount = 5;
#endif

#endif // RSMLIGHTING

	std::for_each(scenebindings.begin(), scenebindings.end(), [&binding](VkDescriptorSetLayoutBinding& imageLayoutBinding) {
        imageLayoutBinding.binding = binding++;
	});

    layoutInfo.bindingCount = bindingcount;
    layoutInfo.pBindings = scenebindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayoutS) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

}

void HelloVulkan::createDescriptorSet()
{
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayoutM;

    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSetM) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to allocate descriptor set!");
    }

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSetM;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = nullptr; // Optional
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    if(shadow != nullptr)
	{
		std::array<VkWriteDescriptorSet, 2> descriptorWrites = { descriptorWrite , descriptorWrite };

		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].pBufferInfo = &shadow->GetDescriptorBufferInfo();

		vkUpdateDescriptorSets(device, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}

    allocInfo.pSetLayouts = &descriptorSetLayoutS;
    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSetS) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to allocate descriptor set!");
    }

    bufferInfo.buffer = uniformBufferL;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UBOParams);

    descriptorWrite.dstSet = descriptorSetS;
    descriptorWrite.pBufferInfo = &bufferInfo;

	std::array<VkWriteDescriptorSet, 6> sceneDescriptorWrites = { descriptorWrite, descriptorWrite, descriptorWrite, descriptorWrite, descriptorWrite, descriptorWrite };

    VkDescriptorImageInfo imageInfo = {};
#ifdef RSMLIGHTING
    imageInfo = rsm->GetDepthDescriptorImageInfo();
#endif

#ifdef SHADOW
    imageInfo = shadow->GetDescriptorImageInfo();
#endif

	sceneDescriptorWrites[1].dstBinding = 1;
	sceneDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sceneDescriptorWrites[1].descriptorCount = 1;
	sceneDescriptorWrites[1].pBufferInfo = nullptr;
	sceneDescriptorWrites[1].pImageInfo = &imageInfo; // Optional
	sceneDescriptorWrites[1].pTexelBufferView = nullptr; // Optional

	sceneDescriptorWrites[2].dstBinding = 2;
	sceneDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sceneDescriptorWrites[2].descriptorCount = 1;
	sceneDescriptorWrites[2].pBufferInfo = nullptr;
	sceneDescriptorWrites[2].pImageInfo = &EmuMap.descriptor; // Optional
	sceneDescriptorWrites[2].pTexelBufferView = nullptr; // Optional

	sceneDescriptorWrites[3].dstBinding = 3;
	sceneDescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sceneDescriptorWrites[3].descriptorCount = 1;
	sceneDescriptorWrites[3].pBufferInfo = nullptr;
	sceneDescriptorWrites[3].pImageInfo = &EavgMap.descriptor; // Optional
	sceneDescriptorWrites[3].pTexelBufferView = nullptr; // Optional

    int sceneDescriptorsCount = 4;

#ifdef IBLLIGHTING

    sceneDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sceneDescriptorWrites[2].pBufferInfo = nullptr;
	sceneDescriptorWrites[2].dstBinding = 2;
	sceneDescriptorWrites[2].pImageInfo = &envLight.irradianceCube.descriptor; // Optional

	sceneDescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sceneDescriptorWrites[3].pBufferInfo = nullptr;
	sceneDescriptorWrites[3].dstBinding = 3;
	sceneDescriptorWrites[3].pImageInfo = &envLight.prefilteredMap.descriptor; // Optional

	sceneDescriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sceneDescriptorWrites[4].pBufferInfo = nullptr;
	sceneDescriptorWrites[4].dstBinding = 4;
	sceneDescriptorWrites[4].pImageInfo = &envLight.BRDFLutMap.descriptor; // Optional

    vkUpdateDescriptorSets(device, (uint32_t)sceneDescriptorWrites.size(), sceneDescriptorWrites.data(), 0, nullptr);
#endif

#ifdef RSMLIGHTING

    rsm->InitRandomBuffer();
	sceneDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sceneDescriptorWrites[2].pBufferInfo = nullptr;
	sceneDescriptorWrites[2].dstBinding = 2;
	sceneDescriptorWrites[2].pImageInfo = &rsm->GetPositionDescriptorImageInfo(); // Optional

	sceneDescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sceneDescriptorWrites[3].pBufferInfo = nullptr;
	sceneDescriptorWrites[3].dstBinding = 3;
	sceneDescriptorWrites[3].pImageInfo = &rsm->GetNormalDescriptorImageInfo(); // Optional

	sceneDescriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sceneDescriptorWrites[4].pBufferInfo = nullptr;
	sceneDescriptorWrites[4].dstBinding = 4;
	sceneDescriptorWrites[4].pImageInfo = &rsm->GetFluxDescriptorImageInfo(); // Optional

	sceneDescriptorWrites[5].dstBinding = 5;
	sceneDescriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	sceneDescriptorWrites[5].descriptorCount = 1;

    VkDescriptorBufferInfo info = rsm->GetBufferInfo();
	sceneDescriptorWrites[5].pBufferInfo = &info;
	sceneDescriptorWrites[5].pImageInfo = nullptr; // Optional
	sceneDescriptorWrites[5].pTexelBufferView = nullptr; // Optional

	vkUpdateDescriptorSets(device, (uint32_t)sceneDescriptorWrites.size(), sceneDescriptorWrites.data(), 0, nullptr);

#endif

#ifdef SCREENSPACEAO
	sceneDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sceneDescriptorWrites[2].pBufferInfo = nullptr;
	sceneDescriptorWrites[2].dstBinding = 2;
	sceneDescriptorWrites[2].pImageInfo = &ssao->GetSSAODescriptorImageInfo(); // Optional

    sceneDescriptorsCount = 3;
#endif

    vkUpdateDescriptorSets(device, sceneDescriptorsCount, sceneDescriptorWrites.data(), 0, nullptr);

#ifdef SCREENSPACEAO
	imageInfo = ssao->GetSSAODescriptorImageInfo();
#endif // SCREENSPACEAO

    debug.SetupDescriptSet(descriptorPool, imageInfo);
}
