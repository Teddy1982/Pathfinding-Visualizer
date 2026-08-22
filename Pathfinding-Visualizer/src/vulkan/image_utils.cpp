#include "image_utils.h"
#include "renderData.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stdexcept>
#include <vulkan/vulkan.h>

ImageUtils::ImageUtils(RenderData& rData) : renderData(rData) {}

// Erstellen der Bilderansicht für einen Würfel (Skybox)
VkImageView ImageUtils::createCubeImageView(VkImage image, VkFormat format) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 6;

	VkImageView imageView;
	if (vkCreateImageView(renderData.vkInst.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create cube image view!");
	}

	return imageView;
}

// Erstellt eine 2D-Image-View für den Zugriff auf ein Vulkan-Image
VkImageView ImageUtils::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	
	// Legt den sichtbaren Bereich des Images fest
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(renderData.vkInst.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image view!");
	}

	return imageView;
}

// Erstellt ein Cube-Image mit sechs Ebenen und weist ihm GPU-Speicher zu
void ImageUtils::createCubeImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	
	// Ein Cube-Image besteht aus sechs Bildebenen
	imageInfo.arrayLayers = 6;

	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(renderData.vkInst.device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create cube image!");
	}

	// Ermittelt den Speicherbedarf des Images
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(renderData.vkInst.device, image, &memRequirements);

	// Reserviert einen geeigneten Speicherbereich
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = helperFunctions.findMemoryType(memRequirements.memoryTypeBits, properties, renderData.vkInst.physicalDevice);

	if (vkAllocateMemory(renderData.vkInst.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate cube image memory!");
	}

	vkBindImageMemory(renderData.vkInst.device, image, imageMemory, 0);
}

// Erstellt ein zweidimensionales Vulkan-Image und weist ihm GPU-Speicher zu
void ImageUtils::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags = 0;

	if (vkCreateImage(renderData.vkInst.device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(renderData.vkInst.device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = helperFunctions.findMemoryType(
		memRequirements.memoryTypeBits,
		properties,
		renderData.vkInst.physicalDevice
	);

	if (vkAllocateMemory(renderData.vkInst.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(renderData.vkInst.device, image, imageMemory, 0);
}

// Ändert das Speicherlayout eines zweidimensionalen Images
void ImageUtils::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,	VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = helperFunctions.beginSingleTimeCommands(renderData.commandBuffers.commandPool, renderData.vkInst.device);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage,
		destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	helperFunctions.endSingleTimeCommands(commandBuffer,renderData.vkInst.graphicsQueue, renderData.vkInst.device, renderData.commandBuffers.commandPool);
}

// Kopiert die sechs Bildflächen eines Buffers in ein Cube-Image
void ImageUtils::copyBufferToCubeImage(VkBuffer buffer,	VkImage image, uint32_t width, uint32_t height) {
	std::array<VkBufferImageCopy, 6> regions{};

	VkDeviceSize faceSize = width * height * 4;

	for (uint32_t face = 0; face < 6; face++) {
		regions[face].bufferOffset = faceSize * face;
		regions[face].bufferRowLength = 0;
		regions[face].bufferImageHeight = 0;

		regions[face].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		regions[face].imageSubresource.mipLevel = 0;
		regions[face].imageSubresource.baseArrayLayer = face;
		regions[face].imageSubresource.layerCount = 1;

		regions[face].imageOffset = { 0, 0, 0 };
		regions[face].imageExtent = { width, height, 1 };
	}

	VkCommandBuffer commandBuffer = helperFunctions.beginSingleTimeCommands(renderData.commandBuffers.commandPool, renderData.vkInst.device);

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		static_cast<uint32_t>(regions.size()),
		regions.data()
	);

	helperFunctions.endSingleTimeCommands(commandBuffer, renderData.vkInst.graphicsQueue, renderData.vkInst.device, renderData.commandBuffers.commandPool);
}

// Ändert das Speicherlayout aller sechs Ebenen eines Cube-Images
void ImageUtils::transitionCubeImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,	VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = helperFunctions.beginSingleTimeCommands(renderData.commandBuffers.commandPool, renderData.vkInst.device);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 6;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage,
		destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	helperFunctions.endSingleTimeCommands(commandBuffer,renderData.vkInst.graphicsQueue, renderData.vkInst.device, renderData.commandBuffers.commandPool);
}

// Kopiert die Bilddaten eines Buffers in ein zweidimensionales Image
void ImageUtils::copyBufferToImage(VkBuffer buffer,	VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = helperFunctions.beginSingleTimeCommands(renderData.commandBuffers.commandPool, renderData.vkInst.device);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;   // 0 = dicht gepackt
	region.bufferImageHeight = 0; // 0 = dicht gepackt

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	helperFunctions.endSingleTimeCommands(commandBuffer, renderData.vkInst.graphicsQueue, renderData.vkInst.device, renderData.commandBuffers.commandPool);
}

// Lädt die Skybox-Textur und erstellt daraus ein verwendbares Vulkan-Cubemap-Image
void ImageUtils::createSkyboxTexture() {
	int imgWidth, imgHeight, imgChannels;
	unsigned char* imageData = stbi_load(
		"assets/cubemap_sunny_sky.png",
		&imgWidth,
		&imgHeight,
		&imgChannels,
		STBI_rgb_alpha
	);

	if (!imageData) {
		throw std::runtime_error("Failed to load skybox texture!");
	}

	// Prüft, ob das Bild im erwarteten 4×3-Cubemap-Format vorliegt
	const uint32_t faceSize = imgWidth / 4;

	if (imgWidth != faceSize * 4 || imgHeight != faceSize * 3) {
		throw std::runtime_error("Skybox image must be 4x3 cubemap cross");
	}

	const VkDeviceSize faceImageSize = faceSize * faceSize * 4;
	const VkDeviceSize imageSize = faceImageSize * 6;

	// Erstellt einen CPU-seitig beschreibbaren Staging Buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	// Kopiert die sechs Flächen aus dem Kreuzlayout hintereinander
    // in den Staging Buffer
	void* data;
	vkMapMemory(renderData.vkInst.device, stagingBufferMemory, 0, imageSize, 0, &data);

	unsigned char* dst = reinterpret_cast<unsigned char*>(data);

	for (uint32_t face = 0; face < 6; face++) {
		for (uint32_t y = 0; y < faceSize; y++) {
			unsigned char* srcRow =
				imageData +
				(((geometryBuilder.skyboxFaces[face].y * faceSize + y) * imgWidth) +
					(geometryBuilder.skyboxFaces[face].x * faceSize)) * 4;

			unsigned char* dstRow =
				dst + face * faceImageSize + y * faceSize * 4;

			memcpy(dstRow, srcRow, faceSize * 4);
		}
	}

	vkUnmapMemory(renderData.vkInst.device, stagingBufferMemory);
	stbi_image_free(imageData);

	// Erstellt das endgültige Cube-Image im GPU-Speicher
	createCubeImage(
		faceSize,
		faceSize,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		renderData.images.skyboxImage,
		renderData.images.skyboxImageMemory
	);

	// Bereitet das Image für den Datentransfer vor
	transitionCubeImageLayout(
		renderData.images.skyboxImage,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	// Überträgt die sechs Flächen in das Cube-Image
	copyBufferToCubeImage(
		stagingBuffer,
		renderData.images.skyboxImage,
		faceSize,
		faceSize
	);

	// Bereitet die Cubemap für den Zugriff im Shader vor
	transitionCubeImageLayout(
		renderData.images.skyboxImage,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	// Der temporäre Staging Buffer wird nicht mehr benötigt
	vkDestroyBuffer(renderData.vkInst.device, stagingBuffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, stagingBufferMemory, nullptr);

	// Erstellt die Image-View und den Sampler für den Shaderzugriff
	renderData.images.skyboxImageView = createCubeImageView(
		renderData.images.skyboxImage,
		VK_FORMAT_R8G8B8A8_SRGB
	);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	if (vkCreateSampler(renderData.vkInst.device, &samplerInfo, nullptr,
		&renderData.images.skyboxSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create skybox sampler!");
	}
}

// Erstellt einen Vulkan-Buffer, reserviert geeigneten Speicher
// und verknüpft beide Objekte miteinander
void ImageUtils::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(renderData.vkInst.device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(renderData.vkInst.device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = helperFunctions.findMemoryType(memRequirements.memoryTypeBits, properties, renderData.vkInst.physicalDevice);

	if (vkAllocateMemory(renderData.vkInst.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate buffer memory!");
	}

	if (vkBindBufferMemory(renderData.vkInst.device, buffer, bufferMemory, 0) != VK_SUCCESS) {
		throw std::runtime_error("Failed to bind buffer memory!");
	}
}
