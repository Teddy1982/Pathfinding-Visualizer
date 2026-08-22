#include "buffer_utils.h"
#include "vulkan_context.h"
#include "uniform_buffer_object.h"

#include "../scene/cube_state.h"
#include "../scene/geometry_builder.h"
#include "../application/app_state.h"

#include <vulkan/vulkan.h>
#include <stdexcept>


BufferUtils::BufferUtils(RenderData& rData,AppState& state) : renderData(rData), appState(state) {}

void BufferUtils::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	// Schritt 1: Erstellung des Vulkan Buffer
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(renderData.vkInst.device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	// Schritt 2: Speicherallokierung für den Buffer
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(renderData.vkInst.device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = helperFunctions.findMemoryType(memRequirements.memoryTypeBits, properties, renderData.vkInst.physicalDevice);

	if (vkAllocateMemory(renderData.vkInst.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate buffer memory!");
	}

	// Schritt 3: Bindung des Buffers an den allokierten Speicher
	if (vkBindBufferMemory(renderData.vkInst.device, buffer, bufferMemory, 0) != VK_SUCCESS) {
		throw std::runtime_error("Failed to bind buffer memory!");
	}
}

void BufferUtils::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = helperFunctions.beginSingleTimeCommands(renderData.commandBuffers.commandPool, renderData.vkInst.device);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0; // Start am Anfang des Source Buffers
	copyRegion.dstOffset = 0; // Start am Anfang des Destination Buffers
	copyRegion.size = size;

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	helperFunctions.endSingleTimeCommands(commandBuffer, renderData.vkInst.graphicsQueue, renderData.vkInst.device, renderData.commandBuffers.commandPool);
}

void BufferUtils::createVertexBuffer(VkBuffer& vBuffer, VkBuffer& iBuffer, VkDeviceMemory& vMemory, VkDeviceMemory& iMemory, void* vertexData, VkDeviceSize vertexSize, void* indexData, VkDeviceSize indexSize, uint32_t numIndices) {
	if (vBuffer != VK_NULL_HANDLE || iBuffer != VK_NULL_HANDLE) return;

	// 1. Erstellung eines Staging Buffers
	VkBuffer stagingVertexBuffer, stagingIndexBuffer;
	VkDeviceMemory stagingVertexMemory, stagingIndexMemory;

	// Allokieren und Kopieren von Vertex Buffer Daten
	createBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingVertexBuffer, stagingVertexMemory);
	createBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingIndexBuffer, stagingIndexMemory);

	// 2. Kopieren der Vertex Daten zum Staging Buffer
	void* data;
	vkMapMemory(renderData.vkInst.device, stagingVertexMemory, 0, vertexSize, 0, &data);
	memcpy(data, vertexData, static_cast<size_t>(vertexSize));
	vkUnmapMemory(renderData.vkInst.device, stagingVertexMemory);

	// 3. Kopieren der Index Daten zum Staging Buffer
	vkMapMemory(renderData.vkInst.device, stagingIndexMemory, 0, indexSize, 0, &data);
	memcpy(data, indexData, static_cast<size_t>(indexSize));
	vkUnmapMemory(renderData.vkInst.device, stagingIndexMemory);

	// 4. Erstellung des GPU Buffers
	createBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vBuffer, vMemory);
	createBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, iBuffer, iMemory);

	// 5. Kopieren vom Staging zu GPU Buffers
	copyBuffer(stagingVertexBuffer, vBuffer, vertexSize);
	copyBuffer(stagingIndexBuffer, iBuffer, indexSize);

	// 6. Staging Buffers aufräumen
	vkDestroyBuffer(renderData.vkInst.device, stagingVertexBuffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, stagingVertexMemory, nullptr);

	vkDestroyBuffer(renderData.vkInst.device, stagingIndexBuffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, stagingIndexMemory, nullptr);
}

// Erstellung des Uniform Buffers
void BufferUtils::createUniformBuffer() {
	if (renderData.uniformBuffer.buffer != VK_NULL_HANDLE) return;

	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, renderData.uniformBuffer.buffer, renderData.uniformBuffer.memory);
}

// Erstellung des Storage Buffers für die linke Algorithmusansicht
void BufferUtils::createLeftStorageBuffer() {
	if (renderData.storageBuffer.buffer != VK_NULL_HANDLE) return;

	VkDeviceSize bufferSize = sizeof(CubeState) * appState.cubeColorsLeft.size();

	createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, renderData.storageBuffer.buffer, renderData.storageBuffer.memory);
}

// Erstellung des Storage Buffers für die rechte Algorithmusansicht
void BufferUtils::createRightStorageBuffer() {
	if (renderData.storageBuffer.bufferRight != VK_NULL_HANDLE) return;

	VkDeviceSize bufferSize = sizeof(CubeState) * appState.cubeColorsRight.size();

	createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, renderData.storageBuffer.bufferRight, renderData.storageBuffer.memoryRight);
}

void BufferUtils::createConnectionBuffers() {

	// Erstellen von Buffer für die Pfad-Verbindungslinien für die linke und rechte Algorithmusansicht
	createBuffer(
		sizeof(ConnectionVertex) * 8 * appState.xSize * appState.ySize * appState.zSize - 1,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		renderData.vertexBuffer.connectionVBuffer,
		renderData.vertexBuffer.connectionVMemory
	);

	createBuffer(
		sizeof(uint32_t) * 36 * appState.xSize * appState.ySize * appState.zSize - 1,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		renderData.vertexBuffer.connectionIBuffer,
		renderData.vertexBuffer.connectionIMemory
	);

	createBuffer(
		sizeof(ConnectionVertex) * 8 * appState.xSize * appState.ySize * appState.zSize - 1,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		renderData.vertexBuffer.connectionVBufferRight,
		renderData.vertexBuffer.connectionVMemoryRight
	);

	createBuffer(
		sizeof(uint32_t) * 36 * appState.xSize * appState.ySize * appState.zSize - 1,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		renderData.vertexBuffer.connectionIBufferRight,
		renderData.vertexBuffer.connectionIMemoryRight
	);
}
