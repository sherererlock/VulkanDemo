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
	glm::vec4 baseColorFactor = glm::vec4(1.0f);
	uint32_t baseColorTextureIndex;
};

// Contains the texture for a single glTF image
// Images may be reused by texture objects and are as such separated
struct Image {
public:
	Texture2D texture;
	// We also store (and create) a descriptor set that's used to access this texture from the fragment shader
	VkDescriptorSet descriptorSet;
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
	VkQueue copyQueue;

	~gltfModel();

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
				materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
			}
			// Get base color texture index
			if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) {
				materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
			}
		}
	}

	void loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex1>& vertexBuffer);
};

