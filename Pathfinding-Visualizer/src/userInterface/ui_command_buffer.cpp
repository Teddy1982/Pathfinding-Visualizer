#include "ui_command_buffer.h"
#include "../tools/logger.h"

// Initialisiert CommandBuffer für das Benutzermenü (ImGui)
bool CommandBuffer::init(RenderData &renderData, VkCommandBuffer &commandBuffer) {
  VkCommandBufferAllocateInfo bufferAllocInfo{};
  bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  bufferAllocInfo.commandPool = renderData.commandBuffers.commandPool;
  bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  bufferAllocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(renderData.vkInst.device, &bufferAllocInfo, &commandBuffer) != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate command buffers\n", __FUNCTION__);
    return false;
  }

  return true;
}

// gibt Speicher für CommandBuffer frei
void CommandBuffer::cleanup(RenderData &renderData, VkCommandBuffer &commandBuffer) {
  vkFreeCommandBuffers(renderData.vkInst.device, renderData.commandBuffers.commandPool, 1, &commandBuffer);
}
