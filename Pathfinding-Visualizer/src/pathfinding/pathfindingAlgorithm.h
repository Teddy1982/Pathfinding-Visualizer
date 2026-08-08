#pragma once

#include <string>
#include <vector>
#include "glm/glm.hpp"

class AlgoLogic;

class PathfindingAlgorithm {
public:
    explicit PathfindingAlgorithm(const std::string& t)
        : title(t)
    {
    }

    virtual ~PathfindingAlgorithm() = default;
    std::string title;
    virtual std::vector<glm::ivec3> search(
        AlgoLogic& logic,
        int searchDirections,
        bool draw3D
    ) = 0;

    bool isFCost = false;
    bool isGCost = false;
    bool isHCost = false;
    bool isFLimit = false;
    bool isBound = false;
};