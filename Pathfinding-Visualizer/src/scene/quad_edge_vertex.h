#pragma once

#include <utility>
#include <array>
#include <vulkan/vulkan.h>

// Vertexklasse für Aussenkanten eines Quadrats im 2D-Grid
struct QuadEdgeVertex {
	float position[3];	// x, y, z
	int id[3];

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};
