#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "../vulkan/vulkan_context.h"
#include "../vulkan/buffer_utils.h"
#include "../vulkan/image_utils.h"
#include "../scene/geometry_builder.h"
#include "../tools/user_input_callbacks.h"
#include "../tools/camera.h"
#include "../userInterface/user_interface.h"
#include "../grid/grid_controller.h"
#include "../vulkan/renderer.h"
#include "../vulkan/renderData.h"
#include "../pathfinding/algoLogic.h"
#include "app_state.h"

// Hauptklasse der Anwendung, enthält wichtigsten Klassen
// und Funktion zur Initialisierung, Hauptschleife und Aufräumfunktion
class App {
public:
	App();

	void init();
	void loop();
	void cleanup();

private:
    AppState appState;
    RenderData renderData;

    GeometryBuilder geometryBuilder;
    VulkanContext vulkanContext;
    BufferUtils bufferUtils;
    ImageUtils imageUtils;
    Renderer renderer;

    UserInterface userInterface;
    AlgoLogic algoLogicLeft;
    AlgoLogic algoLogicRight;

    InputHandler inputHandler;
    AppContext appContext;

    GridVisualizer gridVisualizer;
    GridController gridController;

    void cleanupVertexBuffer();
    void initCubeColors();
};