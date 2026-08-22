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

// Aktualisiert die Darstellungsdaten, rendert ein Bild
// und übergibt es anschließend an die Swapchain
void Renderer::drawFrame() {
    UniformBufferObject ub{};
    if (appState.draw3D) {
        // Verwendet im 3D-Modus die Projektions- und
        // Ansichtsmatrizen der Kamera.
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
        // Erstellt im 2D-Modus eine orthografische Projektion,
        // die das gesamte Grid im verfügbaren Viewport darstellt
        const float viewportWidth =
            appState.isComparePaths
            ? static_cast<float>(renderData.swapchain.extent.width) * 0.5f
            : static_cast<float>(renderData.swapchain.extent.width);

        const float viewportHeight = static_cast<float>(renderData.swapchain.extent.height);
        const float aspect = viewportWidth / viewportHeight;
        const float gridWidth = static_cast<float>(appState.xSize);
        const float gridHeight = static_cast<float>(appState.ySize);
        constexpr float margin = 1.0f;

        float visibleWidth;
        float visibleHeight;

        // Passt den sichtbaren Bereich an das Seitenverhältnis an,
        // ohne das Grid zu verzerren oder abzuschneiden.
        if (gridWidth / gridHeight > aspect) {
            visibleWidth = gridWidth + 2.0f * margin;
            visibleHeight = visibleWidth / aspect;
        }
        else {
            visibleHeight = gridHeight + 2.0f * margin;
            visibleWidth = visibleHeight * aspect;
        }

        // Zentriert die orthografische Projektion auf dem Grid
        const float centerX = (gridWidth - 1.0f) * 0.5f;
        const float centerY = (gridHeight - 1.0f) * 0.5f;
        const float left = centerX - visibleWidth * 0.5f;
        const float right = centerX + visibleWidth * 0.5f;
        const float bottom = centerY - visibleHeight * 0.5f;
        const float top = centerY + visibleHeight * 0.5f;

        ub.proj = glm::ortho(left, right, top, bottom, -1.0f, 1.0f);
        ub.view = glm::mat4(1.0f);
        ub.cameraPosition = glm::vec3(0.0f);
        ub.cameraTarget = glm::vec3(0.0f);
    }

    // Überträgt weitere Darstellungsparameter an den Shader
    ub.id = appState.selectedCubeId;
    ub.xSize = static_cast<float>(appState.xSize);
    ub.ySize = static_cast<float>(appState.ySize);

    // Aktualisiert den Uniform Buffer mit den Daten des aktuellen Frames
	void* data;
	vkMapMemory(renderData.vkInst.device, renderData.uniformBuffer.memory, 0, sizeof(UniformBufferObject), 0, &data);
	memcpy(data, &ub, sizeof(UniformBufferObject)); // Copy values
	vkUnmapMemory(renderData.vkInst.device, renderData.uniformBuffer.memory);

    // Fordert das nächste verfügbare Bild der Swapchain an
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(renderData.vkInst.device, renderData.swapchain.swapchain, UINT64_MAX, vulkanContext.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire swapchain image!");
	}

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Erstellt die Swapchain bei einer Änderung
        // der Fenstergröße oder Oberfläche neu
		vulkanContext.recreateSwapchain();
		return;
	}

    // Zeichnet die Renderbefehle für das ausgewählte Swapchain-Bild auf
	recordCommandBuffer(imageIndex);

	VkCommandBuffer commandBuffer = renderData.commandBuffers.commandBuffers[imageIndex];

    // Bereitet die Übergabe des Command Buffers
    // an die Grafik-Queue vor
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { vulkanContext.imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    // Wartet vor dem Rendern auf die Verfügbarkeit
    // des angeforderten Swapchain-Bildes
    submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkSemaphore signalSemaphores[] = { vulkanContext.renderFinishedSemaphore };
    // Signalisiert nach der Ausführung, dass das Bild
    // für die Präsentation bereit ist
    submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	result = vkQueueSubmit(renderData.vkInst.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

    // Bereitet die Präsentation des gerenderten Bildes vor
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &renderData.swapchain.swapchain;
	presentInfo.pImageIndices = &imageIndex;

    // Übergibt das fertige Bild an die Swapchain
	result = vkQueuePresentKHR(renderData.vkInst.graphicsQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		vulkanContext.recreateSwapchain();
		return;
	}

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swapchain image!");
	}

    // Wartet, bis alle Befehle der Grafik-Queue abgeschlossen sind
	vkQueueWaitIdle(renderData.vkInst.graphicsQueue);
}

// Zeichnet alle Renderbefehle für das angegebene
// Swapchain-Bild in dessen Command Buffer auf
void Renderer::recordCommandBuffer(uint32_t imageIndex) {
	VkCommandBuffer commandBuffer =
		renderData.commandBuffers.commandBuffers[imageIndex];

    // Setzt den Command Buffer zurück und beginnt
    // eine neue Befehlsaufzeichnung
	vkResetCommandBuffer(commandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin command buffer!");
	}

    // Legt die Ausgangswerte für Farb- und Tiefenbuffer fest
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.2f, 0.2f, 0.2f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

    // Konfiguriert den Render Pass für das aktuelle
    // Swapchain-Framebuffer
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

    // Teilt die Fensterbreite für den Vergleichsmodus
    // in eine linke und rechte Hälfte
	const uint32_t leftWidth = fullWidth / 2;
	const uint32_t rightWidth = fullWidth - leftWidth;

	// ---------- Viewports ---------
    // Definiert Viewports und Zeichenbereiche
    // für die linke und rechte Ansicht
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
	scissorRight.offset = {static_cast<int32_t>(leftWidth),	0};
	scissorRight.extent = { rightWidth, fullHeight };

    // Zeichnet eine vollständige 3D-Ansicht aus Würfelflächen,
    // Würfelkanten, optionalem Pfad und Skybox
    auto drawScene3D = [&](VkDescriptorSet descriptorSet, VkBuffer connectionVertexBuffer, VkBuffer connectionIndexBuffer, uint32_t connectionIndexCount, bool showPath) {
            VkDeviceSize offset = 0;

            // ---------- Cube Fill ----------
            // Zeichnet die gefüllten Grid-Würfel
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.pipeline.cubePipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.pipeline.layout, 0, 1, &descriptorSet, 0, nullptr);

            VkBuffer cubeVertexBuffer = renderData.vertexBuffer.cubeVBuffer;

            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cubeVertexBuffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, renderData.vertexBuffer.cubeIBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(geometryBuilder.cubeIndices.size()), 1, 0, 0, 0);

            // ---------- Cube Edges ----------
            // Zeichnet die Kanten der Grid-Würfel
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.pipeline.cubeEdgePipeline);

            VkBuffer cubeEdgeVertexBuffer = renderData.vertexBuffer.cubeEdgeVBuffer;

            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cubeEdgeVertexBuffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, renderData.vertexBuffer.cubeEdgeIBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(geometryBuilder.cubeEdgeIndices.size()), 1, 0, 0, 0);

            // ---------- Path ----------
            // Zeichnet den gefundenen Pfad, sofern er eingeblendet ist
            if (showPath && connectionIndexCount > 0) {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.pipeline.connectionPipeline);
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &connectionVertexBuffer, &offset);
                vkCmdBindIndexBuffer(commandBuffer, connectionIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, connectionIndexCount, 1, 0, 0, 0);
            }

            // ---------- Skybox ----------
            // Zeichnet die Skybox im Hintergrund
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.pipeline.skyboxPipeline);

            VkBuffer skyboxVertexBuffer = renderData.vertexBuffer.skyboxVBuffer;

            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &skyboxVertexBuffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, renderData.vertexBuffer.skyboxIBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(geometryBuilder.skyboxIndices.size()), 1, 0, 0, 0);
        };

        // Zeichnet eine vollständige 2D-Ansicht aus Quad-Flächen,
        // Quad-Kanten und dem optionalen Pfad
        auto drawScene2D = [&](
            VkDescriptorSet descriptorSet,
            VkBuffer connectionVertexBuffer,
            VkBuffer connectionIndexBuffer,
            uint32_t connectionIndexCount,
            bool showPath
            ) {
                VkDeviceSize offset = 0;

                // ---------- Quad Fill ----------
                // Zeichnet die gefüllten Grid-Flächen
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.pipeline.quadPipeline);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.pipeline.layout, 0, 1, &descriptorSet, 0, nullptr);

                VkBuffer quadVertexBuffer = renderData.vertexBuffer.quadVBuffer;

                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &quadVertexBuffer, &offset);
                vkCmdBindIndexBuffer(commandBuffer, renderData.vertexBuffer.quadIBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(geometryBuilder.quadIndices.size()), 1, 0, 0, 0);

                // ---------- Quad Edges ----------
                // Zeichnet die Kanten der Grid-Flächen
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.pipeline.quadEdgePipeline);

                VkBuffer quadEdgeVertexBuffer = renderData.vertexBuffer.quadEdgeVBuffer;
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &quadEdgeVertexBuffer, &offset);
                vkCmdBindIndexBuffer(commandBuffer, renderData.vertexBuffer.quadEdgeIBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(geometryBuilder.quadEdgeIndices.size()), 1, 0, 0, 0);

                // ---------- Path ----------
                // Zeichnet den gefundenen Pfad, sofern er eingeblendet ist
                if (showPath && connectionIndexCount > 0) {
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.pipeline.connectionPipeline);
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &connectionVertexBuffer, &offset);
                    vkCmdBindIndexBuffer(commandBuffer, connectionIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(commandBuffer, connectionIndexCount, 1, 0, 0, 0);
                }
            };


    // ---------------------------------------------------------
    // Linke Ansicht
    // ---------------------------------------------------------
    // Zeichnet die linke beziehungsweise einzige Ansicht

    vkCmdSetViewport(commandBuffer, 0, 1, &viewportLeft);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissorLeft);

    if (appState.draw3D) {
        drawScene3D(renderData.descriptors.descriptorSet, renderData.vertexBuffer.connectionVBuffer, renderData.vertexBuffer.connectionIBuffer, static_cast<uint32_t>(geometryBuilder.connectionIndicesLeft.size()), appState.showPathLeft);
    }
    else {
        drawScene2D(renderData.descriptors.descriptorSet, renderData.vertexBuffer.connectionVBuffer, renderData.vertexBuffer.connectionIBuffer, static_cast<uint32_t>(geometryBuilder.connectionIndicesLeft.size()), appState.showPathLeft);
    }

    if (appState.isComparePaths) {
        // ---------------------------------------------------------
        // Rechte Ansicht
        // ---------------------------------------------------------
        // Zeichnet im Vergleichsmodus die zweite Ansicht
        // in der rechten Fensterhälfte

        vkCmdSetViewport(commandBuffer, 0, 1, &viewportRight);
        vkCmdSetScissor(commandBuffer, 0,  1, &scissorRight);
    
        if (appState.draw3D) {
            drawScene3D(renderData.descriptors.descriptorSetRight, renderData.vertexBuffer.connectionVBufferRight, renderData.vertexBuffer.connectionIBufferRight, static_cast<uint32_t>(geometryBuilder.connectionIndicesRight.size()), appState.showPathRight);
        }
        else {
            drawScene2D(renderData.descriptors.descriptorSetRight, renderData.vertexBuffer.connectionVBufferRight, renderData.vertexBuffer.connectionIBufferRight, static_cast<uint32_t>(geometryBuilder.connectionIndicesRight.size()), appState.showPathRight);
        }

        // ---------------------------------------------------------
        // ImGui wieder über das vollständige Fenster rendern
        // ---------------------------------------------------------

        if (appState.showMenu) {
            // Stellt für die Benutzeroberfläche wieder den
            // vollständigen Viewport und Zeichenbereich her
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

            vkCmdSetViewport(commandBuffer, 0, 1, &fullViewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &fullScissor);
        }
    }

    // Erstellt und zeichnet die ImGui-Benutzeroberfläche
    userInterface.createFrame(renderData);
    userInterface.render(renderData, imageIndex);

    // Beendet den Render Pass und die Aufzeichnung
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to record command buffer!"
        );
    }
}