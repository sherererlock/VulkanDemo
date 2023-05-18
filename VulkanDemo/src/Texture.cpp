#include "Texture.h"
#include "HelloVulkan.h"

void Texture2D::fromBuffer(HelloVulkan*	helloVulkan,void* buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth, uint32_t texHeight, VkFilter filter, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout)
{
	VkDevice device = helloVulkan->GetDevice();

	this->device = device;
	width = texWidth;
	height = texHeight;
	mipLevels = 1;

	// Create a host-visible staging buffer that contains the raw image data
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	helloVulkan->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(device, stagingMemory, 0, bufferSize, 0, &data);
    memcpy(data, buffer, bufferSize);
    vkUnmapMemory(device, stagingMemory);

	helloVulkan->createImage(texWidth, texHeight, format, VK_IMAGE_TILING_OPTIMAL, imageUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, deviceMemory, mipLevels, VK_SAMPLE_COUNT_1_BIT);

    helloVulkan->transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL , mipLevels);

	helloVulkan->copyBufferToImage(stagingBuffer, image, texWidth,texHeight);

    helloVulkan->transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL , mipLevels);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

	helloVulkan->createTextureSampler(sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, mipLevels);

	view = helloVulkan->createImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

	updateDescriptor();
}

void Texture::updateDescriptor()
{
	descriptor.sampler = sampler;
	descriptor.imageView = view;
	descriptor.imageLayout = imageLayout;
}

void Texture::destroy()
{
	vkDestroyImageView(device, view, nullptr);
	vkDestroyImage(device, image, nullptr);
	if (sampler)
	{
		vkDestroySampler(device, sampler, nullptr);
	}
	vkFreeMemory(device, deviceMemory, nullptr);
}
