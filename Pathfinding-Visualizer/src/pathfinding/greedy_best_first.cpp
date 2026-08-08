#include "greedy_best_first.h"
#include "algoLogic.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <queue>
#include <unordered_set>
#include <vector>
#include <chrono>

Greedy_Best_First::Greedy_Best_First()
    : PathfindingAlgorithm("Greedy-Best-First")
{
    isHCost = true;
}

std::vector<glm::ivec3> Greedy_Best_First::search(
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

    logic.heuristicMode =
        (searchDirections == SEARCH_4_DIRECTIONS ||
            searchDirections == SEARCH_6_DIRECTIONS)
        ? MANHATTAN_DISTANCE
        : EUCLID_DISTANCE;

    const int gridZ = draw3D ? logic.zSize : 1;
    const float infinity = std::numeric_limits<float>::infinity();

    // Knoten zurücksetzen
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
                node.gCost = infinity;
                node.hCost = 0.0f;
                node.fCost = infinity;
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

    auto euclideanDistance =
        [](const Node* first, const Node* second) -> float
        {
            const float dx =
                static_cast<float>(first->x - second->x);
            const float dy =
                static_cast<float>(first->y - second->y);
            const float dz =
                static_cast<float>(first->z - second->z);

            return std::sqrt(dx * dx + dy * dy + dz * dz);
        };

    auto manhattanDistance =
        [](const Node* first, const Node* second) -> float
        {
            return static_cast<float>(
                std::abs(first->x - second->x) +
                std::abs(first->y - second->y) +
                std::abs(first->z - second->z)
                );
        };

    auto heuristic = [&](const Node* node) -> float
        {
            if (logic.heuristicMode == MANHATTAN_DISTANCE)
                return manhattanDistance(node, nodeEnd);

            return euclideanDistance(node, nodeEnd);
        };

    struct QueueEntry
    {
        Node* node;
        float hCost;
        std::size_t insertionOrder;
    };

    struct CompareQueueEntry
    {
        bool operator()(
            const QueueEntry& lhs,
            const QueueEntry& rhs) const
        {
            // Kleinerer hCost erhält höhere Priorität.
            if (lhs.hCost != rhs.hCost)
                return lhs.hCost > rhs.hCost;

            // Stabile Reihenfolge bei gleichem hCost.
            return lhs.insertionOrder > rhs.insertionOrder;
        }
    };

    std::priority_queue<
        QueueEntry,
        std::vector<QueueEntry>,
        CompareQueueEntry
    > openQueue;

    /*
     * discoveredNodes verhindert doppelte Einträge.
     * closedNodes wird für die Visualisierung verwendet.
     */
    std::unordered_set<Node*> discoveredNodes;
    std::unordered_set<Node*> openNodes;
    std::unordered_set<Node*> closedNodes;

    std::size_t insertionOrder = 0;
    int currentStep = 0;
    bool targetFound = false;

    nodeStart->gCost = 0.0f;
    nodeStart->hCost = heuristic(nodeStart);
    nodeStart->fCost = nodeStart->hCost;

    openQueue.push({
        nodeStart,
        nodeStart->hCost,
        insertionOrder++
        });

    discoveredNodes.insert(nodeStart);
    openNodes.insert(nodeStart);

    while (!openQueue.empty())
    {
        const QueueEntry entry = openQueue.top();
        openQueue.pop();

        Node* nodeCurrent = entry.node;

        openNodes.erase(nodeCurrent);

        nodeCurrent->bVisited = true;
        nodeCurrent->step = currentStep++;
        closedNodes.insert(nodeCurrent);

        if (nodeCurrent == nodeEnd)
        {
            targetFound = true;
        }
        else
        {
            for (Node* nodeNeighbour :
                nodeCurrent->vecNeighbours)
            {
                if (nodeNeighbour == nullptr)
                    continue;

                if (nodeNeighbour->bObstacle)
                    continue;

                if (!draw3D && nodeNeighbour->z != 0)
                    continue;

                /*
                 * Der Knoten wurde bereits entdeckt oder
                 * vollständig verarbeitet.
                 */
                if (discoveredNodes.contains(nodeNeighbour))
                    continue;

                /*
                 * Bereits beim Einfügen markieren. Dadurch
                 * kann der Knoten nicht mehrfach in die
                 * Openlist gelangen.
                 */
                discoveredNodes.insert(nodeNeighbour);
                openNodes.insert(nodeNeighbour);

                nodeNeighbour->parent = nodeCurrent;
                nodeNeighbour->hCost =
                    heuristic(nodeNeighbour);

                /*
                 * gCost wird nur zur Information berechnet.
                 * Es beeinflusst die Auswahl nicht.
                 */
                nodeNeighbour->gCost =
                    nodeCurrent->gCost +
                    euclideanDistance(
                        nodeCurrent,
                        nodeNeighbour
                    );

                nodeNeighbour->fCost =
                    nodeNeighbour->hCost;

                openQueue.push({
                    nodeNeighbour,
                    nodeNeighbour->hCost,
                    insertionOrder++
                    });
            }
        }

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

        for (Node* node : openNodes)
        {
            snapshot.open.emplace_back(
                node->x,
                node->y,
                node->z
            );
        }

        logic.nodeSteps.push_back(std::move(snapshot));

        if (targetFound)
            break;
    }

    if (!targetFound) {
        auto end = std::chrono::steady_clock::now();
        logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
        return path;
    }

    // Den zuerst gefundenen Pfad rekonstruieren
    for (Node* node = nodeEnd;
        node != nullptr;
        node = node->parent)
    {
        path.emplace_back(
            node->x,
            node->y,
            node->z
        );
    }

    std::reverse(path.begin(), path.end());

    auto end = std::chrono::steady_clock::now();
    logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();

    return path;
}