#include "connection_vertex.h"

VkVertexInputBindingDescription ConnectionVertex::getBindingDescription() {
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(ConnectionVertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

VkVertexInputAttributeDescription ConnectionVertex::getAttributeDescription() {
	static VkVertexInputAttributeDescription attributeDescription;

	// Position Attribute
	attributeDescription.binding = 0; // Binding index
	attributeDescription.location = 0; // Matches shader location
	attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescription.offset = offsetof(ConnectionVertex, position);

	return attributeDescription;
}