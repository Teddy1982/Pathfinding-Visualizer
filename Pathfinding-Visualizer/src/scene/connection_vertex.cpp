// cpp.Datei enthält Bindungs- und Attributdateien der Vertexklasse für Pfadverbindungslinien für die Vulkan-Grafik-Pipeline

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
	attributeDescription.binding = 0;
	attributeDescription.location = 0;
	attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescription.offset = offsetof(ConnectionVertex, position);

	return attributeDescription;
}