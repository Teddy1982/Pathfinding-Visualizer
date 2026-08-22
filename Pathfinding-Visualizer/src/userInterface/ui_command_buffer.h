#pragma once

#include <vulkan/vulkan.h>

#include "../vulkan/renderData.h"

// CommandBuffer-Klasse für Benutzermenü (ImGui)
class CommandBuffer {
  public:
    static bool init(RenderData &renderData, VkCommandBuffer &commandBuffer);
    static void cleanup(RenderData &renderData, VkCommandBuffer &commandBuffer);
};
