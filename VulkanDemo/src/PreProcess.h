#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <string>
#include <functional>

class TextureCubeMap;
class HelloVulkan;

class PreProcess
{
private:
	static void generateMap(HelloVulkan* vulkan, const TextureCubeMap& envCubeMap, TextureCubeMap& cubemap, VkFormat format, uint32_t dim, std::string vertexShaderPath, std::string fragmentShaderPath, uint32_t size, std::function<void*(float, glm::mat4)> getPushConsts);
public:
	static void generateIrradianceCube(HelloVulkan* vulkan, const TextureCubeMap& cubeMap, TextureCubeMap& irradianceMap);
	static void prefilterEnvMap(HelloVulkan* vulkan, const TextureCubeMap& cubeMap, TextureCubeMap& prifilterMap);

};

