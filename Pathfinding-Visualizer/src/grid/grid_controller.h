#pragma once

#include "grid_visualizer.h"
#include "../scene/geometry_builder.h"

class GridVisualizer;
class AlgoLogic;
class InputHandler;
class UserInterface;
struct AppState;
class Renderer;

class GridController {
public:
	GridController(GridVisualizer& visualizer, UserInterface& ui, AlgoLogic& logicLeft, AlgoLogic& logicRight, InputHandler& handler, Renderer& rndr, AppState& state);
	
	void doAction();

private:
	
	GridVisualizer& gridVisualizer;
	AlgoLogic& algoLogicLeft;
	AlgoLogic& algoLogicRight;
	InputHandler& inputHandler;
	UserInterface& userInterface;
	AppState& appState;
	Renderer& renderer;

	void resetSearchState();
	void clearNodeColor(const std::array<int, 3>& coords);
	void initializeAnimationLeft();
	void initializeAnimationRight();
};
