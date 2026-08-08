#include "depth_first.h"
#include "algoLogic.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <chrono>

Depth_First::Depth_First()
    : PathfindingAlgorithm("Depth-First")
{
}

std::vector<glm::ivec3> Depth_First::search(
    AlgoLogic& logic,
    int searchDirections,
    bool draw3D)
{
    auto start = std::chrono::steady_clock::now();

    std::vector<glm::ivec3> path;

    if (logic.startNodeCoords3D.x < 0 ||
        logic.endNodeCoords3D.x < 0)
    {
        auto end = std::chrono::steady_clock::now();
        logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
        return path;
    }

    logic.NodesInit(searchDirections, draw3D);
    logic.nodeSteps.clear();

    const int gridZ = draw3D ? logic.zSize : 1;

    // Alle Knoten zurücksetzen
    for (int x = 0; x < logic.xSize; ++x)
    {
        for (int y = 0; y < logic.ySize; ++y)
        {
            for (int z = 0; z < gridZ; ++z)
            {
                Node& node =
                    logic.nodes[logic.GetIndex3D(x, y, z)];

                node.bVisited = false;
                node.step = -1;
                node.gCost = std::numeric_limits<float>::infinity();
                node.hCost = 0.0f;
                node.fCost = std::numeric_limits<float>::infinity();
                node.parent = nullptr;
            }
        }
    }

    Node* nodeStart = &logic.nodes[logic.GetIndex3D(
        logic.startNodeCoords3D.x,
        logic.startNodeCoords3D.y,
        logic.startNodeCoords3D.z
    )];

    Node* nodeEnd = &logic.nodes[logic.GetIndex3D(
        logic.endNodeCoords3D.x,
        logic.endNodeCoords3D.y,
        logic.endNodeCoords3D.z
    )];

    if (nodeStart->bObstacle || nodeEnd->bObstacle) {
        auto end = std::chrono::steady_clock::now();
        logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
        return path;
    }

    int currentStep = 0;

    /*
     * Rekursive Tiefensuche.
     *
     * Rückgabewert:
     * true  = Ziel gefunden
     * false = Ziel in diesem Zweig nicht gefunden
     */
    std::function<bool(Node*)> depthFirstSearch;

    depthFirstSearch = [&](Node* nodeCurrent) -> bool
        {
            nodeCurrent->bVisited = true;
            nodeCurrent->step = currentStep++;

            NodeStep snapshot;

            snapshot.current = glm::ivec3(
                nodeCurrent->x,
                nodeCurrent->y,
                nodeCurrent->z
            );

            // Alle bereits besuchten Knoten anzeigen
            for (const Node& node : logic.nodes)
            {
                if (node.bVisited)
                {
                    snapshot.closed.emplace_back(
                        node.x,
                        node.y,
                        node.z
                    );
                }
            }

            /*
             * Bei dieser DFS-Darstellung gibt es keine Openlist.
             * Deshalb wird snapshot.open absichtlich nicht gefüllt.
             */
            logic.nodeSteps.push_back(std::move(snapshot));

            if (nodeCurrent == nodeEnd)
                return true;

            for (Node* nodeNeighbour : nodeCurrent->vecNeighbours)
            {
                if (nodeNeighbour == nullptr)
                    continue;

                if (nodeNeighbour->bObstacle)
                    continue;

                if (nodeNeighbour->bVisited)
                    continue;

                nodeNeighbour->parent = nodeCurrent;

                // Nur zusätzliche Information, nicht zur Knotenauswahl.
                nodeNeighbour->gCost = nodeCurrent->gCost + 1.0f;
                nodeNeighbour->hCost = 0.0f;
                nodeNeighbour->fCost = nodeNeighbour->gCost;

                if (depthFirstSearch(nodeNeighbour))
                    return true;
            }

            // Ziel wurde in diesem Zweig nicht gefunden.
            return false;
        };

    nodeStart->gCost = 0.0f;
    nodeStart->hCost = 0.0f;
    nodeStart->fCost = 0.0f;

    const bool targetFound = depthFirstSearch(nodeStart);

    if (!targetFound) {
        auto end = std::chrono::steady_clock::now();
        logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
        return path;
    }

    // Gefundenen DFS-Pfad über parent rekonstruieren
    for (Node* node = nodeEnd;
        node != nullptr;
        node = node->parent)
    {
        path.emplace_back(node->x, node->y, node->z);
    }

    std::reverse(path.begin(), path.end());

    auto end = std::chrono::steady_clock::now();
    logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();

    return path;
}