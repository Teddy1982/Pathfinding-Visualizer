#include "bellman_ford.h"
#include "algoLogic.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <chrono>
#include <unordered_set>

Bellman_Ford::Bellman_Ford()
    : PathfindingAlgorithm("Bellman-Ford")
{
    isGCost = true;
}

std::vector<glm::ivec3> Bellman_Ford::search(
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
    const float infinity = std::numeric_limits<float>::infinity();
    constexpr float epsilon = 0.00001f;

    std::vector<Node*> activeNodes;

    // Knoten zurücksetzen und aktive Knoten sammeln
    for (int x = 0; x < logic.xSize; ++x)
    {
        for (int y = 0; y < logic.ySize; ++y)
        {
            for (int z = 0; z < gridZ; ++z)
            {
                Node* node =
                    &logic.nodes[logic.GetIndex3D(x, y, z)];

                node->bVisited = false;
                node->step = -1;
                node->gCost = infinity;
                node->hCost = 0.0f;
                node->fCost = infinity;
                node->parent = nullptr;

                if (!node->bObstacle)
                    activeNodes.push_back(node);
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

    auto edgeCost = [](const Node* from, const Node* to) -> float
        {
            const float dx = static_cast<float>(from->x - to->x);
            const float dy = static_cast<float>(from->y - to->y);
            const float dz = static_cast<float>(from->z - to->z);

            return std::sqrt(dx * dx + dy * dy + dz * dz);
        };

    nodeStart->gCost = 0.0f;
    nodeStart->fCost = 0.0f;

    std::unordered_set<Node*> closedNodes;
    int currentStep = 0;

    /*
     * Bei |V| Knoten müssen alle Kanten höchstens
     * |V| - 1 Mal relaxiert werden.
     */
    for (std::size_t iteration = 0;
        iteration + 1 < activeNodes.size();
        ++iteration)
    {
        bool changed = false;

        for (Node* nodeCurrent : activeNodes)
        {
            // Von einem bisher unerreichbaren Knoten kann
            // noch keine sinnvolle Relaxierung ausgehen.
            if (!std::isfinite(nodeCurrent->gCost))
                continue;

            if (!nodeCurrent->bVisited)
            {
                nodeCurrent->bVisited = true;
                nodeCurrent->step = currentStep;
                closedNodes.insert(nodeCurrent);
            }

            for (Node* nodeNeighbour : nodeCurrent->vecNeighbours)
            {
                if (nodeNeighbour == nullptr ||
                    nodeNeighbour->bObstacle)
                {
                    continue;
                }

                if (!draw3D && nodeNeighbour->z != 0)
                    continue;

                const float candidateCost =
                    nodeCurrent->gCost +
                    edgeCost(nodeCurrent, nodeNeighbour);

                if (candidateCost + epsilon < nodeNeighbour->gCost)
                {
                    nodeNeighbour->gCost = candidateCost;
                    nodeNeighbour->fCost = candidateCost;
                    nodeNeighbour->hCost = 0.0f;
                    nodeNeighbour->parent = nodeCurrent;

                    changed = true;
                }
            }

            /*
             * Snapshot für die Visualisierung.
             *
             * Bellman-Ford besitzt keine Open- oder Closed-List.
             * Deshalb bleiben snapshot.open und snapshot.closed leer.
             */
            NodeStep snapshot;

            snapshot.current = glm::ivec3(
                nodeCurrent->x,
                nodeCurrent->y,
                nodeCurrent->z
            );

            for (Node* node : closedNodes)
            {
                snapshot.closed.emplace_back(
                    node->x,
                    node->y,
                    node->z
                );
            }

            logic.nodeSteps.push_back(std::move(snapshot));

            ++currentStep;
        }

        // Wenn keine Kosten verändert wurden, ist das Ergebnis stabil.
        if (!changed)
            break;
    }

    /*
     * Optionale Prüfung auf einen erreichbaren negativen Zyklus.
     * Bei euklidischen Rasterkosten können keine negativen Kanten
     * auftreten. Die Prüfung macht die Implementierung trotzdem
     * zu einem vollständigen Bellman-Ford-Algorithmus.
     */
    bool negativeCycleFound = false;

    for (Node* nodeCurrent : activeNodes)
    {
        if (!std::isfinite(nodeCurrent->gCost))
            continue;

        for (Node* nodeNeighbour : nodeCurrent->vecNeighbours)
        {
            if (nodeNeighbour == nullptr ||
                nodeNeighbour->bObstacle)
            {
                continue;
            }

            if (!draw3D && nodeNeighbour->z != 0)
                continue;

            const float candidateCost =
                nodeCurrent->gCost +
                edgeCost(nodeCurrent, nodeNeighbour);

            if (candidateCost + epsilon < nodeNeighbour->gCost)
            {
                negativeCycleFound = true;
                break;
            }
        }

        if (negativeCycleFound)
            break;
    }

    if (negativeCycleFound)
    {
        // Bei einem negativen Zyklus ist kein wohldefinierter
        // kürzester Pfad vorhanden.
        auto end = std::chrono::steady_clock::now();
        logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
        return path;
    }

    // Das Ziel wurde nicht erreicht.
    if (!std::isfinite(nodeEnd->gCost)) {
        auto end = std::chrono::steady_clock::now();
        logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
        return path;
    }

    // Kürzesten Pfad rekonstruieren
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