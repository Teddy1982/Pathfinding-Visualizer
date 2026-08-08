#pragma once

#include "pathfindingAlgorithm.h"

class Fringe_Search : public PathfindingAlgorithm {
public:
    Fringe_Search();

    std::vector<glm::ivec3> search(AlgoLogic& logic, int searchDirections, bool draw3D) override;
};
