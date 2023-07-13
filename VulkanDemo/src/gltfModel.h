#pragma once

#include "tinygltf/tiny_gltf.h"
#include "glm/glm/glm.hpp"
#include <glm/glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan.h>

#include "Texture.h"

struct Vertex1 {
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
        bindingDescription.stride = sizeof(Vertex1);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }


    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex1, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex1, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex1, uv);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex1, color);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(Vertex1, tangent);

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
			case Vertex1::VertexComponent::Position:

				attributeDescriptions[i].binding = 0;
				attributeDescriptions[i].location = i;
				attributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[i].offset = offsetof(Vertex1, pos);

				break;
			case Vertex1::VertexComponent::Normal:
				attributeDescriptions[i].binding = 0;
				attributeDescriptions[i].location = i;
				attributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[i].offset = offsetof(Vertex1, normal);
				break;
			case Vertex1::VertexComponent::UV:
				attributeDescriptions[i].binding = 0;
				attributeDescriptions[i].location = i;
				attributeDescriptions[i].format = VK_FORMAT_R32G32_SFLOAT;
				attributeDescriptions[i].offset = offsetof(Vertex1, uv);
				break;
			case Vertex1::VertexComponent::Color:
				attributeDescriptions[i].binding = 0;
				attributeDescriptions[i].location = i;
				attributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[i].offset = offsetof(Vertex1, color);
				break;
			case Vertex1::VertexComponent::Tangent:
				attributeDescriptions[i].binding = 0;
				attributeDescriptions[i].location = i;
				attributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[i].offset = offsetof(Vertex1, tangent);
				break;
			case Vertex1::VertexComponent::Joint0:
				break;
			case Vertex1::VertexComponent::Weight0:
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
	float islight = 0.0;
};

struct Mesh {
	std::vector<Primitive> primitives;
};

struct Node;

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

	// for what?
	void loadTextures(tinygltf::Model& input)
	{
		textures.resize(input.textures.size());
		for (size_t i = 0; i < input.textures.size(); i++) {
			textures[i].imageIndex = input.textures[i].source;
		}
	}

	void loadMaterials(tinygltf::Model& input)
	{
		materials.resize(input.materials.size());
		for (size_t i = 0; i < input.materials.size(); i++) {
			// We only read the most basic properties required for our sample
			tinygltf::Material glTFMaterial = input.materials[i];
			// Get the base color factor
			if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
				materials[i].Data.baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
			}
			// Get base color texture index
			if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) {
				materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
			}

			if (glTFMaterial.emissiveFactor.size() == 3)
			{
				materials[i].Data.emissiveFactor = glm::make_vec3(glTFMaterial.emissiveFactor.data());
			}

			materials[i].normalTextureIndex = glTFMaterial.normalTexture.index;
			materials[i].roughnessTextureIndex = glTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
			materials[i].emissiveTextureIndex = glTFMaterial.emissiveTexture.index;
		}
	}

	void loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex1>& vertexBuffer);

	// Draw a single node including child nodes (if present)
	void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Node* node, uint32_t flag) const;

	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t flag) const;

	void drawWithOffset(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t flag) const;

};

