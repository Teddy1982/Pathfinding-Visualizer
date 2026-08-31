#pragma once

#include <vector>
#include "imgui.h"

#include "../vulkan/renderData.h"
#include "../pathfinding/algoLogic.h"


struct AppState;
struct RenderData;

// Klasse für das Benutzermenü
class UserInterface {
 public:
    UserInterface(AppState& state, AlgoLogic& logicLeft, AlgoLogic& logicRight);
      
    bool init(RenderData& renderData);
    void createFrame(RenderData& renderData);
    void render(RenderData& renderData, int commandBufferIndex);
    void cleanup(RenderData& renderData);

    int settings = SET_START_NODE;
    int distance = MANHATTAN_DISTANCE;
    int searchDirections = SEARCH_6_DIRECTIONS;
    int algorithmusTypeLeft = 0;
    int algorithmusTypeRight = 0;

    int xSize = 10;
    int ySize = 10;
    int zSize = 10;

    float f_costs_left;
    float g_costs_left;
    float h_costs_left;
    float fLimit_left;
    float bound_left;
    
    int maxSteps_left = 0;
    int step_left = 0;

    float f_costs_right;
    float g_costs_right;
    float h_costs_right;
    float fLimit_right;
    float bound_right;

    int maxSteps_right = 0;
    int step_right = 0;

    bool isThinking = false;
    bool isSetting = false;

    const char* settingKeyName;

    ImVec2 thinkingWindowPos;


private:

    AppState& appState;
    AlgoLogic& algoLogicLeft;
    AlgoLogic& algoLogicRight;
};
