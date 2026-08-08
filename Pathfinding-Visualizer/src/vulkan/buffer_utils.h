#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "renderData.h"
#include "helper_functions.h"

struct AppState;

class BufferUtils {
public:

	BufferUtils(RenderData& rData, AppState& state);

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void createVertexBuffer(VkBuffer& vBuffer, VkBuffer& iBuffer, VkDeviceMemory& vMemory, VkDeviceMemory& iMemory, void* vertexData, VkDeviceSize vertexSize, void* indexData, VkDeviceSize indexSize, uint32_t numIndices);
	void createUniformBuffer();
	void createLeftStorageBuffer();
	void createRightStorageBuffer();
	void createConnectionBuffers();
	
private:
	HelperFunctions helperFunctions;
	RenderData& renderData;
	AppState& appState;
};

