#pragma once

#include <utility>
#include <array>
#include <vulkan/vulkan.h>

// Vertexklasse für Darstellung eines Knotens als Quadrat (für 2D-Darstellung)
struct QuadVertex {
    float position[3];
    int id[3];

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};