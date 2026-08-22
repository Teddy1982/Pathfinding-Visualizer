#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <filesystem>

#include "Logger.h"

#include <GLFW/glfw3.h>
#include <unordered_map>

// Strukturen für Tastensteuerung

struct RebindContext {
    bool awaitingKey = false;
    std::string actionName;
    int* targetKey = nullptr;
};

struct InputState {
    bool cameraUp = false;
    bool cameraDown = false;
    bool cameraLeft = false;
    bool cameraRight = false;
    bool cameraForward = false;
    bool cameraBackward = false;
    bool moveUp = false;
    bool moveDown = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool moveForward = false;
    bool moveBackward = false;
    bool action = false;
    bool stepNextLeft = false;
    bool stepNextRight = false;
    bool stepPrevLeft = false;
    bool stepPrevRight = false;
    bool play_pause = false;
};

struct KeyBindings {
    int cameraUp = GLFW_KEY_KP_9;
    int cameraDown = GLFW_KEY_KP_7;
    int cameraLeft = GLFW_KEY_KP_4;
    int cameraRight = GLFW_KEY_KP_6;
    int cameraForward = GLFW_KEY_KP_8;
    int cameraBackward = GLFW_KEY_KP_5;
    int up = GLFW_KEY_E;
    int down = GLFW_KEY_Q;
    int left = GLFW_KEY_A;
    int right = GLFW_KEY_D;
    int forward = GLFW_KEY_W;
    int backward = GLFW_KEY_S;
    int action = GLFW_KEY_SPACE;
    int stepNextLeft = GLFW_KEY_X;
    int stepNextRight = GLFW_KEY_M;
    int stepPrevLeft = GLFW_KEY_Y;
    int stepPrevRight = GLFW_KEY_N;
    int play_pause = GLFW_KEY_P;
};

//Klasse für Funktionen rundum die Tastensteuerung mit Speichern- und Ladenmöglichkeit
class Controls {
public:
    Controls();

    void keyCallback(int key, int action);
    void clearFrameTriggers();
    const KeyBindings& getKeyBindings() const;
    void setKeyBindings(const KeyBindings& bindings);
    const InputState& getInputState();
    const char* getKeyDisplayName(int key);
    void update(GLFWwindow* window);

    void loadBindings();
    void saveBindings();

private:
    KeyBindings keyMap;
    InputState state;

    std::string filename;
};