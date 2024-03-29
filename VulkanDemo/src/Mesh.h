#pragma once
#include<vector>
#include <glm/glm/glm.hpp>
#include "gltfModel.h"

class CubeMesh
{
public:
	const std::vector<glm::vec3> positions = {
		// Front face
		{- 1.0, -1.0, 1.0,},
		{1.0, -1.0, 1.0,  },
		{1.0, 1.0, 1.0,	  },
		{-1.0, 1.0, 1.0,  },
							   
		// Back face	   
		{-1.0, -1.0, -1.0,},
		{-1.0, 1.0, -1.0, },
		{1.0, 1.0, -1.0,  },
		{1.0, -1.0, -1.0, },
							   
		// Top face	  	   
		{-1.0, 1.0, -1.0, },
		{-1.0, 1.0, 1.0,  },
		{1.0, 1.0, 1.0,	  },
		{1.0, 1.0, -1.0,  },
							   
		// Bottom face	   
		{-1.0, -1.0, -1.0,},
		{1.0, -1.0, -1.0, },
		{1.0, -1.0, 1.0,  },
		{-1.0, -1.0, 1.0, },
							   
		// Right face	   
		{1.0, -1.0, -1.0, },
		{1.0, 1.0, -1.0,  },
		{1.0, 1.0, 1.0,	  },
		{1.0, -1.0, 1.0,  },
							   
		// Left face	   
		{-1.0, -1.0, -1.0,},
		{-1.0, -1.0, 1.0, },
		{-1.0, 1.0, 1.0,  },
		{-1.0, 1.0, -1.0, },
	};

	const std::vector<unsigned int> indices = {
		0, 1, 2, 0, 2, 3,    // front
		4, 5, 6, 4, 6, 7,    // back
		8, 9, 10, 8, 10, 11,   // top
		12, 13, 14, 12, 14, 15,   // bottom
		16, 17, 18, 16, 18, 19,   // right
		20, 21, 22, 20, 22, 23,   // left
	};

	std::vector<Vertex> vertex;

public:
	CubeMesh()
	{
		for (int i = 0; i < positions.size(); i++)
		{
			vertex.push_back({positions[i], glm::vec3(1.0f, 0.0f, 0.0f)});
		}
	}
};