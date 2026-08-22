#pragma once

#include "pathfindingAlgorithm.h"

// Floyd-Warshall-Algorithmus

class Floyd_Warshall : public PathfindingAlgorithm {
public:
    Floyd_Warshall();

    std::vector<glm::ivec3> search(AlgoLogic& logic, int searchDirections, bool draw3D) override;
};
