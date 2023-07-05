#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm/glm.hpp>
#include <vulkan/vulkan.h>

class TextureCubeMap;
class HelloVulkan;

class PreProcess
{

public:
	static void generateIrradianceCube(HelloVulkan* vulkan, const TextureCubeMap& cubeMap, TextureCubeMap& irradianceMap);
};

