#pragma once

#include <stdint.h>

struct RenderData;
class VulkanContext;
class GeometryBuilder;
class UserInterface;
struct AppState;

class Renderer {
public:
	Renderer();
	Renderer(RenderData& rData, VulkanContext& vkContext, GeometryBuilder& geoBuilder, UserInterface& ui, AppState& state);

	void drawFrame();
	void recordCommandBuffer(uint32_t imageIndex);

private:
	
	RenderData& renderData;
	VulkanContext& vulkanContext;
	GeometryBuilder& geometryBuilder;
	UserInterface& userInterface;
	AppState& appState;
};
