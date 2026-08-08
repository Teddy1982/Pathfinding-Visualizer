#include "renderer.h"
#include "renderData.h"
#include "vulkan_context.h"
#include "uniform_buffer_object.h"
#include "../scene/geometry_builder.h"
#include "../tools/user_input_callbacks.h"
#include "../userInterface/user_interface.h"
#include "../application/app_state.h"

Renderer::Renderer(RenderData& rData, VulkanContext& vkContext, GeometryBuilder& geoBuilder, UserInterface& ui, AppState& state) :
	renderData(rData), vulkanContext(vkContext), geometryBuilder(geoBuilder), userInterface(ui), appState(state)
{}

void Renderer::drawFrame() {
    const float viewportWidth = appState.isComparePaths ? static_cast<float>(renderData.swapchain.extent.width) * 0.5f : static_cast<float>(renderData.swapchain.extent.width);
    float aspect = viewportWidth / static_cast<float>(renderData.swapchain.extent.height);

    UniformBufferObject ub{};
    if (appState.draw3D) {
        const float viewportWidth =
            appState.isComparePaths
            ? static_cast<float>(renderData.swapchain.extent.width) * 0.5f
            : static_cast<float>(renderData.swapchain.extent.width);

        const float aspect =
            viewportWidth /
            static_cast<float>(renderData.swapchain.extent.height);

        ub.proj = camera->getProjectionMatrix(aspect);
        ub.view = camera->getViewMatrix();

        ub.cameraPosition = camera->getPosition();
        ub.cameraTarget = camera->getTarget();
    }
    else {
        const float viewportWidth =
            appState.isComparePaths
            ? static_cast<float>(renderData.swapchain.extent.width) * 0.5f
            : static_cast<float>(renderData.swapchain.extent.width);

        const float viewportHeight =
            static_cast<float>(renderData.swapchain.extent.height);

        const float aspect =
            viewportWidth / viewportHeight;

        const float gridWidth =
            static_cast<float>(appState.xSize);

        const float gridHeight =
            static_cast<float>(appState.ySize);

        // Etwas Rand um das Grid
        constexpr float margin = 1.0f;

        float visibleWidth;
        float visibleHeight;

        // Grid vollständig anzeigen und dabei quadratische Zellen behalten.
        if (gridWidth / gridHeight > aspect) {
            // Grid ist relativ breiter als der Viewport.
            visibleWidth = gridWidth + 2.0f * margin;
            visibleHeight = visibleWidth / aspect;
        }
        else {
            // Grid ist relativ höher als der Viewport.
            visibleHeight = gridHeight + 2.0f * margin;
            visibleWidth = visibleHeight * aspect;
        }

        const float centerX =
            (gridWidth - 1.0f) * 0.5f;

        const float centerY =
            (gridHeight - 1.0f) * 0.5f;

        const float left =
            centerX - visibleWidth * 0.5f;

        const float right =
            centerX + visibleWidth * 0.5f;

        const float bottom =
            centerY - visibleHeight * 0.5f;

        const float top =
            centerY + visibleHeight * 0.5f;

        ub.proj = glm::ortho(
            left,
            right,
            top,
            bottom,
            -1.0f,
            1.0f
        );

        ub.view = glm::mat4(1.0f);

        ub.cameraPosition = glm::vec3(0.0f);
        ub.cameraTarget = glm::vec3(0.0f);
    }

    ub.id = appState.selectedCubeId;
    ub.xSize = static_cast<float>(appState.xSize);
    ub.ySize = static_cast<float>(appState.ySize);

	void* data;
	vkMapMemory(renderData.vkInst.device, renderData.uniformBuffer.memory, 0, sizeof(UniformBufferObject), 0, &data);
	memcpy(data, &ub, sizeof(UniformBufferObject)); // Copy values
	vkUnmapMemory(renderData.vkInst.device, renderData.uniformBuffer.memory);

	// Acquire swapchain image (we need to get the image index to use in the command buffer)
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(renderData.vkInst.device, renderData.swapchain.swapchain, UINT64_MAX, vulkanContext.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire swapchain image!");
	}

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		// Swapchain is out of date
		vulkanContext.recreateSwapchain();
		return;
	}

	recordCommandBuffer(imageIndex);

	VkCommandBuffer commandBuffer = renderData.commandBuffers.commandBuffers[imageIndex];

	// Update the command buffer with the image index
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { vulkanContext.imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	// Set the command buffer to execute
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkSemaphore signalSemaphores[] = { vulkanContext.renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// Submit the command buffer for execution
	result = vkQueueSubmit(renderData.vkInst.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	// Present the frame
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &renderData.swapchain.swapchain;
	presentInfo.pImageIndices = &imageIndex;

	// We need to specify that the queue should be synchronized with the semaphores.
	result = vkQueuePresentKHR(renderData.vkInst.graphicsQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		vulkanContext.recreateSwapchain();
		return;
	}

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swapchain image!");
	}

	// Ensure that the frame has been presented before continuing
	vkQueueWaitIdle(renderData.vkInst.graphicsQueue);
}

void Renderer::recordCommandBuffer(uint32_t imageIndex) {
	VkCommandBuffer commandBuffer =
		renderData.commandBuffers.commandBuffers[imageIndex];

	vkResetCommandBuffer(commandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin command buffer!");
	}

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.2f, 0.2f, 0.2f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderData.renderPass.renderPass;
	renderPassBeginInfo.framebuffer = renderData.framebuffers.framebuffers[imageIndex];
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = renderData.swapchain.extent;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	const uint32_t fullWidth = renderData.swapchain.extent.width;
	const uint32_t fullHeight =	renderData.swapchain.extent.height;

	const uint32_t leftWidth = fullWidth / 2;
	const uint32_t rightWidth = fullWidth - leftWidth;

	// ---------- Viewports ---------
	VkViewport viewportLeft{};
	viewportLeft.x = 0.0f;
	viewportLeft.y = 0.0f;
	viewportLeft.width = appState.isComparePaths ? static_cast<float>(leftWidth) : static_cast<float>(fullWidth);
	viewportLeft.height = static_cast<float>(fullHeight);
	viewportLeft.minDepth = 0.0f;
	viewportLeft.maxDepth = 1.0f;

	VkViewport viewportRight{};
	viewportRight.x = static_cast<float>(leftWidth);
	viewportRight.y = 0.0f;
	viewportRight.width = static_cast<float>(rightWidth);
	viewportRight.height = static_cast<float>(fullHeight);
	viewportRight.minDepth = 0.0f;
	viewportRight.maxDepth = 1.0f;

	VkRect2D scissorLeft{};
	scissorLeft.offset = { 0, 0 };
	scissorLeft.extent = { appState.isComparePaths ? leftWidth : fullWidth, fullHeight };

	VkRect2D scissorRight{};
	scissorRight.offset = {
		static_cast<int32_t>(leftWidth),
		0
	};
	scissorRight.extent = { rightWidth, fullHeight };

    auto drawScene3D = [&](
        VkDescriptorSet descriptorSet,
        VkBuffer connectionVertexBuffer,
        VkBuffer connectionIndexBuffer,
        uint32_t connectionIndexCount,
        bool showPath
        ) {
            VkDeviceSize offset = 0;

            // ---------- Cube Fill ----------
            vkCmdBindPipeline(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                renderData.pipeline.cubePipeline
            );

            // Der Descriptor-Set-Inhalt bestimmt hier beispielsweise
            // den Knotenzustand des gewählten Algorithmus.
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                renderData.pipeline.layout,
                0,
                1,
                &descriptorSet,
                0,
                nullptr
            );

            VkBuffer cubeVertexBuffer =
                renderData.vertexBuffer.cubeVBuffer;

            vkCmdBindVertexBuffers(
                commandBuffer,
                0,
                1,
                &cubeVertexBuffer,
                &offset
            );

            vkCmdBindIndexBuffer(
                commandBuffer,
                renderData.vertexBuffer.cubeIBuffer,
                0,
                VK_INDEX_TYPE_UINT32
            );

            vkCmdDrawIndexed(
                commandBuffer,
                static_cast<uint32_t>(
                    geometryBuilder.cubeIndices.size()
                    ),
                1,
                0,
                0,
                0
            );

            // ---------- Cube Edges ----------
            vkCmdBindPipeline(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                renderData.pipeline.cubeEdgePipeline
            );

            /*
             * Falls der Edge-Vertexshader ebenfalls das UBO aus Set 0
             * verwendet, muss der Descriptor auch für diese Pipeline
             * kompatibel gebunden sein. Ein bereits gebundenes Set bleibt
             * zwar grundsätzlich erhalten, aber nur bei kompatiblen
             * Pipeline-Layouts.
             */

            VkBuffer cubeEdgeVertexBuffer =
                renderData.vertexBuffer.cubeEdgeVBuffer;

            vkCmdBindVertexBuffers(
                commandBuffer,
                0,
                1,
                &cubeEdgeVertexBuffer,
                &offset
            );

            vkCmdBindIndexBuffer(
                commandBuffer,
                renderData.vertexBuffer.cubeEdgeIBuffer,
                0,
                VK_INDEX_TYPE_UINT32
            );

            vkCmdDrawIndexed(
                commandBuffer,
                static_cast<uint32_t>(
                    geometryBuilder.cubeEdgeIndices.size()
                    ),
                1,
                0,
                0,
                0
            );

            // ---------- Path ----------
            if (showPath && connectionIndexCount > 0) {
                vkCmdBindPipeline(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderData.pipeline.connectionPipeline
                );

                vkCmdBindVertexBuffers(
                    commandBuffer,
                    0,
                    1,
                    &connectionVertexBuffer,
                    &offset
                );

                vkCmdBindIndexBuffer(
                    commandBuffer,
                    connectionIndexBuffer,
                    0,
                    VK_INDEX_TYPE_UINT32
                );

                vkCmdDrawIndexed(
                    commandBuffer,
                    connectionIndexCount,
                    1,
                    0,
                    0,
                    0
                );
            }

            // ---------- Skybox ----------
            vkCmdBindPipeline(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                renderData.pipeline.skyboxPipeline
            );

            VkBuffer skyboxVertexBuffer =
                renderData.vertexBuffer.skyboxVBuffer;

            vkCmdBindVertexBuffers(
                commandBuffer,
                0,
                1,
                &skyboxVertexBuffer,
                &offset
            );

            vkCmdBindIndexBuffer(
                commandBuffer,
                renderData.vertexBuffer.skyboxIBuffer,
                0,
                VK_INDEX_TYPE_UINT32
            );

            vkCmdDrawIndexed(
                commandBuffer,
                static_cast<uint32_t>(
                    geometryBuilder.skyboxIndices.size()
                    ),
                1,
                0,
                0,
                0
            );
        };

        auto drawScene2D = [&](
            VkDescriptorSet descriptorSet,
            VkBuffer connectionVertexBuffer,
            VkBuffer connectionIndexBuffer,
            uint32_t connectionIndexCount,
            bool showPath
            ) {
                VkDeviceSize offset = 0;

                // ---------- Quad Fill ----------
                vkCmdBindPipeline(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderData.pipeline.quadPipeline
                );

                // Der Descriptor-Set-Inhalt bestimmt hier beispielsweise
                // den Knotenzustand des gewählten Algorithmus.
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderData.pipeline.layout,
                    0,
                    1,
                    &descriptorSet,
                    0,
                    nullptr
                );

                VkBuffer quadVertexBuffer =
                    renderData.vertexBuffer.quadVBuffer;

                vkCmdBindVertexBuffers(
                    commandBuffer,
                    0,
                    1,
                    &quadVertexBuffer,
                    &offset
                );

                vkCmdBindIndexBuffer(
                    commandBuffer,
                    renderData.vertexBuffer.quadIBuffer,
                    0,
                    VK_INDEX_TYPE_UINT32
                );

                vkCmdDrawIndexed(
                    commandBuffer,
                    static_cast<uint32_t>(
                        geometryBuilder.quadIndices.size()
                        ),
                    1,
                    0,
                    0,
                    0
                );

                // ---------- Quad Edges ----------
                vkCmdBindPipeline(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderData.pipeline.quadEdgePipeline
                );

                /*
                 * Falls der Edge-Vertexshader ebenfalls das UBO aus Set 0
                 * verwendet, muss der Descriptor auch für diese Pipeline
                 * kompatibel gebunden sein. Ein bereits gebundenes Set bleibt
                 * zwar grundsätzlich erhalten, aber nur bei kompatiblen
                 * Pipeline-Layouts.
                 */

                VkBuffer quadEdgeVertexBuffer =
                    renderData.vertexBuffer.quadEdgeVBuffer;

                vkCmdBindVertexBuffers(
                    commandBuffer,
                    0,
                    1,
                    &quadEdgeVertexBuffer,
                    &offset
                );

                vkCmdBindIndexBuffer(
                    commandBuffer,
                    renderData.vertexBuffer.quadEdgeIBuffer,
                    0,
                    VK_INDEX_TYPE_UINT32
                );

                vkCmdDrawIndexed(
                    commandBuffer,
                    static_cast<uint32_t>(
                        geometryBuilder.quadEdgeIndices.size()
                        ),
                    1,
                    0,
                    0,
                    0
                );

                // ---------- Path ----------
                if (showPath && connectionIndexCount > 0) {
                    vkCmdBindPipeline(
                        commandBuffer,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        renderData.pipeline.connectionPipeline
                    );

                    vkCmdBindVertexBuffers(
                        commandBuffer,
                        0,
                        1,
                        &connectionVertexBuffer,
                        &offset
                    );

                    vkCmdBindIndexBuffer(
                        commandBuffer,
                        connectionIndexBuffer,
                        0,
                        VK_INDEX_TYPE_UINT32
                    );

                    vkCmdDrawIndexed(
                        commandBuffer,
                        connectionIndexCount,
                        1,
                        0,
                        0,
                        0
                    );
                }
            };


    // ---------------------------------------------------------
    // Linke Ansicht
    // ---------------------------------------------------------

    vkCmdSetViewport(
        commandBuffer,
        0,
        1,
        &viewportLeft
    );

    vkCmdSetScissor(
        commandBuffer,
        0,
        1,
        &scissorLeft
    );

    if (appState.draw3D) {
        drawScene3D(
            renderData.descriptors.descriptorSet,
            renderData.vertexBuffer.connectionVBuffer,
            renderData.vertexBuffer.connectionIBuffer,
            static_cast<uint32_t>(
                geometryBuilder.connectionIndicesLeft.size()
                ),
            appState.showPathLeft
        );
    }
    else {
        drawScene2D(
            renderData.descriptors.descriptorSet,
            renderData.vertexBuffer.connectionVBuffer,
            renderData.vertexBuffer.connectionIBuffer,
            static_cast<uint32_t>(
                geometryBuilder.connectionIndicesLeft.size()
                ),
            appState.showPathLeft
        );
    }

    if (appState.isComparePaths) {
        // ---------------------------------------------------------
        // Rechte Ansicht
        // ---------------------------------------------------------

        vkCmdSetViewport(
            commandBuffer,
            0,
            1,
            &viewportRight
        );

        vkCmdSetScissor(
            commandBuffer,
            0,
            1,
            &scissorRight
        );
    
        if (appState.draw3D) {
            drawScene3D(
                renderData.descriptors.descriptorSetRight,
                renderData.vertexBuffer.connectionVBufferRight,
                renderData.vertexBuffer.connectionIBufferRight,
                static_cast<uint32_t>(
                    geometryBuilder.connectionIndicesRight.size()
                    ),
                appState.showPathRight
            );
        }
        else {
            drawScene2D(
                renderData.descriptors.descriptorSetRight,
                renderData.vertexBuffer.connectionVBufferRight,
                renderData.vertexBuffer.connectionIBufferRight,
                static_cast<uint32_t>(
                    geometryBuilder.connectionIndicesRight.size()
                    ),
                appState.showPathRight
            );
        }

        // ---------------------------------------------------------
        // ImGui wieder über das vollständige Fenster rendern
        // ---------------------------------------------------------

        if (appState.showMenu) {
            VkViewport fullViewport{};
            fullViewport.x = 0.0f;
            fullViewport.y = 0.0f;
            fullViewport.width = static_cast<float>(fullWidth);
            fullViewport.height = static_cast<float>(fullHeight);
            fullViewport.minDepth = 0.0f;
            fullViewport.maxDepth = 1.0f;

            VkRect2D fullScissor{};
            fullScissor.offset = { 0, 0 };
            fullScissor.extent = { fullWidth, fullHeight };

            vkCmdSetViewport(
                commandBuffer,
                0,
                1,
                &fullViewport
            );

            vkCmdSetScissor(
                commandBuffer,
                0,
                1,
                &fullScissor
            );
        }
    }

    userInterface.createFrame(renderData);
    userInterface.render(renderData, imageIndex);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to record command buffer!"
        );
    }
}