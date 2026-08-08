#pragma once

#include "pathfindingAlgorithm.h"

class Dijkstra : public PathfindingAlgorithm {
public:
    Dijkstra();

    std::vector<glm::ivec3> search(AlgoLogic& logic, int searchDirections, bool draw3D) override;
};
