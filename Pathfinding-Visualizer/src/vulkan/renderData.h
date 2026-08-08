#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>


// Vulkan Instance & Device
struct VulkanInstance {
	VkInstance			instance = VK_NULL_HANDLE;
	VkPhysicalDevice	physicalDevice = VK_NULL_HANDLE;
	VkDevice			device = VK_NULL_HANDLE;
	VkQueue				graphicsQueue = VK_NULL_HANDLE;
	VkQueue				presentQueue = VK_NULL_HANDLE;
	VkSurfaceKHR		surface = VK_NULL_HANDLE;
};

// Swapchain
struct VulkanSwapchain {
	VkSwapchainKHR				swapchain = VK_NULL_HANDLE;
	VkFormat					imageFormat;
	VkExtent2D					extent;
	std::vector<VkImage>		images;
	std::vector<VkImageView>	imageViews;
};

// Render Pass
struct VulkanRenderPass {
	VkRenderPass	renderPass = VK_NULL_HANDLE;
};

// Descriptor Sets (for Uniform Buffers)
struct VulkanDescriptors {
	VkDescriptorSetLayout	layout = VK_NULL_HANDLE;
	VkDescriptorPool		pool = VK_NULL_HANDLE;
	VkDescriptorSet			descriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet			descriptorSetRight = VK_NULL_HANDLE;
};

// Graphics Pipeline
struct VulkanPipeline {
	VkPipelineLayout	layout = VK_NULL_HANDLE;
	VkPipeline			cubePipeline = VK_NULL_HANDLE;
	VkPipeline			cubeEdgePipeline = VK_NULL_HANDLE;
	VkPipeline			connectionPipeline = VK_NULL_HANDLE;
	VkPipeline			quadPipeline = VK_NULL_HANDLE;
	VkPipeline			quadEdgePipeline = VK_NULL_HANDLE;
//	VkPipeline			connection2DPipeline = VK_NULL_HANDLE;
	VkPipeline			skyboxPipeline = VK_NULL_HANDLE;
	VkViewport			viewport;
	VkRect2D			scissor;
};

// Framebuffers
struct VulkanFramebuffers {
	std::vector<VkFramebuffer> framebuffers;
	VkImage depthImage;
	VkImageView depthImageView;
	VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
};

// Vertex Buffer
struct VulkanVertexBuffer {
	VkBuffer		cubeVBuffer = VK_NULL_HANDLE;
	VkBuffer		cubeEdgeVBuffer = VK_NULL_HANDLE;
	VkBuffer		cubeIBuffer = VK_NULL_HANDLE;
	VkBuffer		cubeEdgeIBuffer = VK_NULL_HANDLE;
	VkBuffer		connectionVBuffer = VK_NULL_HANDLE;
	VkBuffer		connectionIBuffer = VK_NULL_HANDLE;
	VkBuffer		skyboxVBuffer = VK_NULL_HANDLE;
	VkBuffer		skyboxIBuffer = VK_NULL_HANDLE;

	VkBuffer		quadVBuffer = VK_NULL_HANDLE;
	VkBuffer		quadEdgeVBuffer = VK_NULL_HANDLE;
	VkBuffer		quadIBuffer = VK_NULL_HANDLE;
	VkBuffer		quadEdgeIBuffer = VK_NULL_HANDLE;

	VkBuffer		connectionVBufferRight = VK_NULL_HANDLE;
	VkBuffer		connectionIBufferRight = VK_NULL_HANDLE;
	VkDeviceMemory	connectionIMemoryRight = VK_NULL_HANDLE;
	VkDeviceMemory	connectionVMemoryRight = VK_NULL_HANDLE;

	VkDeviceMemory	cubeVMemory = VK_NULL_HANDLE;
	VkDeviceMemory	cubeIMemory = VK_NULL_HANDLE;
	VkDeviceMemory	cubeEdgeVMemory = VK_NULL_HANDLE;
	VkDeviceMemory	cubeEdgeIMemory = VK_NULL_HANDLE;

	VkDeviceMemory	quadVMemory = VK_NULL_HANDLE;
	VkDeviceMemory	quadIMemory = VK_NULL_HANDLE;
	VkDeviceMemory	quadEdgeVMemory = VK_NULL_HANDLE;
	VkDeviceMemory	quadEdgeIMemory = VK_NULL_HANDLE;

	VkDeviceMemory	connectionIMemory = VK_NULL_HANDLE;
	VkDeviceMemory	connectionVMemory = VK_NULL_HANDLE;
	VkDeviceMemory  skyboxIMemory = VK_NULL_HANDLE;
	VkDeviceMemory  skyboxVMemory = VK_NULL_HANDLE;
	
	uint32_t cubeNumIndices;
	uint32_t cubeVertexSize;
	uint32_t cubeEdgeNumIndices;
	uint32_t cubeEdgeVertexSize;
	
	uint32_t quadNumIndices;
	uint32_t quadVertexSize;
	uint32_t quadEdgeNumIndices;
	uint32_t quadEdgeVertexSize;
	
	uint32_t connectionNumIndices;
	uint32_t connectionNumIndicesRight;
	uint32_t connectionVertexSize;
};

// Uniform Buffer
struct VulkanUniformBuffer {
	VkBuffer		buffer = VK_NULL_HANDLE;
	VkDeviceMemory	memory = VK_NULL_HANDLE;
};

struct VulkanStorageBuffer {
	VkBuffer		buffer = VK_NULL_HANDLE;
	VkBuffer		bufferRight = VK_NULL_HANDLE;
	VkDeviceMemory	memory = VK_NULL_HANDLE;
	VkDeviceMemory	memoryRight = VK_NULL_HANDLE;
};

// Command Buffers
struct VulkanCommandBuffers {
	VkCommandPool commandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> commandBuffers;
};

struct VulkanImages {
	VkImageView skyboxImageView;
	VkSampler skyboxSampler;
	VkImage skyboxImage;
	VkDeviceMemory skyboxImageMemory;
};

struct BufferContext {
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkCommandPool commandPool = VK_NULL_HANDLE;
};

struct RenderData {
	GLFWwindow* window = nullptr;

	VulkanInstance			vkInst{};
	VulkanSwapchain			swapchain{};
	VulkanRenderPass		renderPass{};
	VulkanDescriptors		descriptors{};
	VulkanPipeline			pipeline{};
	VulkanFramebuffers		framebuffers{};
	VulkanCommandBuffers	commandBuffers{};
	VulkanVertexBuffer		vertexBuffer{};
	VulkanUniformBuffer		uniformBuffer{};
	VulkanStorageBuffer		storageBuffer{};
	VulkanImages			images{};

	VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;
};
