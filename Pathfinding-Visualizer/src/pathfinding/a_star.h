#pragma once

#include "pathfindingAlgorithm.h"

class A_Star : public PathfindingAlgorithm {
public:
    A_Star();

    std::vector<glm::ivec3> search(AlgoLogic& logic, int searchDirections, bool draw3D) override;
};
