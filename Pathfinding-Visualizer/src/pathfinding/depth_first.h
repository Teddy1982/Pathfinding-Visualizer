#pragma once

#include "pathfindingAlgorithm.h"

class Depth_First : public PathfindingAlgorithm {
public:
    Depth_First();

    std::vector<glm::ivec3> search(AlgoLogic& logic, int searchDirections, bool draw3D) override;
};
