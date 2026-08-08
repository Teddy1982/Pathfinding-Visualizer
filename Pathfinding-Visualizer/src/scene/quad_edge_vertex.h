#pragma once

#include <utility>
//#include <vector>
#include <array>
#include <vulkan/vulkan.h>


struct QuadEdgeVertex {
	float position[3];	// x, y, z
	int id[3];

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};
