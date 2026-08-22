// cpp.Datei enthält Bindungs- und Attributdateien der Vertexklasse für die Aussenkanten eines Quadrats für die Vulkan-Grafik-Pipeline

#include "quad_edge_vertex.h"

VkVertexInputBindingDescription QuadEdgeVertex::getBindingDescription() {
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(QuadEdgeVertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> QuadEdgeVertex::getAttributeDescriptions() {
	static std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;

	// Position Attribute
	attributeDescriptions[0].binding = 0; // Binding index
	attributeDescriptions[0].location = 0; // Matches shader location
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(QuadEdgeVertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SINT; // int
	attributeDescriptions[1].offset = offsetof(QuadEdgeVertex, id); // Offset of id

	return attributeDescriptions;
}
