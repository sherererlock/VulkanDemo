```mermaid
classDiagram
class vertex{
	glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 color;
}
class vertices{
    VkBuffer buffer;
    VkDeviceMemory memory;
}
class indices{
    int count;
    VkBuffer buffer;
    VkDeviceMemory memory;
} ;
class Primitive {
    uint32_t firstIndex;
    uint32_t indexCount;
    int32_t materialIndex;
};

class Mesh {
	std::vector<Primitive> primitives;
};

class Node {
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

class Model{
	
}
```

