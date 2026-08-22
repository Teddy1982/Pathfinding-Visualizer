#include "user_input_callbacks.h"

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool InputHandler::cameraMouseActive = false;
bool InputHandler::firstMouse = true;
double InputHandler::lastMouseX = 0.0;
double InputHandler::lastMouseY = 0.0;

InputHandler::InputHandler(AppState& state) : appState(state) {}

// behandelt Mauseingaben für Kamerarotationen
void InputHandler::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (!cameraMouseActive) return;

	if (firstMouse) {
		lastMouseX = xpos;
		lastMouseY = ypos;
		firstMouse = false;
		return;
	}

	float xoffset = static_cast<float>((xpos - lastMouseX) * -1);
	float yoffset = static_cast<float>(lastMouseY - ypos);

	lastMouseX = xpos;
	lastMouseY = ypos;

	float sensitivity = 0.1f;

	AppContext* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
	if (ctx && ctx->camera && ctx->camera) {
		ctx->camera->rotate(xoffset * sensitivity, yoffset * sensitivity);
	}
}

// versteckt Mauszeiger bei Betätigung der rechten Maustaste
void InputHandler::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action == GLFW_PRESS) {
			ImGuiIO& io = ImGui::GetIO();

			if (io.WantCaptureMouse) return;

			cameraMouseActive = true;
			firstMouse = true;

			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}

		if (action == GLFW_RELEASE) {
			cameraMouseActive = false;
			firstMouse = true;

			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
}

// behandelt Tastatureingaben
void InputHandler::handleKeys(GLFWwindow* window) {
	gControls.update(window);
	const InputState& input = gControls.getInputState();

	float cameraSpeed = 5.0f * deltaTime;

	if (input.cameraForward) {
		camera->move(cameraSpeed * glm::vec3(0, 0, 1));
	}
	if (input.cameraBackward) {
		camera->move(cameraSpeed * glm::vec3(0, 0, -1));
	}

	if (input.cameraLeft) {
		camera->move(cameraSpeed * glm::vec3(1, 0, 0));
	}
	if (input.cameraRight) {
		camera->move(cameraSpeed * glm::vec3(-1, 0, 0));
	}

	if (input.cameraUp) {
		camera->move(cameraSpeed * glm::vec3(0, -1, 0));
	}
	if (input.cameraDown) {
		camera->move(cameraSpeed * glm::vec3(0, 1, 0));
	}
	if (input.moveUp) {
		if (appState.selY < appState.ySize - 1) {
			appState.selY++;
		}
	}
	if (input.moveDown) {
		if (appState.selY > 0) {
			appState.selY--;
		}
	}
	if (input.moveLeft) {
		if (appState.selX > 0) {
			appState.selX--;
		}
	}
	if (input.moveRight) {
		if (appState.selX < appState.xSize - 1) {
			appState.selX++;
		}
	}
	if (input.moveForward) {
		if (appState.selZ < appState.zSize - 1) {
			appState.selZ++;
		}
	}
	if (input.moveBackward) {
		if (appState.selZ > 0) {
			appState.selZ--;
		}
	}

	if (input.play_pause) {
		play_pause = true;
	}
	else {
		play_pause = false;
	}

	if (input.action) {
		action = true;
	}
	else {
		action = false;
	}

	if (input.stepNextLeft) {
		appState.stepValueLeft = +1;
		action = true;
	}
	if (input.stepNextRight) {
		appState.stepValueRight = +1;
		action = true;
	}
	if (input.stepPrevLeft) {
		appState.stepValueLeft = -1;
		action = true;
	}
	if (input.stepPrevRight) {
		appState.stepValueRight = -1;
		action = true;
	}

	gControls.clearFrameTriggers();
}

// behandelt Aktivierungszustand der Tasten
void InputHandler::handleKeyEvents(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		showMenu = !showMenu;
	}
	if (rebindContext.awaitingKey && action == GLFW_PRESS) {
		if (rebindContext.targetKey) {
			*rebindContext.targetKey = key;
		}
		rebindContext.awaitingKey = false;
		rebindContext.targetKey = nullptr;
	}
	else {
		gControls.keyCallback(key, action);
	}
}