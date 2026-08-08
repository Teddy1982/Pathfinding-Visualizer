#pragma once

#include <vector>
#include "glm/glm.hpp"

#include "../globals/global_constants.h"
#include "../scene/cube_state.h"

struct AppState {
public:
	glm::vec3 selectedCubeId = glm::vec3(0.0f, 0.0f, 0.0f);
	int selX = 0;
	int selY = 0;
	int selZ = 0;
	std::vector<glm::ivec3> pathLeft;
	std::vector<glm::ivec3> pathRight;
	int stepValueLeft = 0;
	int stepValueRight = 0;
	int visualStepLeft = 0;
	int visualStepRight = 0;
	int animationSpeed = 1;
	bool stepSearchInitializedLeft = false;
	bool stepSearchInitializedRight = false;
	bool showPathLeft = false;
	bool showPathRight = false;
	bool isComparePaths = false;
	bool isPlaying = false;
	int lastSearchDirections = SEARCH_6_DIRECTIONS;
	bool draw3D = true;
	bool showMenu = true;
	std::vector<CubeState> cubeColorsLeft;
	std::vector<CubeState> cubeColorsRight;
	bool initArray = false;
	int xSize = 10;
	int ySize = 10;
	int zSize = 10;
};
