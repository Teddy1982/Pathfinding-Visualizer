#pragma once

#include "pathfindingAlgorithm.h"

class Bellman_Ford : public PathfindingAlgorithm {
public:
    Bellman_Ford();

    std::vector<glm::ivec3> search(AlgoLogic& logic, int searchDirections, bool draw3D) override;
};