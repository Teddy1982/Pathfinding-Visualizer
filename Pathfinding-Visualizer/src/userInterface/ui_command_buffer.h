/* Vulkan command buffers */
#pragma once

#include <vulkan/vulkan.h>

#include "../vulkan/renderData.h"

class CommandBuffer {
  public:
    static bool init(RenderData &renderData, VkCommandBuffer &commandBuffer);
    static void cleanup(RenderData &renderData, VkCommandBuffer &commandBuffer);
};
