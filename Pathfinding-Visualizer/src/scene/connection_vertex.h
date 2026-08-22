#pragma once

#include <utility>
#include <vector>
#include <array>
#include <vulkan/vulkan.h>

//Vertexklasse für Pfad-Verbindungslinien 

struct ConnectionVertex {
	float position[3];	// x, y, z

	static VkVertexInputBindingDescription getBindingDescription();
	static VkVertexInputAttributeDescription getAttributeDescription();
};

