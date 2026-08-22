#include "Controls.h"

Controls::Controls() {
    // Erstellung einer Steuerungkonfigurationsdatei, falls noch nicht vorhanden
    filename = "data/controls.txt";
    if (!std::filesystem::exists(filename)) {
        if (!std::filesystem::exists("data")) {
            std::filesystem::create_directory("data");
        }
        std::ofstream ofs(filename);
        if (!ofs) {
            Logger::log(2, "error while creating '%s'\n", filename.c_str());
        }

        saveBindings();
    }
}

// gibt die Tastenbelegung zurück
const KeyBindings& Controls::getKeyBindings() const {
    return keyMap;
}

// überschreibt die Tastenbelegung
void Controls::setKeyBindings(const KeyBindings& bindings) {
    keyMap = bindings;
}

// Einstellung ob bestimmte Tasten aufgrund ihres Aktivierungszustands als gedrückt oder al 1x betätigt behandelt werden sollen
void Controls::keyCallback(int key, int action) {
    if (action == GLFW_REPEAT) {
        return;
    }

    bool pressed = (action == GLFW_PRESS);

    if (key == keyMap.cameraUp)                         state.cameraUp = pressed;
    else if (key == keyMap.cameraDown)                  state.cameraDown = pressed;
    else if (key == keyMap.cameraLeft)                  state.cameraLeft = pressed;
    else if (key == keyMap.cameraRight)                 state.cameraRight = pressed;
    else if (key == keyMap.cameraForward)               state.cameraForward = pressed;
    else if (key == keyMap.cameraBackward)              state.cameraBackward = pressed;

    else if (pressed && key == keyMap.up)               state.moveUp = true;
    else if (pressed && key == keyMap.down)             state.moveDown = true;
    else if (pressed && key == keyMap.left)             state.moveLeft = true;
    else if (pressed && key == keyMap.right)            state.moveRight = true;
    else if (pressed && key == keyMap.forward)          state.moveForward = true;
    else if (pressed && key == keyMap.backward)         state.moveBackward = true;
    else if (pressed && key == keyMap.action)           state.action = true;
    else if (pressed && key == keyMap.stepNextLeft)     state.stepNextLeft = true;
    else if (pressed && key == keyMap.stepNextRight)    state.stepNextRight = true;
    else if (pressed && key == keyMap.stepPrevLeft)     state.stepPrevLeft = true;
    else if (pressed && key == keyMap.stepPrevRight)    state.stepPrevRight = true;
    else if (pressed && key == keyMap.play_pause)       state.play_pause = true;
}

// Zurücksetzen der Tastenaktivierungen
void Controls::clearFrameTriggers() {
    state.moveUp = false;
    state.moveDown = false;
    state.moveLeft = false;
    state.moveRight = false;
    state.moveForward = false;
    state.moveBackward = false;
    state.action = false;
    state.stepNextLeft = false;
    state.stepNextRight = false;
    state.stepPrevLeft = false;
    state.stepPrevRight = false;
    state.play_pause = false;
}

// gibt Aktivierungszustände der Tasten zurück (aktiviert oder nicht)
const InputState& Controls::getInputState() {
    return state;
}

// gibt Bezeichnung einer Taste anhand ihres Key-Codes zurück
const char* Controls::getKeyDisplayName(int key) {
    const char* name = glfwGetKeyName(key, 0);
    if (name) return name;

    switch (key) {
    case GLFW_KEY_UP: return "<PFEIL_OBEN>";
    case GLFW_KEY_DOWN: return "<PFEIL_UNTEN>";
    case GLFW_KEY_LEFT: return "<PFEIL_LINKS>";
    case GLFW_KEY_RIGHT: return "<PFEIL_RECHTS>";
    case GLFW_KEY_SPACE: return "<LEERTASTE>";
    case GLFW_KEY_ENTER: return "<EINGABETASTE>";
    case GLFW_KEY_ESCAPE: return "<ESC>";
    default: return "???";
    }
}

// setzt Kamerasteuerungstaste auf aktiviert, falls sie gedrückt wird
void Controls::update(GLFWwindow* window) {
    state.cameraUp = glfwGetKey(window, keyMap.cameraUp) == GLFW_PRESS;
    state.cameraDown = glfwGetKey(window, keyMap.cameraDown) == GLFW_PRESS;
    state.cameraLeft = glfwGetKey(window, keyMap.cameraLeft) == GLFW_PRESS;
    state.cameraRight = glfwGetKey(window, keyMap.cameraRight) == GLFW_PRESS;
    state.cameraForward = glfwGetKey(window, keyMap.cameraForward) == GLFW_PRESS;
    state.cameraBackward = glfwGetKey(window, keyMap.cameraBackward) == GLFW_PRESS;
}

// lädt Tastenbelegung aus Steuerungkonfigurationsdatei
void Controls::loadBindings() {
    std::vector<int> entries;
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        int entry;
        entry = std::stoi(line);
        entries.push_back(entry);
    }

    KeyBindings bindings;
    bindings.cameraUp = entries[0];
    bindings.cameraDown = entries[1];
    bindings.cameraLeft = entries[2];
    bindings.cameraRight = entries[3];
    bindings.cameraForward = entries[4];
    bindings.cameraBackward = entries[5];
    bindings.up = entries[6];
    bindings.down = entries[7];
    bindings.left = entries[8];
    bindings.right = entries[9];
    bindings.forward = entries[10];
    bindings.backward = entries[11];
    bindings.action = entries[12];
    bindings.stepNextLeft = entries[13];
    bindings.stepNextRight = entries[14];
    bindings.stepPrevLeft = entries[15];
    bindings.stepPrevRight = entries[16];
    bindings.play_pause = entries[17];

    setKeyBindings(bindings);
}

// speichert Tastenbelegung in Steuerungkonfigurationsdatei
void Controls::saveBindings() {
    std::ofstream file(filename, std::ios::trunc);
    file << keyMap.cameraUp << "\n";
    file << keyMap.cameraDown << "\n";
    file << keyMap.cameraLeft << "\n";
    file << keyMap.cameraRight << "\n";
    file << keyMap.cameraForward << "\n";
    file << keyMap.cameraBackward << "\n";
    file << keyMap.up << "\n";
    file << keyMap.down << "\n";
    file << keyMap.left << "\n";
    file << keyMap.right << "\n";
    file << keyMap.forward << "\n";
    file << keyMap.backward << "\n";
    file << keyMap.action << "\n";
    file << keyMap.stepNextLeft << "\n";
    file << keyMap.stepNextRight << "\n";
    file << keyMap.stepPrevLeft << "\n";
    file << keyMap.stepPrevRight << "\n";
    file << keyMap.play_pause << "\n";

    file.close();
}
