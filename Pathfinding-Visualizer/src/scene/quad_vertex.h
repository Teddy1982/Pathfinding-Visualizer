#pragma once

#include <utility>
//#include <vector>
#include <array>
#include <vulkan/vulkan.h>

struct QuadVertex {
    float position[3];   // x, y
    int id[3];           // Grid-Koordinaten x, y

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};