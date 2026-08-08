#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

#include "helper_functions.h"
#include "image_utils.h"

struct RenderData;

class VulkanContext {
public:

	VulkanContext(RenderData& rData);

	void createVulkanInstance(GLFWwindow* window);
	void createSwapchain();
	void createRenderPass();
	void createVulkanDescriptors();
	void createGraphicsPipelines();
	void createCommandPool();
	void createCommandBuffers();
	void createSemaphores();
	void createFramebuffers();
	void recreateRenderPass();
	void recreateSwapchain();
	void cleanupSwapchain();

private:
	RenderData& renderData;
	HelperFunctions helperFunctions;

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

public:
	VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
	VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
};