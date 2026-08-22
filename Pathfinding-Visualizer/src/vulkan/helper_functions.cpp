#include "helper_functions.h"
#include <stdexcept>
#include <fstream>

// Hilfsfunktion zum Finden eines passenden Speichertyp für einen Buffer
uint32_t HelperFunctions::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) {
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	// Iterieren über die Speichertypen um den richtigen zu finden
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i; 
		}
	}

	// wenn kein passender Speichertyp gefunden wurde, wirf einen Fehler
	throw std::runtime_error("Failed to find suitable memory type!");
}

// Erstellt einen temporären Command Buffer und beginnt dessen
// einmalige Aufzeichnung.
VkCommandBuffer HelperFunctions::beginSingleTimeCommands(VkCommandPool commandPool, VkDevice device)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffer!");
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin command buffer!");
	}

	return commandBuffer;
}

// Beendet und übermittelt einen temporären Command Buffer,
// wartet auf dessen Ausführung und gibt ihn anschließend frei.
void HelperFunctions::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue graphicsQueue, VkDevice device, VkCommandPool commandPool)
{
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record command buffer!");
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit command buffer!");
	}

	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

// Hilfsfunktion zum Lesen einer Datei in einen Vectortyp aus Bytes
std::vector<char> HelperFunctions::readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open shader file: " + filename);
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

// Funktion zum Erstellen eines Shader Modules
VkShaderModule HelperFunctions::createShaderModule(VkDevice device, const std::string& filename) {
	// Einlesen einer Shaderdatei in einen Vector aus Bytes
	std::vector<char> code = readFile(filename);

	// Erstellen des Shadermodules
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module!");
	}

	return shaderModule;
}
