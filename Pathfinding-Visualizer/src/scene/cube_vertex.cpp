// cpp.Datei enthält Bindungs- und Attributdateien der Vertexklasse für einen Würfel für die Vulkan-Grafik-Pipeline

#include "cube_vertex.h"

VkVertexInputBindingDescription CubeVertex::getBindingDescription() {
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(CubeVertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 4> CubeVertex::getAttributeDescriptions() {
	std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

	// Position Attribute
	attributeDescriptions[0].binding = 0; // Binding index
	attributeDescriptions[0].location = 0; // Matches shader location
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(CubeVertex, position); // Offset

	// Coord Attribute
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (float)
	attributeDescriptions[1].offset = offsetof(CubeVertex, color);	// Offset of color in Vertex struct

	// Normal Attribute
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (float)
	attributeDescriptions[2].offset = offsetof(CubeVertex, normal); // Offset of normal in Vertex struct

	// ID Attribute
	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SINT; // int
	attributeDescriptions[3].offset = offsetof(CubeVertex, id); // Offset of id

	return attributeDescriptions;
}