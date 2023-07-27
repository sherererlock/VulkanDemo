#pragma once

#include "glm/glm/glm.hpp"
#include <glm/glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan.h>

#include "tinygltf/tiny_gltf.h"
#include "Texture.h"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
	glm::vec3 color;
	glm::vec3 tangent;

	enum class VertexComponent { Position, Normal, UV, Color, Tangent, Joint0, Weight0 };

	static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }


    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, uv);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, color);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(Vertex, tangent);

        return attributeDescriptions;
    }

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(const std::vector<VertexComponent> components)
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
		attributeDescriptions.resize(components.size());

		for (int i = 0; i < components.size(); i++)
		{
			VertexComponent component = components[i];
			switch (component)
			{
			case Vertex::VertexComponent::Position:

				attributeDescriptions[i].binding = 0;
				attributeDescriptions[i].location = i;
				attributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[i].offset = offsetof(Vertex, pos);

				break;
			case Vertex::VertexComponent::Normal:
				attributeDescriptions[i].binding = 0;
				attributeDescriptions[i].location = i;
				attributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[i].offset = offsetof(Vertex, normal);
				break;
			case Vertex::VertexComponent::UV:
				attributeDescriptions[i].binding = 0;
				attributeDescriptions[i].location = i;
				attributeDescriptions[i].format = VK_FORMAT_R32G32_SFLOAT;
				attributeDescriptions[i].offset = offsetof(Vertex, uv);
				break;
			case Vertex::VertexComponent::Color:
				attributeDescriptions[i].binding = 0;
				attributeDescriptions[i].location = i;
				attributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[i].offset = offsetof(Vertex, color);
				break;
			case Vertex::VertexComponent::Tangent:
				attributeDescriptions[i].binding = 0;
				attributeDescriptions[i].location = i;
				attributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[i].offset = offsetof(Vertex, tangent);
				break;
			case Vertex::VertexComponent::Joint0:
				break;
			case Vertex::VertexComponent::Weight0:
				break;
			default:
				break;
			}

		}

		return attributeDescriptions;
	}
};

// A primitive contains the data for a single draw call
struct Primitive {
	uint32_t firstIndex;
	uint32_t indexCount;
	int32_t materialIndex;
};

struct Mesh {
	std::vector<Primitive> primitives;
};

	// A node represents an object in the glTF scene graph
struct Node {
	Node* parent;
	std::vector<Node*> children;
	Mesh mesh;
	glm::mat4 matrix;
	~Node() {
		for (auto& child : children) {
			delete child;
		}
	}
};


// A glTF material stores information in e.g. the texture that is attached to it and colors
struct Material {
	struct MaterialData
	{
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		glm::vec3 emissiveFactor = glm::vec3(1.0f);
	} Data;

	uint32_t baseColorTextureIndex;
	uint32_t normalTextureIndex;
	uint32_t roughnessTextureIndex;
	uint32_t emissiveTextureIndex;

	VkDescriptorSet descriptorSet;
	VkBuffer materialBuffer;
	VkDeviceMemory materialBufferMemory;

	void UpdateMaterialBuffer(VkDevice device)
	{
		void* data;
		vkMapMemory(device, materialBufferMemory, 0, sizeof(MaterialData), 0, &data);
		memcpy(data, &Data, sizeof(MaterialData));
		vkUnmapMemory(device, materialBufferMemory);
	}

	void Cleanup(VkDevice device)
	{
		vkDestroyBuffer(device, materialBuffer, nullptr);
		vkFreeMemory(device, materialBufferMemory, nullptr);
	}
};

// Contains the texture for a single glTF image
// Images may be reused by texture objects and are as such separated
struct Image {
public:
	Texture2D texture;
	// We also store (and create) a descriptor set that's used to access this texture from the fragment shader
	
};

// A glTF texture stores a reference to the image and a sampler
// In this sample, we are only interested in the image
struct Texture1 {
	int32_t imageIndex;
};

class gltfModel
{
public:
	struct {
		VkBuffer buffer;
		VkDeviceMemory memory;
	} vertices;

	// Single index buffer for all primitives
	struct {
		int count;
		VkBuffer buffer;
		VkDeviceMemory memory;
	} indices;

	/*
		Model data
	*/
	std::vector<Image> images;
	std::vector<Texture1> textures;
	std::vector<Material> materials;
	std::vector<Node*> nodes;

	VkDevice logicalDevice;

	mutable glm::mat4 translation;
	mutable int cascadedIndex = 0; 
	void Cleanup();

	void loadImages(tinygltf::Model& input);

	void loadImages(std::string path, tinygltf::Model& input);

	// for what?
	void loadTextures(tinygltf::Model& input)
	{
		textures.resize(input.textures.size());
		for (size_t i = 0; i < input.textures.size(); i++) {
			textures[i].imageIndex = input.textures[i].source;
		}
	}

	void loadMaterials(tinygltf::Model& input);

	void loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);

	// Draw a single node including child nodes (if present)
	void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Node* node, uint32_t flag, uint32_t offset) const;

	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t flag, uint32_t offset) const;

	void drawWithOffset(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t flag, uint32_t offset) const;

};

