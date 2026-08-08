#pragma once

#include <vulkan/vulkan.h>

#include "helper_functions.h"
#include "../scene/geometry_builder.h"

class RenderData;

class ImageUtils {
public:
	ImageUtils(RenderData& rData);

	VkImageView createCubeImageView(VkImage image, VkFormat format);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void createCubeImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToCubeImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void transitionCubeImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void createSkyboxTexture();
private:
	HelperFunctions helperFunctions;
	RenderData& renderData;
	GeometryBuilder geometryBuilder;

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
};
