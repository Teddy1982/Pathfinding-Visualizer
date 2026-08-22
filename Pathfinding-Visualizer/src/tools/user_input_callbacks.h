 #pragma once

#include <GLFW/glfw3.h>
#include "camera.h"
#include "controls.h"
#include "imgui.h"
#include "../vulkan/renderData.h"
#include "../application/app_state.h"

// globale Variablen Steuerung, Kamera und Framezeiten
extern Controls gControls;
extern RebindContext rebindContext;

extern Camera* camera;

extern float deltaTime;
extern float lastFrame;


class Camera;
class InputHandler;

//Struktur für glfw-Funktionen
struct AppContext {
	Camera* camera;
	InputHandler* inputHandler;
};

//Klasse für Benutzereingabenverwaltung
class InputHandler {
public:

	InputHandler(AppState& state);

	bool action = false;
	bool play_pause = false;
	bool showMenu = true;

	static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

	void handleKeys(GLFWwindow* window);
	void handleKeyEvents(GLFWwindow* window, int key, int scancode, int action, int mods);
	
private:
	AppState& appState;

	bool lockMouse = false;
	static bool cameraMouseActive;
	static bool firstMouse;
	static double lastMouseX;
	static double lastMouseY;
};
