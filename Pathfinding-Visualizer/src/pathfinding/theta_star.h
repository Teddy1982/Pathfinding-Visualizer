#pragma once

#include "pathfindingAlgorithm.h"

class Theta_Star : public PathfindingAlgorithm {
public:
    Theta_Star();

    std::vector<glm::ivec3> search(AlgoLogic& logic, int searchDirections, bool draw3D) override;
};
