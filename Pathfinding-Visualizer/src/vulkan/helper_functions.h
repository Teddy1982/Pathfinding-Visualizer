#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

class HelperFunctions {
public:
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice);
	VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool, VkDevice device);
	void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue graphicsQueue, VkDevice device, VkCommandPool commandPool);
	std::vector<char> readFile(const std::string & filename);
	VkShaderModule createShaderModule(VkDevice device, const std::string & filename);
};
