#pragma once

#include <utility>
#include <vector>
#include <array>
#include <vulkan/vulkan.h>

// Vertexklasse für einen Knoten als Würfeldarstellung (3D-Darstellung)
struct CubeVertex {
	float position[3];
	float color[3];
	float normal[3];
	int id[3];

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
};
