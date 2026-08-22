#include "grid_controller.h"
#include "grid_visualizer.h"
#include "../userInterface/user_interface.h"
#include "../tools/user_input_callbacks.h"
#include "../vulkan/renderer.h"
#include "../pathfinding/algoLogic.h"
#include "../application/app_state.h"
#include "../globals/global_constants.h"

GridController::GridController(GridVisualizer& visualizer, UserInterface& ui, AlgoLogic& logicLeft, AlgoLogic& logicRight, InputHandler& handler, Renderer& rndr, AppState& state) : 
	gridVisualizer(visualizer), userInterface(ui), algoLogicLeft(logicLeft),algoLogicRight(logicRight), inputHandler(handler), renderer(rndr), appState(state)
{}

void GridController::doAction() {
	// Setzen des Startknotens
	if (userInterface.settings == SET_START_NODE) {
		resetSearchState();
		appState.stepSearchInitializedLeft = false;
		appState.stepSearchInitializedRight = false;
		std::array<int, 3> lastNodeCoords = { algoLogicLeft.startNodeCoords3D[0],algoLogicLeft.startNodeCoords3D[1],algoLogicLeft.startNodeCoords3D[2] };
		clearNodeColor(lastNodeCoords);
		algoLogicLeft.setStartNode(appState.selX, appState.selY, appState.selZ);
		algoLogicRight.setStartNode(appState.selX, appState.selY, appState.selZ);
		std::array<int, 3> newNodeCoords = { algoLogicLeft.startNodeCoords3D[0],algoLogicLeft.startNodeCoords3D[1],algoLogicLeft.startNodeCoords3D[2] };
		gridVisualizer.updateCubeColorsLeft(newNodeCoords, FULL_SCALE, BLUE);
		gridVisualizer.updateCubeColorsRight(newNodeCoords, FULL_SCALE, BLUE);
	}
	// Setzen des Zielknotens
	else if (userInterface.settings == SET_END_NODE) {
		resetSearchState();
		appState.stepSearchInitializedLeft = false;
		appState.stepSearchInitializedRight = false;
		std::array<int, 3> lastNodeCoords = { algoLogicLeft.endNodeCoords3D[0],algoLogicLeft.endNodeCoords3D[1], algoLogicLeft.endNodeCoords3D[2] };
		clearNodeColor(lastNodeCoords);
		algoLogicLeft.setEndNode(appState.selX, appState.selY, appState.selZ);
		algoLogicRight.setEndNode(appState.selX, appState.selY, appState.selZ);
		std::array<int, 3> newNodeCoords = { algoLogicLeft.endNodeCoords3D[0],algoLogicLeft.endNodeCoords3D[1],algoLogicLeft.endNodeCoords3D[2] };
		gridVisualizer.updateCubeColorsLeft(newNodeCoords, FULL_SCALE, PINK);
		gridVisualizer.updateCubeColorsRight(newNodeCoords, FULL_SCALE, PINK);
	}
	//Setzen des Hindernisknotens
	else if (userInterface.settings == SET_OBSTACLE_NODE) {
		resetSearchState();
		appState.stepSearchInitializedLeft = false;
		appState.stepSearchInitializedRight = false;
		algoLogicLeft.setObstacleNode(appState.selX, appState.selY, appState.selZ);
		algoLogicRight.setObstacleNode(appState.selX, appState.selY, appState.selZ);
		gridVisualizer.updateCubeColorsLeft({ appState.selX, appState.selY, appState.selZ }, FULL_SCALE, BLACK);
		gridVisualizer.updateCubeColorsRight({ appState.selX, appState.selY, appState.selZ }, FULL_SCALE, BLACK);
	}
	// Löschen eines einzelnen Knotens
	else if (userInterface.settings == SET_ERASE_NODE) {
		resetSearchState();
		appState.stepSearchInitializedLeft = false;
		appState.stepSearchInitializedRight = false;
		algoLogicLeft.eraseNodeValue(appState.selX, appState.selY, appState.selZ);
		algoLogicRight.eraseNodeValue(appState.selX, appState.selY, appState.selZ);
		gridVisualizer.updateCubeColorsLeft({ appState.selX, appState.selY, appState.selZ }, ZERO_SCALE, CLEAR);
		gridVisualizer.updateCubeColorsRight({ appState.selX, appState.selY, appState.selZ }, ZERO_SCALE, CLEAR);
	}
	// Löschen aller Knoten
	else if (userInterface.settings == SET_ERASE_ALL_NODES) {
		resetSearchState();
		appState.stepSearchInitializedLeft = false;
		appState.stepSearchInitializedRight = false;
		algoLogicLeft.initArray(userInterface.xSize, userInterface.ySize, userInterface.zSize);
		algoLogicRight.initArray(userInterface.xSize, userInterface.ySize, userInterface.zSize);
		algoLogicLeft.nodes.clear();
		algoLogicRight.nodes.clear();
		algoLogicLeft.nodeSteps.clear();
		algoLogicRight.nodeSteps.clear();
		int gridZ = algoLogicLeft.zSize;

		if (!appState.draw3D) {
			gridZ = 1;
		}

		for (int x = 0; x < algoLogicLeft.xSize; x++) {
			for (int y = 0; y < algoLogicLeft.ySize; y++) {
				for (int z = 0; z < gridZ; z++) {
					gridVisualizer.updateCubeColorsLeft({ x, y, z }, ZERO_SCALE, CLEAR);
					gridVisualizer.updateCubeColorsRight({ x, y, z }, ZERO_SCALE, CLEAR);
				}
			}
		}
		appState.pathLeft.clear();
		appState.pathRight.clear();
		appState.showPathLeft = false;
		appState.showPathRight = false;
	}
	// Löschen aller Knoten, die bei der Suche entstehen, wie offene, bearbeitete, PfadKnoten und Verbindungslinien
	else if (userInterface.settings == SET_ERASE_ALL_PATH_NODES) {
		resetSearchState();
		appState.stepSearchInitializedLeft = false;
		appState.stepSearchInitializedRight = false;
		algoLogicLeft.nodes.clear();
		algoLogicRight.nodes.clear();
		algoLogicLeft.nodeSteps.clear();
		algoLogicRight.nodeSteps.clear();
		int gridZ = algoLogicLeft.zSize;

		if (!appState.draw3D) {
			gridZ = 1;
		}

		for (int x = 0; x < algoLogicLeft.xSize; x++) {
			for (int y = 0; y < algoLogicLeft.ySize; y++) {
				for (int z = 0; z < gridZ; z++) {
					int index = algoLogicLeft.GetIndex3D(x, y, z);
					if (algoLogicLeft.array3D[index] == OBSTACLE_NODE) {
						continue;
					}
					gridVisualizer.updateCubeColorsLeft({ x, y, z }, ZERO_SCALE, CLEAR);
					gridVisualizer.updateCubeColorsRight({ x, y, z }, ZERO_SCALE, CLEAR);
				}
			}
		}
		if (algoLogicLeft.startNodeCoords3D.x != -1) {
			gridVisualizer.updateCubeColorsLeft({ algoLogicLeft.startNodeCoords3D.x, algoLogicLeft.startNodeCoords3D.y, algoLogicLeft.startNodeCoords3D.z }, FULL_SCALE, BLUE);
			gridVisualizer.updateCubeColorsRight({ algoLogicLeft.startNodeCoords3D.x, algoLogicLeft.startNodeCoords3D.y, algoLogicLeft.startNodeCoords3D.z }, FULL_SCALE, BLUE);
		}
		if (algoLogicLeft.endNodeCoords3D.x != -1) {
			gridVisualizer.updateCubeColorsLeft({ algoLogicLeft.endNodeCoords3D.x, algoLogicLeft.endNodeCoords3D.y, algoLogicLeft.endNodeCoords3D.z }, FULL_SCALE, PINK);
			gridVisualizer.updateCubeColorsRight({ algoLogicLeft.endNodeCoords3D.x, algoLogicLeft.endNodeCoords3D.y, algoLogicLeft.endNodeCoords3D.z }, FULL_SCALE, PINK);
		}

		appState.pathLeft.clear();
		appState.pathRight.clear();
		appState.showPathLeft = false;
		appState.showPathRight = false;
	}
	// direkte Pfadsuche
	else if (userInterface.settings == SET_SEARCH_PATH) {
		if (algoLogicLeft.startNodeCoords3D.x == -1 || algoLogicLeft.endNodeCoords3D.x == -1 || appState.stepValueLeft != 0 || appState.stepValueRight != 0) {
			appState.stepValueLeft = 0;
			appState.stepValueRight = 0;
			return;
		}
		userInterface.isThinking = true;
		renderer.drawFrame();

		int index = userInterface.algorithmusTypeLeft;
		if (index >= 0 && index < algoLogicLeft.algorithms.size()) {
			appState.pathLeft = algoLogicLeft.algorithms[index]->search(algoLogicLeft, userInterface.searchDirections, appState.draw3D);
		}

		for (int i = 0; i < algoLogicLeft.nodeSteps.size(); i++) {
			for (int j = 0; j < algoLogicLeft.nodeSteps[i].closed.size(); j++) {
				int x = algoLogicLeft.nodeSteps[i].closed[j].x;
				int y = algoLogicLeft.nodeSteps[i].closed[j].y;
				int z = algoLogicLeft.nodeSteps[i].closed[j].z;
				gridVisualizer.updateCubeColorsLeft({ x, y, z }, HALF_SCALE, RED);
			}
			for (int k = 0; k < algoLogicLeft.nodeSteps[i].open.size(); k++) {
				int x = algoLogicLeft.nodeSteps[i].open[k].x;
				int y = algoLogicLeft.nodeSteps[i].open[k].y;
				int z = algoLogicLeft.nodeSteps[i].open[k].z;
				gridVisualizer.updateCubeColorsLeft({ x, y, z }, HALF_SCALE, GREEN);
			}
		}

		if (appState.isComparePaths == true) {
			index = userInterface.algorithmusTypeRight;
			if (index >= 0 && index < algoLogicRight.algorithms.size()) {
				appState.pathRight = algoLogicRight.algorithms[index]->search(algoLogicRight, userInterface.searchDirections, appState.draw3D);
			}

			for (int i = 0; i < algoLogicRight.nodeSteps.size(); i++) {
				for (int j = 0; j < algoLogicRight.nodeSteps[i].closed.size(); j++) {
					int x = algoLogicRight.nodeSteps[i].closed[j].x;
					int y = algoLogicRight.nodeSteps[i].closed[j].y;
					int z = algoLogicRight.nodeSteps[i].closed[j].z;
					gridVisualizer.updateCubeColorsRight({ x, y, z }, HALF_SCALE, RED);
				}
				for (int k = 0; k < algoLogicRight.nodeSteps[i].open.size(); k++) {
					int x = algoLogicRight.nodeSteps[i].open[k].x;
					int y = algoLogicRight.nodeSteps[i].open[k].y;
					int z = algoLogicRight.nodeSteps[i].open[k].z;
					gridVisualizer.updateCubeColorsRight({ x, y, z }, HALF_SCALE, GREEN);
				}
			}
		}

		gridVisualizer.drawPathLeft();

		gridVisualizer.geometryBuilder.rebuildPathGeometry(appState.pathLeft, true);
		gridVisualizer.updateConnectionBuffer(true);

		if (!appState.pathLeft.empty()) {
			appState.showPathLeft = true;
			userInterface.isThinking = false;
		}

		if (appState.isComparePaths == true) {
			gridVisualizer.drawPathRight();
			gridVisualizer.geometryBuilder.rebuildPathGeometry(appState.pathRight, false);
			gridVisualizer.updateConnectionBuffer(false);

			if (!appState.pathRight.empty()) {
				appState.showPathRight = true;
				userInterface.isThinking = false;
			}
		}
	}
	// Pfadsuche im Einzelschrittmodus oder Wiedergabe in einer Animation
	else if (userInterface.settings == SET_SEARCH_PATH_STEP || userInterface.settings == SET_SEARCH_PATH_PLAY) {
		if (algoLogicLeft.startNodeCoords3D.x == -1 || algoLogicLeft.endNodeCoords3D.x == -1) {
			return;
		}
		userInterface.isThinking = true;
		renderer.drawFrame();

		if (!appState.stepSearchInitializedLeft) {
			initializeAnimationLeft();
		}

		gridVisualizer.updateVisualizationLeft(appState.stepValueLeft);

		if (appState.isComparePaths) {
			if (!appState.stepSearchInitializedRight) {
				initializeAnimationRight();
			}

			gridVisualizer.updateVisualizationRight(appState.stepValueRight);
		}
	}

	userInterface.isThinking = false;

	gridVisualizer.updateStorageBufferLeft();
	gridVisualizer.updateStorageBufferRight();
}

// Setzt Sucheinstellungen auf Startzustand
void GridController::resetSearchState() {
	appState.visualStepLeft = -1;
	appState.visualStepRight = -1;
	appState.pathLeft.clear();
	appState.pathRight.clear();
	appState.showPathLeft = false;
	appState.showPathRight = false;
	appState.isPlaying = false;

	algoLogicLeft.runtime = 0;
	algoLogicRight.runtime = 0;
}

// Löscht Knoten und seinen Zustand in der linken und rechten Algorithmusansicht
void GridController::clearNodeColor(const std::array<int, 3>& coords) {
	if (coords[0] != -1) {
		gridVisualizer.updateCubeColorsLeft(coords, ZERO_SCALE, CLEAR);
		gridVisualizer.updateCubeColorsRight(coords, ZERO_SCALE, CLEAR);
	}
}

// Initialisert Pfadeinstellungen für schrittweise Suche und Animationswiedergabe für die linke Algorithmusansicht
void GridController::initializeAnimationLeft() {
	appState.visualStepLeft = -1;
	appState.pathLeft.clear();
	appState.showPathLeft = false;
	algoLogicLeft.nodeSteps.clear();

	int index = userInterface.algorithmusTypeLeft;
	if (index >= 0 && index < algoLogicLeft.algorithms.size()) {
		appState.pathLeft = algoLogicLeft.algorithms[index]->search(algoLogicLeft, userInterface.searchDirections, appState.draw3D);
	}
	appState.stepSearchInitializedLeft = true;

}

// Initialisert Pfadeinstellungen für schrittweise Suche und Animationswiedergabe für die rechte Algorithmusansicht
void GridController::initializeAnimationRight() {
	appState.visualStepRight = -1;
	appState.pathRight.clear();
	appState.showPathRight = false;
	algoLogicRight.nodeSteps.clear();

	int index = userInterface.algorithmusTypeRight;
	if (index >= 0 && index < algoLogicRight.algorithms.size()) {
		appState.pathRight = algoLogicRight.algorithms[index]->search(algoLogicRight, userInterface.searchDirections, appState.draw3D);
	}
	appState.stepSearchInitializedRight = true;
}