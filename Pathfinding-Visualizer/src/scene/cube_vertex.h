#pragma once

#include <utility>
#include <vector>
#include <array>
#include <vulkan/vulkan.h>

// Vertex Data
struct CubeVertex {
	float position[3];	// x, y, z
	float color[3];		// u, v, w - texture coords
	float normal[3];	// nx, ny, nz
	int id[3];		// id (x, y, z)

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
};
