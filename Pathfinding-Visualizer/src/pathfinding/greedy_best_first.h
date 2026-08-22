#pragma once

#include "pathfindingAlgorithm.h"

// Greedy-Best-First-Algorithmus

class Greedy_Best_First : public PathfindingAlgorithm {
public:
    Greedy_Best_First();

    std::vector<glm::ivec3> search(AlgoLogic& logic, int searchDirections, bool draw3D) override;
};
