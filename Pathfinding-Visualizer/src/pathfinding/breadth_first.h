#pragma once

#include "pathfindingAlgorithm.h"

// Breadth-First-ALgorithmus

class Breadth_First : public PathfindingAlgorithm {
public:
    Breadth_First();

    std::vector<glm::ivec3> search(AlgoLogic& logic, int searchDirections, bool draw3D) override;
};
