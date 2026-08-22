#include "application.h"
#include "app_state.h"
#include "../tools/logger.h"

App::App() : 
	renderData(),
	vulkanContext(renderData),
	geometryBuilder(),
	bufferUtils(renderData,appState),
	imageUtils(renderData),
	algoLogicLeft(),
	algoLogicRight(),
	userInterface(appState, algoLogicLeft, algoLogicRight),
	inputHandler(appState),
	appContext(),
	renderer(renderData, vulkanContext, geometryBuilder, userInterface, appState),
	gridVisualizer(renderData, algoLogicLeft, algoLogicRight, geometryBuilder, appState),
	gridController(gridVisualizer, userInterface, algoLogicLeft, algoLogicRight, inputHandler, renderer, appState)
{
}

// Initialisieren aller Würfel und Quadrate im 3D- und 2D-Grid mit einem Standardwert
void App::initCubeColors() {
	int gridZ = appState.zSize;
	
	if (!appState.draw3D) {
		gridZ = 1;
	}
	
	for (int x = 0; x < appState.xSize; x++) {
		for (int y = 0; y < appState.ySize; y++) {
			geometryBuilder.createUnitQuad(x, y);
			geometryBuilder.createQuadEdges((float)x, (float)y);
			for (int z = 0; z < gridZ; z++) {
				geometryBuilder.createUnitCube(x, y, z);
				geometryBuilder.createCubeEdges((float)x, (float)y, (float)z);
				
				CubeState empty;
				empty.color[0] = 0.0f;
				empty.color[1] = 0.0f;
				empty.color[2] = 0.0f;
				empty.color[3] = 0.0f;

				empty.scale[0] = 0.0f;
				empty.scale[1] = 0.0f;
				empty.scale[2] = 0.0f;
				empty.scale[3] = 1.0f;

				appState.cubeColorsLeft.push_back(empty);
				appState.cubeColorsRight.push_back(empty);
			}
		}
	}
}

// Löschen von Vertex-, Index- und Memorybuffer rund um das 3D- bzw. 2D-Grid
void App::cleanupVertexBuffer() {
	vkDestroyBuffer(renderData.vkInst.device, renderData.vertexBuffer.cubeVBuffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.vertexBuffer.cubeVMemory, nullptr);
	vkDestroyBuffer(renderData.vkInst.device, renderData.vertexBuffer.cubeIBuffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.vertexBuffer.cubeIMemory, nullptr);
	vkDestroyBuffer(renderData.vkInst.device, renderData.vertexBuffer.cubeEdgeVBuffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.vertexBuffer.cubeEdgeVMemory, nullptr);
	vkDestroyBuffer(renderData.vkInst.device, renderData.vertexBuffer.cubeEdgeIBuffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.vertexBuffer.cubeEdgeIMemory, nullptr);

	vkDestroyBuffer(renderData.vkInst.device, renderData.vertexBuffer.quadVBuffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.vertexBuffer.quadVMemory, nullptr);
	vkDestroyBuffer(renderData.vkInst.device, renderData.vertexBuffer.quadIBuffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.vertexBuffer.quadIMemory, nullptr);
	vkDestroyBuffer(renderData.vkInst.device, renderData.vertexBuffer.quadEdgeVBuffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.vertexBuffer.quadEdgeVMemory, nullptr);
	vkDestroyBuffer(renderData.vkInst.device, renderData.vertexBuffer.quadEdgeIBuffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.vertexBuffer.quadEdgeIMemory, nullptr);

	vkDestroyBuffer(renderData.vkInst.device, renderData.vertexBuffer.connectionVBuffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.vertexBuffer.connectionVMemory, nullptr);
	vkDestroyBuffer(renderData.vkInst.device, renderData.vertexBuffer.connectionIBuffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.vertexBuffer.connectionIMemory, nullptr);
	vkDestroyBuffer(renderData.vkInst.device, renderData.vertexBuffer.connectionVBufferRight, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.vertexBuffer.connectionVMemoryRight, nullptr);
	vkDestroyBuffer(renderData.vkInst.device, renderData.vertexBuffer.connectionIBufferRight, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.vertexBuffer.connectionIMemoryRight, nullptr);

	renderData.vertexBuffer.cubeVBuffer = VK_NULL_HANDLE;
	renderData.vertexBuffer.cubeVMemory = VK_NULL_HANDLE;
	renderData.vertexBuffer.cubeIBuffer = VK_NULL_HANDLE;
	renderData.vertexBuffer.cubeIMemory = VK_NULL_HANDLE;
	renderData.vertexBuffer.cubeEdgeVBuffer = VK_NULL_HANDLE;
	renderData.vertexBuffer.cubeEdgeVMemory = VK_NULL_HANDLE;
	renderData.vertexBuffer.cubeEdgeIBuffer = VK_NULL_HANDLE;
	renderData.vertexBuffer.cubeEdgeIMemory = VK_NULL_HANDLE;

	renderData.vertexBuffer.quadVBuffer = VK_NULL_HANDLE;
	renderData.vertexBuffer.quadVMemory = VK_NULL_HANDLE;
	renderData.vertexBuffer.quadIBuffer = VK_NULL_HANDLE;
	renderData.vertexBuffer.quadIMemory = VK_NULL_HANDLE;
	renderData.vertexBuffer.quadEdgeVBuffer = VK_NULL_HANDLE;
	renderData.vertexBuffer.quadEdgeVMemory = VK_NULL_HANDLE;
	renderData.vertexBuffer.quadEdgeIBuffer = VK_NULL_HANDLE;
	renderData.vertexBuffer.quadEdgeIMemory = VK_NULL_HANDLE;

	renderData.vertexBuffer.connectionVBuffer = VK_NULL_HANDLE;
	renderData.vertexBuffer.connectionVMemory = VK_NULL_HANDLE;
	renderData.vertexBuffer.connectionIBuffer = VK_NULL_HANDLE;
	renderData.vertexBuffer.connectionIMemory = VK_NULL_HANDLE;
	renderData.vertexBuffer.connectionVBufferRight = VK_NULL_HANDLE;
	renderData.vertexBuffer.connectionVMemoryRight = VK_NULL_HANDLE;
	renderData.vertexBuffer.connectionIBufferRight = VK_NULL_HANDLE;
	renderData.vertexBuffer.connectionIMemoryRight = VK_NULL_HANDLE;
}

// Aufräumfunktion beim Beenden der Anwendung
// Alle Vulkan-, Speicher-, Kamera- sowie GLFW-Ressourcen werden freigegeben
void App::cleanup() {
	vkDestroyPipeline(renderData.vkInst.device, renderData.pipeline.cubePipeline, nullptr);
	vkDestroyPipeline(renderData.vkInst.device, renderData.pipeline.cubeEdgePipeline, nullptr);
	vkDestroyPipeline(renderData.vkInst.device, renderData.pipeline.connectionPipeline, nullptr);

	vkDestroyPipeline(renderData.vkInst.device, renderData.pipeline.quadPipeline, nullptr);
	vkDestroyPipeline(renderData.vkInst.device, renderData.pipeline.quadEdgePipeline, nullptr);

	vkDestroyPipelineLayout(renderData.vkInst.device, renderData.pipeline.layout, nullptr);
	vkDestroyRenderPass(renderData.vkInst.device, renderData.renderPass.renderPass, nullptr);
	vkDestroySwapchainKHR(renderData.vkInst.device, renderData.swapchain.swapchain, nullptr);

	for (auto framebuffer : renderData.framebuffers.framebuffers) {
		vkDestroyFramebuffer(renderData.vkInst.device, framebuffer, nullptr);
	}

	cleanupVertexBuffer();

	vkDestroyBuffer(renderData.vkInst.device, renderData.uniformBuffer.buffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.uniformBuffer.memory, nullptr);
	vkDestroyBuffer(renderData.vkInst.device, renderData.storageBuffer.buffer, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.storageBuffer.memory, nullptr);

	vkDestroyDescriptorSetLayout(renderData.vkInst.device, renderData.descriptors.layout, nullptr);
	vkDestroyDescriptorPool(renderData.vkInst.device, renderData.descriptors.pool, nullptr);

	vkDestroyImageView(renderData.vkInst.device, renderData.framebuffers.depthImageView, nullptr);
	vkDestroyImage(renderData.vkInst.device, renderData.framebuffers.depthImage, nullptr);
	vkFreeMemory(renderData.vkInst.device, renderData.framebuffers.depthImageMemory, nullptr);

	if (camera != nullptr) {
		delete camera;
		camera = nullptr;
	}

	glfwDestroyWindow(renderData.window);
	glfwTerminate();
}

// Start- und Initialisierungsfunktion 
void App::init() {

	//Initialisierung und Starten von GLFW-Komponenten

	if (!glfwInit()) {
		std::runtime_error("Failed to initialize GLFW!");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	renderData.window = glfwCreateWindow(mode->width, mode->height, "Pfadfindungsalgorithmus-Visualisierung", nullptr, nullptr);

	appContext.camera = camera;
	appContext.inputHandler = &inputHandler;

	glfwSetWindowUserPointer(renderData.window, &appContext);
	glfwSetKeyCallback(renderData.window, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
		auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(win));
		if (!ctx) return;

		ctx->inputHandler->handleKeyEvents(win, key, scancode, action, mods);
		});
	glfwSetCursorPosCallback(renderData.window,
		[](GLFWwindow* win, double xpos, double ypos)
		{
			auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(win));
			if (!ctx) return;

			ctx->inputHandler->mouse_callback(win, xpos, ypos);
		});
	glfwSetMouseButtonCallback(renderData.window,
		[](GLFWwindow* win, int button, int action, int mods)
		{
			auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(win));
			if (!ctx) return;

			ctx->inputHandler->mouse_button_callback(win, button, action, mods);
		});
	if (glfwRawMouseMotionSupported()) {
		glfwSetInputMode(renderData.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

	// Erstellung und Initialisierung von Vulkankomponenten
	vulkanContext.createVulkanInstance();
	vulkanContext.createCommandPool();
	vulkanContext.createSwapchain();
	vulkanContext.createRenderPass();
	
	bufferUtils.createUniformBuffer();
	imageUtils.createSkyboxTexture();

	initCubeColors();

	bufferUtils.createLeftStorageBuffer();
	bufferUtils.createRightStorageBuffer();

	geometryBuilder.createSkybox();

	// Erstellung von Vertexbuffern
	bufferUtils.createVertexBuffer(renderData.vertexBuffer.cubeVBuffer, renderData.vertexBuffer.cubeIBuffer, renderData.vertexBuffer.cubeVMemory, renderData.vertexBuffer.cubeIMemory, (void*)geometryBuilder.cubeVertices.data(), sizeof(CubeVertex) * geometryBuilder.cubeVertices.size(), (void*)geometryBuilder.cubeIndices.data(), sizeof(uint32_t) * geometryBuilder.cubeIndices.size(), static_cast<uint32_t>(geometryBuilder.cubeIndices.size()));
	bufferUtils.createVertexBuffer(renderData.vertexBuffer.cubeEdgeVBuffer, renderData.vertexBuffer.cubeEdgeIBuffer, renderData.vertexBuffer.cubeEdgeVMemory, renderData.vertexBuffer.cubeEdgeIMemory, (void*)geometryBuilder.cubeEdgeVertices.data(), sizeof(CubeEdgeVertex) * geometryBuilder.cubeEdgeVertices.size(), (void*)geometryBuilder.cubeEdgeIndices.data(), sizeof(uint32_t) * geometryBuilder.cubeEdgeIndices.size(), static_cast<uint32_t>(geometryBuilder.cubeEdgeIndices.size()));

	bufferUtils.createVertexBuffer(renderData.vertexBuffer.quadVBuffer, renderData.vertexBuffer.quadIBuffer, renderData.vertexBuffer.quadVMemory, renderData.vertexBuffer.quadIMemory, (void*)geometryBuilder.quadVertices.data(), sizeof(QuadVertex) * geometryBuilder.quadVertices.size(), (void*)geometryBuilder.quadIndices.data(), sizeof(uint32_t) * geometryBuilder.quadIndices.size(), static_cast<uint32_t>(geometryBuilder.quadIndices.size()));
	bufferUtils.createVertexBuffer(renderData.vertexBuffer.quadEdgeVBuffer, renderData.vertexBuffer.quadEdgeIBuffer, renderData.vertexBuffer.quadEdgeVMemory, renderData.vertexBuffer.quadEdgeIMemory, (void*)geometryBuilder.quadEdgeVertices.data(), sizeof(QuadEdgeVertex) * geometryBuilder.quadEdgeVertices.size(), (void*)geometryBuilder.quadEdgeIndices.data(), sizeof(uint32_t) * geometryBuilder.quadEdgeIndices.size(), static_cast<uint32_t>(geometryBuilder.quadEdgeIndices.size()));

	bufferUtils.createVertexBuffer(renderData.vertexBuffer.skyboxVBuffer, renderData.vertexBuffer.skyboxIBuffer, renderData.vertexBuffer.skyboxVMemory, renderData.vertexBuffer.skyboxIMemory, (void*)geometryBuilder.skyboxVertices.data(), sizeof(glm::vec4) * geometryBuilder.skyboxVertices.size(), (void*)geometryBuilder.skyboxIndices.data(), sizeof(uint32_t) * geometryBuilder.skyboxIndices.size(), static_cast<uint32_t>(geometryBuilder.skyboxIndices.size()));

	bufferUtils.createConnectionBuffers();

	vulkanContext.createVulkanDescriptors();
	vulkanContext.createGraphicsPipelines();
	vulkanContext.createFramebuffers();

	userInterface.init(renderData);

	vulkanContext.createCommandBuffers();
	vulkanContext.createSemaphores();

	float lastTime = glfwGetTime();

	camera->move(glm::vec3(-5.0f, 0.0f, -10.0f));
	algoLogicLeft.initArray(userInterface.xSize, userInterface.ySize, userInterface.zSize);
	algoLogicRight.initArray(userInterface.xSize, userInterface.ySize, userInterface.zSize);
}

// Hauptschleife der Anwendung
void App::loop() {
	while (!glfwWindowShouldClose(renderData.window)) {
		glfwPollEvents();
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		userInterface.thinkingWindowPos = ImVec2(renderData.swapchain.extent.width / 2 - 100, renderData.swapchain.extent.height / 2 - 50);

		inputHandler.handleKeys(renderData.window);

		if (!appState.draw3D) {
			appState.selZ = 0;
		}

		// falls Einstellung der Suchrichtungen sich ändert oder der Suchraum initialisiert werden soll
		// Leeren und Neuinitialisierung des 2D- und 3D-Grids
		if (appState.lastSearchDirections != userInterface.searchDirections || appState.initArray == true) {
			int gridZ = appState.zSize;

			algoLogicLeft.initArray(appState.xSize, appState.ySize, appState.zSize);
			algoLogicRight.initArray(appState.xSize, appState.ySize, appState.zSize);
			algoLogicLeft.nodeSteps.clear();
			algoLogicRight.nodeSteps.clear();
			appState.pathLeft.clear();
			appState.pathRight.clear();
			appState.cubeColorsLeft.clear();
			appState.cubeColorsRight.clear();
			algoLogicLeft.nodes.clear();
			algoLogicRight.nodes.clear();
			algoLogicLeft.nodeSteps.clear();
			algoLogicRight.nodeSteps.clear();
			appState.stepSearchInitializedLeft = false;
			appState.stepSearchInitializedRight = false;
			appState.visualStepLeft = 0;
			appState.visualStepRight = 0;
			appState.initArray = false;

			geometryBuilder.cubeVertices.clear();
			geometryBuilder.cubeIndices.clear();
			geometryBuilder.cubeEdgeVertices.clear();
			geometryBuilder.cubeEdgeIndices.clear();

			geometryBuilder.quadVertices.clear();
			geometryBuilder.quadIndices.clear();
			geometryBuilder.quadEdgeVertices.clear();
			geometryBuilder.quadEdgeIndices.clear();

			geometryBuilder.connectionVerticesLeft.clear();
			geometryBuilder.connectionVerticesRight.clear();
			geometryBuilder.connectionIndicesLeft.clear();
			geometryBuilder.connectionIndicesRight.clear();

			vkDeviceWaitIdle(renderData.vkInst.device);

			cleanupVertexBuffer();

			if (userInterface.searchDirections == SEARCH_4_DIRECTIONS || userInterface.searchDirections == SEARCH_8_DIRECTIONS) {
				appState.draw3D = false;
			}
			else {
				appState.draw3D = true;
			}

			initCubeColors();

			bufferUtils.createVertexBuffer(renderData.vertexBuffer.cubeVBuffer, renderData.vertexBuffer.cubeIBuffer, renderData.vertexBuffer.cubeVMemory, renderData.vertexBuffer.cubeIMemory, geometryBuilder.cubeVertices.data(), sizeof(CubeVertex) * geometryBuilder.cubeVertices.size(), geometryBuilder.cubeIndices.data(), sizeof(uint32_t) * geometryBuilder.cubeIndices.size(), static_cast<uint32_t>(geometryBuilder.cubeIndices.size()));
			bufferUtils.createVertexBuffer(renderData.vertexBuffer.cubeEdgeVBuffer, renderData.vertexBuffer.cubeEdgeIBuffer, renderData.vertexBuffer.cubeEdgeVMemory, renderData.vertexBuffer.cubeEdgeIMemory, geometryBuilder.cubeEdgeVertices.data(), sizeof(CubeEdgeVertex) * geometryBuilder.cubeEdgeVertices.size(), geometryBuilder.cubeEdgeIndices.data(), sizeof(uint32_t) * geometryBuilder.cubeEdgeIndices.size(), static_cast<uint32_t>(geometryBuilder.cubeEdgeIndices.size()));
			bufferUtils.createVertexBuffer(renderData.vertexBuffer.quadVBuffer, renderData.vertexBuffer.quadIBuffer, renderData.vertexBuffer.quadVMemory, renderData.vertexBuffer.quadIMemory, geometryBuilder.quadVertices.data(), sizeof(QuadVertex) * geometryBuilder.quadVertices.size(), geometryBuilder.quadIndices.data(), sizeof(uint32_t) * geometryBuilder.quadIndices.size(), static_cast<uint32_t>(geometryBuilder.quadIndices.size()));
			bufferUtils.createVertexBuffer(renderData.vertexBuffer.quadEdgeVBuffer, renderData.vertexBuffer.quadEdgeIBuffer, renderData.vertexBuffer.quadEdgeVMemory, renderData.vertexBuffer.quadEdgeIMemory, geometryBuilder.quadEdgeVertices.data(), sizeof(QuadEdgeVertex) * geometryBuilder.quadEdgeVertices.size(), geometryBuilder.quadEdgeIndices.data(), sizeof(uint32_t) * geometryBuilder.quadEdgeIndices.size(), static_cast<uint32_t>(geometryBuilder.quadEdgeIndices.size()));

			bufferUtils.createConnectionBuffers();

			vkFreeCommandBuffers(renderData.vkInst.device, renderData.commandBuffers.commandPool, static_cast<uint32_t>(renderData.commandBuffers.commandBuffers.size()), renderData.commandBuffers.commandBuffers.data());

			renderData.commandBuffers.commandBuffers.clear();

			vulkanContext.createCommandBuffers();

			gridVisualizer.updateStorageBufferLeft();
			gridVisualizer.updateStorageBufferRight();

			appState.lastSearchDirections = userInterface.searchDirections;
		}

		// Falls keine offenen oder bearbeiteten Knoten existieren sollen die Bewertungsinformationen auf den Standardwert gesetzt werden
		appState.selectedCubeId = glm::vec3((float)appState.selX, (float)appState.selY, (float)appState.selZ);
		if (algoLogicLeft.nodes.size() == 0 && algoLogicRight.nodes.size() == 0) {
			userInterface.f_costs_left = 0.0f;
			userInterface.g_costs_left = 0.0f;
			userInterface.h_costs_left = 0.0f;
			userInterface.fLimit_left = 0.0f;
			userInterface.bound_left = 0.0f;
			userInterface.step_left = 0;
			userInterface.f_costs_right = 0.0f;
			userInterface.g_costs_right = 0.0f;
			userInterface.h_costs_right = 0.0f;
			userInterface.fLimit_right = 0.0f;
			userInterface.bound_right = 0.0f;
			userInterface.step_right = 0;
		}
		
		// Anzeigen der Bwertungsinformationen des ausgewählten Knotens für die linke und rechte Algorithmusansicht
		int idx = gridVisualizer.toIndex(appState.selectedCubeId.x, appState.selectedCubeId.y, appState.selectedCubeId.z);
		if (idx < algoLogicLeft.nodes.size()) {
			userInterface.f_costs_left = algoLogicLeft.nodes[idx].fCost;
			userInterface.g_costs_left = algoLogicLeft.nodes[idx].gCost;
			userInterface.h_costs_left = algoLogicLeft.nodes[idx].hCost;
			userInterface.fLimit_left = algoLogicLeft.nodes[idx].fLimit;
			userInterface.bound_left = algoLogicLeft.nodes[idx].bound;
			userInterface.step_left = algoLogicLeft.nodes[idx].step + 1;
		}
		if (idx < algoLogicRight.nodes.size()) {
			userInterface.f_costs_right = algoLogicRight.nodes[idx].fCost;
			userInterface.g_costs_right = algoLogicRight.nodes[idx].gCost;
			userInterface.h_costs_right = algoLogicRight.nodes[idx].hCost;
			userInterface.fLimit_right = algoLogicRight.nodes[idx].fLimit;
			userInterface.bound_right = algoLogicRight.nodes[idx].bound;
			userInterface.step_right = algoLogicRight.nodes[idx].step + 1;
		}

		// Falls Aktion-Taste gedrückt wurde, rufe entsprechende Funktion im Controller auf
		if (inputHandler.action) {
			gridController.doAction();
		}

		// Animationseinstellungen falls Wiedergabe der Suche aktiviert wurde
		if (userInterface.settings == SET_SEARCH_PATH_PLAY)
		{
			static float elapsedTime = 0.0f;
			if (inputHandler.play_pause)
			{
				appState.isPlaying = !appState.isPlaying;
				elapsedTime = 0.0f;
			}

			if (appState.isPlaying)
			{
				elapsedTime += deltaTime;
				const float animationInterval = 5.0f / static_cast<float>(appState.animationSpeed);

				if (elapsedTime >= animationInterval)
				{
					appState.stepValueLeft++;
					appState.stepValueRight++;
					elapsedTime -= animationInterval;

					gridController.doAction();
				}
			}
		}

		renderer.drawFrame();
	}
}