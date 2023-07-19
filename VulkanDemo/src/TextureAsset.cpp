
#include "Texture.h"
#include "HelloVulkan.h"

	/**
	* Load a 2D texture including all mip levels
	*
	* @param filename File to load (supports .ktx)
	* @param format Vulkan format of the image data stored in the file
	* @param device Vulkan device to create the texture on
	* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	* @param (Optional) forceLinear Force linear tiling (not advised, defaults to false)
	*
	*/
ktxResult Texture::loadKTXFile(std::string filename, ktxTexture **target)
{
	ktxResult result = KTX_SUCCESS;
	result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, target);				
	return result;
}

void Texture2D::loadFromFile(HelloVulkan* helloVulkan,std::string filename, VkFormat format, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout, bool forceLinear)
{
	device = helloVulkan->GetDevice();

	ktxTexture* ktxTexture;
	ktxResult result = loadKTXFile(filename, &ktxTexture);
	assert(result == KTX_SUCCESS);

	width = ktxTexture->baseWidth;
	height = ktxTexture->baseHeight;
	mipLevels = ktxTexture->numLevels;

	ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
	ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

	// Create a host-visible staging buffer that contains the raw image data
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	helloVulkan->createBuffer(ktxTextureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(device, stagingMemory, 0, ktxTextureSize, 0, &data);
    memcpy(data, ktxTexture, ktxTextureSize);
    vkUnmapMemory(device, stagingMemory);

	helloVulkan->createImage(width, height, format, VK_IMAGE_TILING_OPTIMAL, imageUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, deviceMemory, mipLevels, VK_SAMPLE_COUNT_1_BIT);

    helloVulkan->transitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL , mipLevels);

	helloVulkan->copyBufferToImage(stagingBuffer, image, width, height);

    helloVulkan->transitionImageLayout(image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL , mipLevels);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

	ktxTexture_Destroy(ktxTexture);

	helloVulkan->createTextureSampler(sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, mipLevels);

	helloVulkan->createImageView(view, image, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

	this->imageLayout = imageLayout;

	// Update descriptor image info member that can be used for setting up descriptor sets
	updateDescriptor();
}

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

    helloVulkan->transitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL , mipLevels);

	helloVulkan->copyBufferToImage(stagingBuffer, image, texWidth,texHeight);

    helloVulkan->transitionImageLayout(image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL , mipLevels);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

	helloVulkan->createTextureSampler(sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, mipLevels);

	helloVulkan->createImageView(view, image, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

	this->imageLayout = imageLayout;

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

void TextureCubeMap::loadFromFile(HelloVulkan*	helloVulkan, std::string filename, VkFormat format, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout)
{
	ktxTexture* ktxTexture;
	ktxResult result = loadKTXFile(filename, &ktxTexture);
	assert(result == KTX_SUCCESS);

	this->device = helloVulkan->GetDevice();
	width = ktxTexture->baseWidth;
	height = ktxTexture->baseHeight;
	mipLevels = ktxTexture->numLevels;

	ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
	ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

	// Create a host-visible staging buffer that contains the raw image data
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	helloVulkan->createBuffer(ktxTextureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);
	// Copy texture data into staging buffer
	void *data;
	vkMapMemory(device, stagingMemory, 0, ktxTextureSize, 0, &data);
	memcpy(data, ktxTextureData, ktxTextureSize);
	vkUnmapMemory(device, stagingMemory);

	// Setup buffer copy regions for each face including all of its mip levels
	std::vector<VkBufferImageCopy> bufferCopyRegions;

	for (uint32_t face = 0; face < 6; face++)
	{
		for (uint32_t level = 0; level < mipLevels; level++)
		{
			ktx_size_t offset;
			KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset);
			assert(result == KTX_SUCCESS);

			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = level;
			bufferCopyRegion.imageSubresource.baseArrayLayer = face;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> level;
			bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> level;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;

			bufferCopyRegions.push_back(bufferCopyRegion);
		}
	}

	helloVulkan->createImage(width, height, format, VK_IMAGE_TILING_OPTIMAL, imageUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, deviceMemory, mipLevels, VK_SAMPLE_COUNT_1_BIT, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

	// Image barrier for optimal image (target)
	// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = mipLevels;
	subresourceRange.layerCount = 6;

	VkCommandBuffer commandBuffer = helloVulkan->beginSingleTimeCommands();

	helloVulkan->transitionImageLayout(commandBuffer,
		image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange);

	vkCmdCopyBufferToImage(
		commandBuffer,
		stagingBuffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		static_cast<uint32_t>(bufferCopyRegions.size()),
		bufferCopyRegions.data());

	helloVulkan->transitionImageLayout(commandBuffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		imageLayout,
		subresourceRange);

	helloVulkan->endSingleTimeCommands(commandBuffer);

	// Change texture image layout to shader read after all faces have been copied
	this->imageLayout = imageLayout;

	// Create sampler
	helloVulkan->createTextureSampler(sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, mipLevels, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	// Create image view
	helloVulkan->createImageView(view, image, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, VK_IMAGE_VIEW_TYPE_CUBE, 6);

	// Clean up staging resources
	ktxTexture_Destroy(ktxTexture);
	vkFreeMemory(device, stagingMemory, nullptr);
	vkDestroyBuffer(device, stagingBuffer, nullptr);

	// Update descriptor image info member that can be used for setting up descriptor sets
	updateDescriptor();
}