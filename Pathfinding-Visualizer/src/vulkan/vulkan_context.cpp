#include "vulkan_context.h"
#include "uniform_buffer_object.h"
#include "renderData.h"
#include "../scene/cube_vertex.h"
#include "../scene/cube_edge_vertex.h"
#include "../scene/connection_vertex.h"
#include "../scene/quad_vertex.h"
#include "../scene/quad_edge_vertex.h"
#include "../scene/skybox_structures.h"
#include "../application/app_state.h"

#include <stdexcept>
#include <algorithm>
#include <array>
#include <fstream>

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

VulkanContext::VulkanContext(RenderData& rData) : renderData(rData)
{ }

// Erstellt die Vulkan-Instanz, die Fensteroberfläche sowie das
// physische und logische Gerät einschließlich der Grafik-Queue
void VulkanContext::createVulkanInstance() {
	// Verhindert eine erneute Initialisierung
	if (renderData.vkInst.instance != VK_NULL_HANDLE) return;

	// Beschreibt die Anwendung und die verwendete Vulkan-Version
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	// Bereitet die Erstellung der Vulkan-Instanz vor
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Ermittelt die vom Vulkan-Treiber unterstützten
    // Instanz-Erweiterungen
	uint32_t count = 0;
	auto result = vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
	std::vector<VkExtensionProperties> instanceExtension;

	instanceExtension.resize(count);
	result = vkEnumerateInstanceExtensionProperties(NULL, &count, &instanceExtension[0]);

	// Aktiviert die Vulkan-Validierungsschicht
	const char* layers[] = { "VK_LAYER_KHRONOS_validation" };

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = layers;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	// Aktiviert die für die Fensterausgabe und
	// Debugmeldungen benötigten Erweiterungen
	std::vector<const char*> extensions = {
		"VK_KHR_surface",
		"VK_KHR_win32_surface"
	};

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	createInfo.enabledExtensionCount =
		static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	// Erstellt die zentrale Vulkan-Instanz
	if (vkCreateInstance(&createInfo, nullptr, &renderData.vkInst.instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan instance!");
	}

#ifdef ENABLE_VULKAN_DEBUG_CALLBACK
	{
		if (enableValidationLayers) {
			// Richtet Rückruffunktionen für Fehler-, Warnungs-
		// und Leistungsmeldungen der Validierungsschicht ein
			VkDebugReportCallbackEXT error_callback = VK_NULL_HANDLE;
			VkDebugReportCallbackEXT warning_callback = VK_NULL_HANDLE;

			PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = NULL;

			// Lädt die Erweiterungsfunktion zur Erstellung
			// eines Debug-Callbacks
			*(void**)&vkCreateDebugReportCallbackEXT = vkGetInstanceProcAddr(renderData.vkInst.instance, "vkCreateDebugReportCallbackEXT");
			DBG_ASSERT(vkCreateDebugReportCallbackEXT);

			VkDebugReportCallbackCreateInfoEXT cb_create_info = {};
			cb_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;

			// Erstellt den Callback für Vulkan-Fehlermeldungen
			cb_create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT;
			cb_create_info.pfnCallback = &MyDebugReportCallback;

			VkResult result = vkCreateDebugReportCallbackEXT(renderData.vkInst.instance, &cb_create_info, nullptr, &error_callback);
			DBG_ASSERT(result == VK_SUCCESS);

			// Erstellt einen weiteren Callback für Warnungen
			// und mögliche Leistungsprobleme
			cb_create_info.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			cb_create_info.pfnCallback = &MyDebugReportCallback;

			result = vkCreateDebugReportCallbackEXT(renderData.vkInst.instance, &cb_create_info, nullptr, &warning_callback);
			DBG_ASSERT(result == VK_SUCCESS);
		}
	}
#endif

	// Erstellt die Verbindung zwischen dem GLFW-Fenster
	// und der Vulkan-Instanz
	if (glfwCreateWindowSurface(renderData.vkInst.instance, renderData.window, nullptr, &renderData.vkInst.surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface!");
	}

	// Ermittelt die verfügbaren physischen Vulkan-Geräte
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(renderData.vkInst.instance, &deviceCount, nullptr);
	if (deviceCount == 0) throw std::runtime_error("No Vulkan -compatible GPUs found");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(renderData.vkInst.instance, &deviceCount, devices.data());
	// Verwendet vereinfachend das erste gefundene Gerät
	renderData.vkInst.physicalDevice = devices[0]; 

	// Konfiguriert eine Queue für Grafikbefehle
	float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = 0;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	// Bereitet die Erstellung des logischen Gerätes vor
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

	// Aktiviert die für die Bildschirmausgabe
	// benötigte Swapchain-Erweiterung
	const char* deviceExtensions[] = { "VK_KHR_swapchain" };
	deviceCreateInfo.enabledExtensionCount = 1;
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
	// Geräteschichten sind in aktuellen Vulkan-Versionen veraltet
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = layers;

	// Erstellt das logische Vulkan-Gerät
	if (vkCreateDevice(renderData.vkInst.physicalDevice, &deviceCreateInfo, nullptr, &renderData.vkInst.device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device!");
	}

	// Ruft die Grafik-Queue aus dem logischen Gerät ab
	vkGetDeviceQueue(renderData.vkInst.device, 0, 0, &renderData.vkInst.graphicsQueue);
}

// Erstellt die Swapchain einschließlich ihrer Bilder und Image-Views
void VulkanContext::createSwapchain() {
	// Verhindert eine erneute Erstellung bei bereits vorhandener Swapchain
	if (renderData.swapchain.swapchain != VK_NULL_HANDLE) return;

	// Ermittelt die Eigenschaften und Grenzen der Fensteroberfläche
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderData.vkInst.physicalDevice, renderData.vkInst.surface, &surfaceCapabilities);

	// Ermittelt die unterstützten Bild- und Farbraumformate
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(renderData.vkInst.physicalDevice, renderData.vkInst.surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(renderData.vkInst.physicalDevice, renderData.vkInst.surface, &formatCount, formats.data());

	// Verwendet standardmäßig das erste unterstützte Format
	VkSurfaceFormatKHR chosenFormat = formats[0];
	
	// Bevorzugt ein SRGB-Format für eine korrekte Farbdarstellung
	for (const auto& format : formats) {
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			chosenFormat = format;
			break;
		}
	}

	// Ermittelt die unterstützten Präsentationsmodi
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(renderData.vkInst.physicalDevice, renderData.vkInst.surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(renderData.vkInst.physicalDevice, renderData.vkInst.surface, &presentModeCount, presentModes.data());

	// FIFO wird von Vulkan garantiert unterstützt
	// und entspricht einer Darstellung mit V-Sync
	VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	// Mailbox wird bevorzugt, sofern der Treiber ihn unterstützt
	for (const auto& mode : presentModes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			chosenPresentMode = mode;
			break;
		}
	}

	// Übernimmt nach Möglichkeit die von der Oberfläche
    // vorgegebene Bildgröße
	VkExtent2D extent = surfaceCapabilities.currentExtent;
	
	// Falls keine feste Größe vorgegeben ist, wird eine
	// gültige Ausgangsgröße innerhalb der Grenzen gewählt
	if (extent.width == UINT32_MAX) { 
		extent.width = std::clamp(640u, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
		extent.height = std::clamp(480u, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
	}

	// Verwendet ein Bild mehr als die mindestens benötigte Anzahl,
	// sofern die Oberfläche dies erlaubt
	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
		imageCount = surfaceCapabilities.maxImageCount;
	}

	// Die Bilder werden ausschließlich von einer Queue-Familie verwendet
	VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	uint32_t queueFamilyIndexCount = 0;

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = renderData.vkInst.surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = chosenFormat.format;
	createInfo.imageColorSpace = chosenFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	
	// Die Swapchain-Bilder dienen als Farbausgabe des Render Passes
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = sharingMode;
	createInfo.queueFamilyIndexCount = queueFamilyIndexCount;
	createInfo.pQueueFamilyIndices = 0;
	createInfo.preTransform = surfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = chosenPresentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	// Erstellt die konfigurierte Swapchain
	if (vkCreateSwapchainKHR(renderData.vkInst.device, &createInfo, nullptr, &renderData.swapchain.swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swapchain!");
	}

	// Ruft die von der Swapchain bereitgestellten Bilder ab
	vkGetSwapchainImagesKHR(renderData.vkInst.device, renderData.swapchain.swapchain, &imageCount, nullptr);
	renderData.swapchain.images.resize(imageCount);
	vkGetSwapchainImagesKHR(renderData.vkInst.device, renderData.swapchain.swapchain, &imageCount, renderData.swapchain.images.data());

	// Speichert Format und Größe für weitere Renderressourcen
	renderData.swapchain.imageFormat = chosenFormat.format;
	renderData.swapchain.extent = extent;

	// Erstellt für jedes Swapchain-Bild eine Image-View
	renderData.swapchain.imageViews.resize(renderData.swapchain.images.size());
	for (size_t i = 0; i < renderData.swapchain.images.size(); i++) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = renderData.swapchain.images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = renderData.swapchain.imageFormat;
		
		// Verwendet die Farbkanäle des Images unverändert
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		
		// Die Image-View umfasst die Farbkomponente
		// der einzigen Mipmap- und Bildebene
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(renderData.vkInst.device, &createInfo, nullptr, &renderData.swapchain.imageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image views!");
		}
	}
}

// Erstellt einen Render Pass mit Farb- und Tiefenpuffer
void VulkanContext::createRenderPass() {
	if (renderData.renderPass.renderPass != VK_NULL_HANDLE) return;

	// Konfiguriert die Farbausgabe für die spätere Präsentation
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Konfiguriert den Tiefenpuffer für den Depth Test
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = VK_FORMAT_D32_SFLOAT;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Verknüpft Farb- und Tiefenpuffer mit dem Grafik-Subpass
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(renderData.vkInst.device, &renderPassInfo, nullptr, &renderData.renderPass.renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass!");
	}
}

// Erstellt und aktualisiert die Descriptor Sets für die linke
// und rechte Darstellung
void VulkanContext::createVulkanDescriptors() {
	if (renderData.descriptors.layout != VK_NULL_HANDLE) {
		return;
	}

	constexpr uint32_t descriptorSetCount = 2;

	// =========================================================
	// Descriptor-Set-Layout
	// =========================================================
	// Definiert die Shaderzugriffe auf Uniform Buffer,
	// Storage Buffer und Skybox-Textur
	std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].pImmutableSamplers = nullptr;

	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags =
		VK_SHADER_STAGE_VERTEX_BIT |
		VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[1].pImmutableSamplers = nullptr;

	bindings[2].binding = 2;
	bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[2].descriptorCount = 1;
	bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[2].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(renderData.vkInst.device, &layoutInfo, nullptr, &renderData.descriptors.layout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout!");
	}

	// Reserviert Deskriptoren für zwei vollständige Descriptor Sets
	std::array<VkDescriptorPoolSize, 3> poolSizes{};

	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = descriptorSetCount;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[1].descriptorCount = descriptorSetCount;

	poolSizes[2].type =	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = descriptorSetCount;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = descriptorSetCount;

	if (vkCreateDescriptorPool(renderData.vkInst.device, &poolInfo,	nullptr, &renderData.descriptors.pool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool!");
	}

	// =========================================================
	// Zwei Descriptor Sets allokieren
	// =========================================================
	// Allokiert je ein Descriptor Set für die linke
	// und rechte Vergleichsansicht
	std::array<VkDescriptorSetLayout, descriptorSetCount> descriptorLayouts = {
		renderData.descriptors.layout,
		renderData.descriptors.layout
	};

	std::array<VkDescriptorSet, descriptorSetCount>	descriptorSets = {
		VK_NULL_HANDLE,
		VK_NULL_HANDLE
	};

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = renderData.descriptors.pool;
	allocInfo.descriptorSetCount = descriptorSetCount;
	allocInfo.pSetLayouts = descriptorLayouts.data();

	if (vkAllocateDescriptorSets(renderData.vkInst.device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	renderData.descriptors.descriptorSet = descriptorSets[0];
	renderData.descriptors.descriptorSetRight =	descriptorSets[1];

	// Sicherheitsprüfung
	if (renderData.descriptors.descriptorSet ==	VK_NULL_HANDLE || renderData.descriptors.descriptorSetRight == VK_NULL_HANDLE) {
		throw std::runtime_error("Allocated descriptor set is VK_NULL_HANDLE!");
	}

	VkDescriptorBufferInfo uniformBufferInfo{};
	uniformBufferInfo.buffer = renderData.uniformBuffer.buffer;
	uniformBufferInfo.offset = 0;
	uniformBufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorImageInfo skyboxImageInfo{};
	skyboxImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	skyboxImageInfo.imageView =	renderData.images.skyboxImageView;
	skyboxImageInfo.sampler = renderData.images.skyboxSampler;

	// =========================================================
	// Hilfsfunktion zum Aktualisieren eines Sets
	// =========================================================
	// Verknüpft ein Descriptor Set mit dem gemeinsamen Uniform Buffer,
	// seinem Storage Buffer und der Skybox-Textur
	auto updateDescriptorSet = [&](VkDescriptorSet descriptorSet, VkBuffer storageBuffer) {
		if (descriptorSet == VK_NULL_HANDLE) {
			throw std::runtime_error("Cannot update VK_NULL_HANDLE descriptor set!");
		}
		
		if (storageBuffer == VK_NULL_HANDLE) {
			throw std::runtime_error("Cannot bind VK_NULL_HANDLE storage buffer!");
		}

		VkDescriptorBufferInfo storageBufferInfo{};
		storageBufferInfo.buffer = storageBuffer;
		storageBufferInfo.offset = 0;
		storageBufferInfo.range = VK_WHOLE_SIZE;

		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

		// Binding 0: UBO
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

		// Binding 1: Storage Buffer
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &storageBufferInfo;

		// Binding 2: Skybox-Sampler
		descriptorWrites[2].sType =	VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSet;
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = &skyboxImageInfo;

		vkUpdateDescriptorSets(renderData.vkInst.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	};

	// =========================================================
	// Linkes und rechtes Set aktualisieren
	// =========================================================
	// Die Ansichten teilen sich UBO und Skybox,
	// verwenden jedoch getrennte Storage Buffer
	updateDescriptorSet(renderData.descriptors.descriptorSet, renderData.storageBuffer.buffer);
	updateDescriptorSet(renderData.descriptors.descriptorSetRight, renderData.storageBuffer.bufferRight);
}

// Erstellt die Grafik-Pipelines für die 2D- und 3D-Darstellung,
// den Pfad und die Skybox
void VulkanContext::createGraphicsPipelines() {
	// Verhindert die erneute Erstellung bereits vorhandener Pipelines
	if (renderData.pipeline.cubePipeline != VK_NULL_HANDLE || renderData.pipeline.cubeEdgePipeline != VK_NULL_HANDLE || renderData.pipeline.connectionPipeline != VK_NULL_HANDLE ||
		renderData.pipeline.quadPipeline != VK_NULL_HANDLE || renderData.pipeline.quadEdgePipeline != VK_NULL_HANDLE)
		return;

	// ---------- Shader laden ----------
	// Lädt die kompilierten Vertex- und Fragment-Shader
	VkShaderModule cubeVertShaderModule = helperFunctions.createShaderModule(renderData.vkInst.device, "shader/cube.vert.spv");
	VkShaderModule cubeEdgeVertShaderModule = helperFunctions.createShaderModule(renderData.vkInst.device, "shader/cubeEdge.vert.spv");
	VkShaderModule connectionVertShaderModule = helperFunctions.createShaderModule(renderData.vkInst.device, "shader/connection.vert.spv");
	VkShaderModule cubeFragShaderModule = helperFunctions.createShaderModule(renderData.vkInst.device, "shader/cube.frag.spv");
	VkShaderModule cubeEdgeFragShaderModule = helperFunctions.createShaderModule(renderData.vkInst.device, "shader/cubeEdge.frag.spv");
	VkShaderModule connectionFragShaderModule = helperFunctions.createShaderModule(renderData.vkInst.device, "shader/connection.frag.spv");
	VkShaderModule skyboxVertShaderModule = helperFunctions.createShaderModule(renderData.vkInst.device, "shader/skybox.vert.spv");
	VkShaderModule skyboxFragShaderModule = helperFunctions.createShaderModule(renderData.vkInst.device, "shader/skybox.frag.spv");

	VkShaderModule quadVertShaderModule = helperFunctions.createShaderModule(renderData.vkInst.device, "shader/quad.vert.spv");
	VkShaderModule quadEdgeVertShaderModule = helperFunctions.createShaderModule(renderData.vkInst.device, "shader/quadEdge.vert.spv");
	VkShaderModule quadFragShaderModule = helperFunctions.createShaderModule(renderData.vkInst.device, "shader/quad.frag.spv");
	VkShaderModule quadEdgeFragShaderModule = helperFunctions.createShaderModule(renderData.vkInst.device, "shader/quadEdge.frag.spv");

	// Konfiguriert gemeinsame, dynamische Pipeline-Zustände
	VkPipelineShaderStageCreateInfo cubeVertShaderStageInfo{};
	cubeVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	cubeVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	cubeVertShaderStageInfo.module = cubeVertShaderModule;
	cubeVertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo cubeEdgeVertShaderStageInfo{};
	cubeEdgeVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	cubeEdgeVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	cubeEdgeVertShaderStageInfo.module = cubeEdgeVertShaderModule;
	cubeEdgeVertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo cubeFragShaderStageInfo{};
	cubeFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	cubeFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	cubeFragShaderStageInfo.module = cubeFragShaderModule;
	cubeFragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo cubeEdgeFragShaderStageInfo{};
	cubeEdgeFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	cubeEdgeFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	cubeEdgeFragShaderStageInfo.module = cubeEdgeFragShaderModule;
	cubeEdgeFragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo connectionVertShaderStageInfo{};
	connectionVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	connectionVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	connectionVertShaderStageInfo.module = connectionVertShaderModule;
	connectionVertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo connectionFragShaderStageInfo{};
	connectionFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	connectionFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	connectionFragShaderStageInfo.module = connectionFragShaderModule;
	connectionFragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo skyboxVertShaderStageInfo{};
	skyboxVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	skyboxVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	skyboxVertShaderStageInfo.module = skyboxVertShaderModule;
	skyboxVertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo skyboxFragShaderStageInfo{};
	skyboxFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	skyboxFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	skyboxFragShaderStageInfo.module = skyboxFragShaderModule;
	skyboxFragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo quadVertShaderStageInfo{};
	quadVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	quadVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	quadVertShaderStageInfo.module = quadVertShaderModule;
	quadVertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo quadEdgeVertShaderStageInfo{};
	quadEdgeVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	quadEdgeVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	quadEdgeVertShaderStageInfo.module = quadEdgeVertShaderModule;
	quadEdgeVertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo quadFragShaderStageInfo{};
	quadFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	quadFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	quadFragShaderStageInfo.module = quadFragShaderModule;
	quadFragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo quadEdgeFragShaderStageInfo{};
	quadEdgeFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	quadEdgeFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	quadEdgeFragShaderStageInfo.module = quadEdgeFragShaderModule;
	quadEdgeFragShaderStageInfo.pName = "main";

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = nullptr;
	viewportState.scissorCount = 1;
	viewportState.pScissors = nullptr;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = 2;
	dynamicStateInfo.pDynamicStates = dynamicStates;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Erstellt das gemeinsame Pipeline-Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &renderData.descriptors.layout;

	if (vkCreatePipelineLayout(renderData.vkInst.device, &pipelineLayoutInfo, nullptr, &renderData.pipeline.layout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout!");
	}

	// =========================================================
	// CUBE PIPELINE
	// =========================================================
	// Erstellt die Pipeline für die gefüllten 3D-Gridwürfel
	auto cubeBindingDescription = CubeVertex::getBindingDescription();
	auto cubeAttributeDescriptions = CubeVertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo cubeVertexInputInfo{};
	cubeVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	cubeVertexInputInfo.vertexBindingDescriptionCount = 1;
	cubeVertexInputInfo.pVertexBindingDescriptions = &cubeBindingDescription;
	cubeVertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(cubeAttributeDescriptions.size());
	cubeVertexInputInfo.pVertexAttributeDescriptions = cubeAttributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo cubeInputAssembly{};
	cubeInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	cubeInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	cubeInputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo cubeRasterizer{};
	cubeRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	cubeRasterizer.depthClampEnable = VK_FALSE;
	cubeRasterizer.rasterizerDiscardEnable = VK_FALSE;
	cubeRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	cubeRasterizer.lineWidth = 1.0f;
	cubeRasterizer.cullMode = VK_CULL_MODE_NONE;
	cubeRasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	cubeRasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState cubeColorBlendAttachment{};
	cubeColorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;

	cubeColorBlendAttachment.blendEnable = VK_TRUE;
	cubeColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	cubeColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	cubeColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	cubeColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	cubeColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	cubeColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo cubeColorBlending{};
	cubeColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	cubeColorBlending.logicOpEnable = VK_FALSE;
	cubeColorBlending.attachmentCount = 1;
	cubeColorBlending.pAttachments = &cubeColorBlendAttachment;

	VkPipelineDepthStencilStateCreateInfo cubeDepthStencil{};
	cubeDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	cubeDepthStencil.depthTestEnable = VK_TRUE;
	cubeDepthStencil.depthWriteEnable = VK_TRUE;
	cubeDepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	cubeDepthStencil.depthBoundsTestEnable = VK_FALSE;
	cubeDepthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineShaderStageCreateInfo cubeShaderStages[] = {
		cubeVertShaderStageInfo,
		cubeFragShaderStageInfo
	};

	VkGraphicsPipelineCreateInfo cubePipelineInfo{};
	cubePipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	cubePipelineInfo.stageCount = 2;
	cubePipelineInfo.pStages = cubeShaderStages;
	cubePipelineInfo.pVertexInputState = &cubeVertexInputInfo;
	cubePipelineInfo.pInputAssemblyState = &cubeInputAssembly;
	cubePipelineInfo.pViewportState = &viewportState;
	cubePipelineInfo.pRasterizationState = &cubeRasterizer;
	cubePipelineInfo.pMultisampleState = &multisampling;
	cubePipelineInfo.pDepthStencilState = &cubeDepthStencil;
	cubePipelineInfo.pColorBlendState = &cubeColorBlending;
	cubePipelineInfo.pDynamicState = &dynamicStateInfo;
	cubePipelineInfo.layout = renderData.pipeline.layout;
	cubePipelineInfo.renderPass = renderData.renderPass.renderPass;
	cubePipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(renderData.vkInst.device, VK_NULL_HANDLE, 1, &cubePipelineInfo, nullptr, &renderData.pipeline.cubePipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create fill pipeline!");
	}

	// =========================================================
	// CUBEEDGE PIPELINE
	// =========================================================
	// Erstellt die Pipeline für die Kanten der 3D-Gridwürfel
	VkPipelineInputAssemblyStateCreateInfo cubeEdgeInputAssembly{};
	cubeEdgeInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	cubeEdgeInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	cubeEdgeInputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo cubeEdgeRasterizer{};
	cubeEdgeRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	cubeEdgeRasterizer.depthClampEnable = VK_FALSE;
	cubeEdgeRasterizer.rasterizerDiscardEnable = VK_FALSE;
	cubeEdgeRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	cubeEdgeRasterizer.lineWidth = 1.0f;
	cubeEdgeRasterizer.cullMode = VK_CULL_MODE_NONE;
	cubeEdgeRasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	cubeEdgeRasterizer.depthBiasEnable = VK_TRUE;

	VkPipelineColorBlendAttachmentState cubeEdgeColorBlendAttachment{};
	cubeEdgeColorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	cubeEdgeColorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo cubeEdgeColorBlending{};
	cubeEdgeColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	cubeEdgeColorBlending.logicOpEnable = VK_FALSE;
	cubeEdgeColorBlending.attachmentCount = 1;
	cubeEdgeColorBlending.pAttachments = &cubeEdgeColorBlendAttachment;

	VkPipelineDepthStencilStateCreateInfo cubeEdgeDepthStencil{};
	cubeEdgeDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	cubeEdgeDepthStencil.depthTestEnable = VK_TRUE;
	cubeEdgeDepthStencil.depthWriteEnable = VK_TRUE;
	cubeEdgeDepthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	cubeEdgeDepthStencil.depthBoundsTestEnable = VK_FALSE;
	cubeEdgeDepthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineShaderStageCreateInfo cubeEdgeShaderStages[] = {
		cubeEdgeVertShaderStageInfo,
		cubeEdgeFragShaderStageInfo
	};
	auto cubeEdgeBindingDescription = CubeEdgeVertex::getBindingDescription();
	auto cubeEdgeAttributeDescriptions = CubeEdgeVertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo cubeEdgeVertexInputInfo{};
	cubeEdgeVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	cubeEdgeVertexInputInfo.vertexBindingDescriptionCount = 1;
	cubeEdgeVertexInputInfo.pVertexBindingDescriptions = &cubeEdgeBindingDescription;
	cubeEdgeVertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(cubeEdgeAttributeDescriptions.size());
	cubeEdgeVertexInputInfo.pVertexAttributeDescriptions = cubeEdgeAttributeDescriptions.data();

	VkGraphicsPipelineCreateInfo cubeEdgePipelineInfo{};
	cubeEdgePipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	cubeEdgePipelineInfo.stageCount = 2;
	cubeEdgePipelineInfo.pStages = cubeEdgeShaderStages;
	cubeEdgePipelineInfo.pVertexInputState = &cubeEdgeVertexInputInfo;
	cubeEdgePipelineInfo.pInputAssemblyState = &cubeEdgeInputAssembly;
	cubeEdgePipelineInfo.pViewportState = &viewportState;
	cubeEdgePipelineInfo.pRasterizationState = &cubeEdgeRasterizer;
	cubeEdgePipelineInfo.pMultisampleState = &multisampling;
	cubeEdgePipelineInfo.pDepthStencilState = &cubeEdgeDepthStencil;
	cubeEdgePipelineInfo.pColorBlendState = &cubeEdgeColorBlending;
	cubeEdgePipelineInfo.pDynamicState = &dynamicStateInfo;
	cubeEdgePipelineInfo.layout = renderData.pipeline.layout;
	cubeEdgePipelineInfo.renderPass = renderData.renderPass.renderPass;
	cubeEdgePipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(renderData.vkInst.device, VK_NULL_HANDLE, 1, &cubeEdgePipelineInfo, nullptr, &renderData.pipeline.cubeEdgePipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create line pipeline!");
	}

	// =========================================================
	// CONNECTION PIPELINE
	// =========================================================
	// Erstellt die Pipeline zur Darstellung des gefundenen Pfades
	VkPipelineInputAssemblyStateCreateInfo connectionInputAssembly{};
	connectionInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	connectionInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	connectionInputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo connectionRasterizer{};
	connectionRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	connectionRasterizer.depthClampEnable = VK_FALSE;
	connectionRasterizer.rasterizerDiscardEnable = VK_FALSE;
	connectionRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	connectionRasterizer.lineWidth = 1.0f;
	connectionRasterizer.cullMode = VK_CULL_MODE_NONE;
	connectionRasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	connectionRasterizer.depthBiasEnable = VK_TRUE;

	VkPipelineColorBlendAttachmentState connectionColorBlendAttachment{};
	connectionColorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	connectionColorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo connectionColorBlending{};
	connectionColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	connectionColorBlending.logicOpEnable = VK_FALSE;
	connectionColorBlending.attachmentCount = 1;
	connectionColorBlending.pAttachments = &connectionColorBlendAttachment;

	VkPipelineDepthStencilStateCreateInfo connectionDepthStencil{};
	connectionDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	connectionDepthStencil.depthTestEnable = VK_TRUE;
	connectionDepthStencil.depthWriteEnable = VK_TRUE;
	connectionDepthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	connectionDepthStencil.depthBoundsTestEnable = VK_FALSE;
	connectionDepthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineShaderStageCreateInfo connectionShaderStages[] = {
		connectionVertShaderStageInfo,
		connectionFragShaderStageInfo
	};
	auto connectionBindingDescription = ConnectionVertex::getBindingDescription();
	auto connectionAttributeDescription = ConnectionVertex::getAttributeDescription();

	VkPipelineVertexInputStateCreateInfo connectionVertexInputInfo{};
	connectionVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	connectionVertexInputInfo.vertexBindingDescriptionCount = 1;
	connectionVertexInputInfo.pVertexBindingDescriptions = &connectionBindingDescription;
	connectionVertexInputInfo.vertexAttributeDescriptionCount = 1;
	connectionVertexInputInfo.pVertexAttributeDescriptions = &connectionAttributeDescription;

	VkGraphicsPipelineCreateInfo connectionPipelineInfo{};
	connectionPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	connectionPipelineInfo.stageCount = 2;
	connectionPipelineInfo.pStages = connectionShaderStages;
	connectionPipelineInfo.pVertexInputState = &connectionVertexInputInfo;
	connectionPipelineInfo.pInputAssemblyState = &connectionInputAssembly;
	connectionPipelineInfo.pViewportState = &viewportState;
	connectionPipelineInfo.pRasterizationState = &connectionRasterizer;
	connectionPipelineInfo.pMultisampleState = &multisampling;
	connectionPipelineInfo.pDepthStencilState = &connectionDepthStencil;
	connectionPipelineInfo.pColorBlendState = &connectionColorBlending;
	connectionPipelineInfo.pDynamicState = &dynamicStateInfo;
	connectionPipelineInfo.layout = renderData.pipeline.layout;
	connectionPipelineInfo.renderPass = renderData.renderPass.renderPass;
	connectionPipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(renderData.vkInst.device, VK_NULL_HANDLE, 1, &connectionPipelineInfo, nullptr, &renderData.pipeline.connectionPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create line pipeline!");
	}

	// =========================================================
	// SKYBOX PIPELINE
	// =========================================================
	// Erstellt die Pipeline zur Darstellung der Skybox
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	std::array<VkVertexInputBindingDescription, 1> vertexBindings{};

	vertexBindings[0].binding = 0;
	vertexBindings[0].stride = sizeof(SkyboxVertex);
	vertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 1> attributes{};

	attributes[0].binding = 0;
	attributes[0].location = 0;
	attributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributes[0].offset = offsetof(SkyboxVertex, position);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = vertexBindings.data();
	vertexInputInfo.vertexAttributeDescriptionCount = 1;
	vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

	VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
	rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerInfo.depthClampEnable = VK_FALSE;
	rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerInfo.lineWidth = 1.0f;
	rasterizerInfo.cullMode = VK_CULL_MODE_NONE;
	rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerInfo.depthBiasEnable = VK_FALSE;

	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments{};
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachments.emplace_back(colorBlendAttachment);

	VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
	colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingInfo.logicOpEnable = VK_FALSE;
	colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingInfo.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
	colorBlendingInfo.pAttachments = colorBlendAttachments.data();
	colorBlendingInfo.blendConstants[0] = 0.0f;
	colorBlendingInfo.blendConstants[1] = 0.0f;
	colorBlendingInfo.blendConstants[2] = 0.0f;
	colorBlendingInfo.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
	depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilInfo.depthTestEnable = VK_TRUE;
	depthStencilInfo.depthWriteEnable = VK_FALSE;
	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilInfo.minDepthBounds = 0.0f;
	depthStencilInfo.maxDepthBounds = 1.0f;
	depthStencilInfo.stencilTestEnable = VK_FALSE;

	std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfo = { skyboxVertShaderStageInfo, skyboxFragShaderStageInfo };

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStagesInfo.size());
	pipelineCreateInfo.pStages = shaderStagesInfo.data();
	pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizerInfo;
	pipelineCreateInfo.pMultisampleState = &multisampling;
	pipelineCreateInfo.pColorBlendState = &colorBlendingInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
	pipelineCreateInfo.layout = renderData.pipeline.layout;
	pipelineCreateInfo.renderPass = renderData.renderPass.renderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(renderData.vkInst.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &renderData.pipeline.skyboxPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create line pipeline!");
	}

	// =========================================================
	// QUAD PIPELINE
	// =========================================================
	// Erstellt die Pipeline für die gefüllten 2D-Gridflächen
	auto quadBindingDescription = QuadVertex::getBindingDescription();
	auto quadAttributeDescriptions = QuadVertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo quadVertexInputInfo{};
	quadVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	quadVertexInputInfo.vertexBindingDescriptionCount = 1;
	quadVertexInputInfo.pVertexBindingDescriptions = &quadBindingDescription;
	quadVertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(quadAttributeDescriptions.size());
	quadVertexInputInfo.pVertexAttributeDescriptions = quadAttributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo quadInputAssembly{};
	quadInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	quadInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	quadInputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo quadRasterizer{};
	quadRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	quadRasterizer.depthClampEnable = VK_FALSE;
	quadRasterizer.rasterizerDiscardEnable = VK_FALSE;
	quadRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	quadRasterizer.lineWidth = 1.0f;
	quadRasterizer.cullMode = VK_CULL_MODE_NONE;
	quadRasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	quadRasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState quadColorBlendAttachment{};
	quadColorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;

	quadColorBlendAttachment.blendEnable = VK_TRUE;
	quadColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	quadColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	quadColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	quadColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	quadColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	quadColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo quadColorBlending{};
	quadColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	quadColorBlending.logicOpEnable = VK_FALSE;
	quadColorBlending.attachmentCount = 1;
	quadColorBlending.pAttachments = &quadColorBlendAttachment;

	VkPipelineDepthStencilStateCreateInfo quadDepthStencil{};
	quadDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	quadDepthStencil.depthTestEnable = VK_FALSE;
	quadDepthStencil.depthWriteEnable = VK_FALSE;
	quadDepthStencil.depthBoundsTestEnable = VK_FALSE;
	quadDepthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineShaderStageCreateInfo quadShaderStages[] = {
		quadVertShaderStageInfo,
		quadFragShaderStageInfo
	};

	VkGraphicsPipelineCreateInfo quadPipelineInfo{};
	quadPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	quadPipelineInfo.stageCount = 2;
	quadPipelineInfo.pStages = quadShaderStages;
	quadPipelineInfo.pVertexInputState = &quadVertexInputInfo;
	quadPipelineInfo.pInputAssemblyState = &quadInputAssembly;
	quadPipelineInfo.pViewportState = &viewportState;
	quadPipelineInfo.pRasterizationState = &quadRasterizer;
	quadPipelineInfo.pMultisampleState = &multisampling;
	quadPipelineInfo.pDepthStencilState = &quadDepthStencil;
	quadPipelineInfo.pColorBlendState = &quadColorBlending;
	quadPipelineInfo.pDynamicState = &dynamicStateInfo;
	quadPipelineInfo.layout = renderData.pipeline.layout;
	quadPipelineInfo.renderPass = renderData.renderPass.renderPass;
	quadPipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(renderData.vkInst.device, VK_NULL_HANDLE, 1, &quadPipelineInfo, nullptr, &renderData.pipeline.quadPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create fill pipeline!");
	}

	// =========================================================
	// QUADEDGE PIPELINE
	// =========================================================
	// Erstellt die Pipeline für die Kanten der 2D-Gridflächen
	VkPipelineInputAssemblyStateCreateInfo quadEdgeInputAssembly{};
	quadEdgeInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	quadEdgeInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	quadEdgeInputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo quadEdgeRasterizer{};
	quadEdgeRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	quadEdgeRasterizer.depthClampEnable = VK_FALSE;
	quadEdgeRasterizer.rasterizerDiscardEnable = VK_FALSE;
	quadEdgeRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	quadEdgeRasterizer.lineWidth = 1.0f;
	quadEdgeRasterizer.cullMode = VK_CULL_MODE_NONE;
	quadEdgeRasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	quadEdgeRasterizer.depthBiasEnable = VK_TRUE;

	VkPipelineColorBlendAttachmentState quadEdgeColorBlendAttachment{};
	quadEdgeColorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	quadEdgeColorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo quadEdgeColorBlending{};
	quadEdgeColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	quadEdgeColorBlending.logicOpEnable = VK_FALSE;
	quadEdgeColorBlending.attachmentCount = 1;
	quadEdgeColorBlending.pAttachments = &quadEdgeColorBlendAttachment;

	VkPipelineDepthStencilStateCreateInfo quadEdgeDepthStencil{};
	quadEdgeDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	quadEdgeDepthStencil.depthTestEnable = VK_FALSE;
	quadEdgeDepthStencil.depthWriteEnable = VK_FALSE;
	quadEdgeDepthStencil.depthBoundsTestEnable = VK_FALSE;
	quadEdgeDepthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineShaderStageCreateInfo quadEdgeShaderStages[] = {
		quadEdgeVertShaderStageInfo,
		quadEdgeFragShaderStageInfo
	};
	auto quadEdgeBindingDescription = QuadEdgeVertex::getBindingDescription();
	auto quadEdgeAttributeDescriptions = QuadEdgeVertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo quadEdgeVertexInputInfo{};
	quadEdgeVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	quadEdgeVertexInputInfo.vertexBindingDescriptionCount = 1;
	quadEdgeVertexInputInfo.pVertexBindingDescriptions = &quadEdgeBindingDescription;
	quadEdgeVertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(quadEdgeAttributeDescriptions.size());
	quadEdgeVertexInputInfo.pVertexAttributeDescriptions = quadEdgeAttributeDescriptions.data();

	VkGraphicsPipelineCreateInfo quadEdgePipelineInfo{};
	quadEdgePipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	quadEdgePipelineInfo.stageCount = 2;
	quadEdgePipelineInfo.pStages = quadEdgeShaderStages;
	quadEdgePipelineInfo.pVertexInputState = &quadEdgeVertexInputInfo;
	quadEdgePipelineInfo.pInputAssemblyState = &quadEdgeInputAssembly;
	quadEdgePipelineInfo.pViewportState = &viewportState;
	quadEdgePipelineInfo.pRasterizationState = &quadEdgeRasterizer;
	quadEdgePipelineInfo.pMultisampleState = &multisampling;
	quadEdgePipelineInfo.pDepthStencilState = &quadEdgeDepthStencil;
	quadEdgePipelineInfo.pColorBlendState = &quadEdgeColorBlending;
	quadEdgePipelineInfo.pDynamicState = &dynamicStateInfo;
	quadEdgePipelineInfo.layout = renderData.pipeline.layout;
	quadEdgePipelineInfo.renderPass = renderData.renderPass.renderPass;
	quadEdgePipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(renderData.vkInst.device, VK_NULL_HANDLE, 1, &quadEdgePipelineInfo, nullptr, &renderData.pipeline.quadEdgePipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create line pipeline!");
	}

	// ---------- Shader freigeben ----------
	// Die Shader-Module werden nach der Pipeline-Erstellung
	// nicht mehr benötigt.
	vkDestroyShaderModule(renderData.vkInst.device, cubeVertShaderModule, nullptr);
	vkDestroyShaderModule(renderData.vkInst.device, cubeEdgeVertShaderModule, nullptr);
	vkDestroyShaderModule(renderData.vkInst.device, cubeFragShaderModule, nullptr);
	vkDestroyShaderModule(renderData.vkInst.device, cubeEdgeFragShaderModule, nullptr);
	vkDestroyShaderModule(renderData.vkInst.device, connectionVertShaderModule, nullptr);
	vkDestroyShaderModule(renderData.vkInst.device, connectionFragShaderModule, nullptr);

	vkDestroyShaderModule(renderData.vkInst.device, quadVertShaderModule, nullptr);
	vkDestroyShaderModule(renderData.vkInst.device, quadEdgeVertShaderModule, nullptr);
	vkDestroyShaderModule(renderData.vkInst.device, quadFragShaderModule, nullptr);
	vkDestroyShaderModule(renderData.vkInst.device, quadEdgeFragShaderModule, nullptr);

	vkDestroyShaderModule(renderData.vkInst.device, skyboxVertShaderModule, nullptr);
	vkDestroyShaderModule(renderData.vkInst.device, skyboxFragShaderModule, nullptr);
}

// Erstellt den Command Pool für die verwendete Grafik-Queue
void VulkanContext::createCommandPool() {
	uint32_t queueFamilyIndex = 0;

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndex;
	// Erlaubt das individuelle Zurücksetzen der Command Buffer
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(renderData.vkInst.device, &poolInfo, nullptr, &renderData.commandBuffers.commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool!");
	}
}

// Erstellt für jedes Framebuffer einen primären Command Buffer
void VulkanContext::createCommandBuffers() {
	renderData.commandBuffers.commandBuffers.resize(renderData.framebuffers.framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = renderData.commandBuffers.commandPool;
	allocInfo.commandBufferCount = (uint32_t)renderData.commandBuffers.commandBuffers.size();

	if (vkAllocateCommandBuffers(renderData.vkInst.device, &allocInfo, renderData.commandBuffers.commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers!");
	}
}

// Erstellt die Semaphore zur Synchronisation von Rendering
// und Bildpräsentation
void VulkanContext::createSemaphores() {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(renderData.vkInst.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS || vkCreateSemaphore(renderData.vkInst.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create semaphores!");
	}
}

// Gibt die von der aktuellen Swapchain abhängigen Ressourcen frei
void VulkanContext::cleanupSwapchain() {
	for (auto framebuffer : renderData.framebuffers.framebuffers) {
		vkDestroyFramebuffer(renderData.vkInst.device, framebuffer, nullptr);
	}
	renderData.framebuffers.framebuffers.clear();

	vkDestroyImageView(renderData.vkInst.device, renderData.framebuffers.depthImageView, nullptr);
	vkDestroyImage(renderData.vkInst.device, renderData.framebuffers.depthImage, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.framebuffers.depthImageMemory, nullptr);

	renderData.framebuffers.depthImageView = VK_NULL_HANDLE;
	renderData.framebuffers.depthImage = VK_NULL_HANDLE;
	renderData.framebuffers.depthImageMemory = VK_NULL_HANDLE;

	for (auto imageView : renderData.swapchain.imageViews) {
		vkDestroyImageView(renderData.vkInst.device, imageView, nullptr);
	}
	renderData.swapchain.imageViews.clear();

	if (renderData.renderPass.renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(renderData.vkInst.device, renderData.renderPass.renderPass, nullptr);
		renderData.renderPass.renderPass = VK_NULL_HANDLE;
	}

	if (renderData.swapchain.swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(renderData.vkInst.device, renderData.swapchain.swapchain, nullptr);
		renderData.swapchain.swapchain = VK_NULL_HANDLE;
	}
}

// Erstellt den Tiefenpuffer und je ein Framebuffer
// für jedes Swapchain-Image
void VulkanContext::createFramebuffers() {
	if (!renderData.framebuffers.framebuffers.empty()) return;

	auto depthFormat = VK_FORMAT_D32_SFLOAT;

	// Das Tiefenbild wird von allen Framebuffern verwendet
	createImage(renderData.swapchain.extent.width, renderData.swapchain.extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderData.framebuffers.depthImage, renderData.framebuffers.depthImageMemory);
	renderData.framebuffers.depthImageView = createImageView(renderData.framebuffers.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	renderData.framebuffers.framebuffers.resize(renderData.swapchain.imageViews.size());

	for (size_t i = 0; i < renderData.swapchain.imageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			renderData.swapchain.imageViews[i],
			renderData.framebuffers.depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderData.renderPass.renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = &attachments[0];
		framebufferInfo.width = renderData.swapchain.extent.width;
		framebufferInfo.height = renderData.swapchain.extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(renderData.vkInst.device, &framebufferInfo, nullptr, &renderData.framebuffers.framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create framebuffer!");
		}
	}
}

// Erstellt den Render Pass und die zugehörigen Framebuffer neu
void VulkanContext::recreateRenderPass() {
	if (renderData.renderPass.renderPass) {
		for (auto framebuffer : renderData.framebuffers.framebuffers) {
			vkDestroyFramebuffer(renderData.vkInst.device, framebuffer, nullptr);
			renderData.framebuffers.depthImageMemory = VK_NULL_HANDLE;
		}
		vkDestroyRenderPass(renderData.vkInst.device, renderData.renderPass.renderPass, nullptr);
		renderData.renderPass.renderPass = VK_NULL_HANDLE;
	}
	renderData.framebuffers.framebuffers.clear();

	createRenderPass();
	renderData.framebuffers.framebuffers.resize(renderData.swapchain.images.size());
	for (uint32_t i = 0; i < renderData.swapchain.images.size(); ++i) {
		VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.renderPass = renderData.renderPass.renderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &renderData.swapchain.imageViews[i];
		createInfo.width = renderData.swapchain.extent.width;
		createInfo.height = renderData.swapchain.extent.height;
		createInfo.layers = 1;
		createFramebuffers();
	}
}

// Erstellt die Swapchain und alle von ihr abhängigen
// Ressourcen nach einer Größenänderung neu
void VulkanContext::recreateSwapchain() {
	vkDeviceWaitIdle(renderData.vkInst.device);

	int width = 0, height = 0;
	glfwGetFramebufferSize(renderData.window, &width, &height);

	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(renderData.window, &width, &height);
		glfwWaitEvents();
	}

	cleanupSwapchain();

	createSwapchain();
	createRenderPass();
	createFramebuffers();
	createCommandBuffers();
}

// Erstellt ein zweidimensionales Vulkan-Image und weist ihm
// einen geeigneten Speicherbereich zu
void VulkanContext::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
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

	// Ermittelt den Speicherbedarf und reserviert
	// einen passenden Speichertyp
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

	// Verknüpft das Image mit dem reservierten Speicher
	vkBindImageMemory(renderData.vkInst.device, image, imageMemory, 0);
}

// Erstellt eine zweidimensionale Image-View für den
// Zugriff auf ein Vulkan-Image
VkImageView VulkanContext::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	
	// Legt den sichtbaren Image-Bereich fest
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