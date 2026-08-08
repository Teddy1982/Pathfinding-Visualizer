#include "quad_vertex.h"

VkVertexInputBindingDescription QuadVertex::getBindingDescription() {
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(QuadVertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> QuadVertex::getAttributeDescriptions() {
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

	// Position Attribute
	attributeDescriptions[0].binding = 0; // Binding index
	attributeDescriptions[0].location = 0; // Matches shader location
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(QuadVertex, position); // Offset

	// ID Attribute
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SINT; // int
	attributeDescriptions[1].offset = offsetof(QuadVertex, id); // Offset of id

	return attributeDescriptions;
}