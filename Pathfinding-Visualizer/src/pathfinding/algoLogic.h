#pragma once

#include <array>
#include <vector>
#include <list>
#include <cmath>
#include <limits>
#include <functional>
#include <algorithm>
#include <queue>
#include <stack>
#include <string>
#include <memory>

#include "glm/glm.hpp"
#include "../globals/global_constants.h"

#include "pathfindingAlgorithm.h"
#include "a_star.h"
#include "dijkstra.h"
#include "breadth_first.h"
#include "depth_first.h"
#include "bellman_ford.h"
#include "floyd_warshall.h"
#include "greedy_best_first.h"
#include "fringe_search.h"

//Struktur für Visualisierung für schrittweise Suche
struct NodeStep
{
    glm::ivec3 current;
    std::vector<glm::ivec3> open;
    std::vector<glm::ivec3> closed;
};

// Knotenstruktur
struct Node
{
    bool bObstacle = false;
    bool bVisited = false;
    int step = -1;

    float gCost = std::numeric_limits<float>::infinity();
    float hCost = 0.0f;
    float fCost = std::numeric_limits<float>::infinity();
    float fLimit = 0.0f;
    float bound = 0.0f;

    int x = 0;
    int y = 0;
    int z = 0;

    std::vector<Node*> vecNeighbours;
    Node* parent = nullptr;
};

// Basisklasse für Pfadfindungsalgorithmen
class AlgoLogic {
public:
    int heuristicMode = MANHATTAN_DISTANCE;

    std::vector<int> array3D;

    int xSize;
    int ySize;
    int zSize;

    double runtime = 0.0f;

    glm::ivec3 startNodeCoords3D = { -1, -1, -1 };
    glm::ivec3 endNodeCoords3D = { -1, -1, -1 };

    std::vector<NodeStep> nodeSteps;
    std::vector<Node> nodes;

    std::vector<std::unique_ptr<PathfindingAlgorithm>> algorithms;

    AlgoLogic()
    {
        algorithms.push_back(std::make_unique<A_Star>());
        algorithms.push_back(std::make_unique<Bellman_Ford>());
        algorithms.push_back(std::make_unique<Dijkstra>());
        algorithms.push_back(std::make_unique<Breadth_First>());
        algorithms.push_back(std::make_unique<Depth_First>());
        algorithms.push_back(std::make_unique<Floyd_Warshall>());
        algorithms.push_back(std::make_unique<Fringe_Search>());
        algorithms.push_back(std::make_unique<Greedy_Best_First>());
    }

    int GetIndex3D(int x, int y, int z);
    void initArray(int x, int y, int z);

    void setStartNode(int x, int y, int z);
    void setEndNode(int x, int y, int z);
    void setObstacleNode(int x, int y, int z);
    void eraseNodeValue(int x, int y, int z);

    void NodesInit(int searchDirections, bool draw3D);

};
