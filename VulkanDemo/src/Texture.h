#pragma once
#include <vulkan/vulkan.h>

class HelloVulkan;

struct Texture
{
public:
	VkDevice			  device;
	VkImage               image;
	VkImageLayout         imageLayout;
	VkDeviceMemory        deviceMemory;
	VkImageView           view;
	uint32_t              width, height;
	uint32_t              mipLevels;
	uint32_t              layerCount;
	VkDescriptorImageInfo descriptor;
	VkSampler             sampler;

	void      updateDescriptor();
	void      destroy();
};

class Texture2D : public Texture
{
public:
	void fromBuffer(
		HelloVulkan*	   helloVulkan,
		void *             buffer,
		VkDeviceSize       bufferSize,
		VkFormat           format,
		uint32_t           texWidth,
		uint32_t           texHeight,
		VkFilter           filter          = VK_FILTER_LINEAR,
		VkImageUsageFlags  imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout      imageLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
};