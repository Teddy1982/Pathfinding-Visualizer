#pragma once

#include "pathfindingAlgorithm.h"

class IDA_Star : public PathfindingAlgorithm {
public:
    IDA_Star();

    std::vector<glm::ivec3> search(AlgoLogic& logic, int searchDirections, bool draw3D) override;
};
