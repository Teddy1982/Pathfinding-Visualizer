#include "dijkstra.h"
#include "algoLogic.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <queue>
#include <unordered_set>
#include <vector>
#include <chrono>

Dijkstra::Dijkstra()
    : PathfindingAlgorithm("Dijkstra")
{
    isGCost = true;
}

std::vector<glm::ivec3> Dijkstra::search(
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

    // Alle aktiven Rasterknoten zurücksetzen
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

    auto edgeCost =
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

    struct QueueEntry
    {
        Node* node;
        float gCost;
        std::size_t insertionOrder;
    };

    struct CompareQueueEntry
    {
        bool operator()(
            const QueueEntry& lhs,
            const QueueEntry& rhs) const
        {
            // Das kleinste gCost erhält die höchste Priorität.
            if (lhs.gCost != rhs.gCost)
                return lhs.gCost > rhs.gCost;

            return lhs.insertionOrder > rhs.insertionOrder;
        }
    };

    std::priority_queue<
        QueueEntry,
        std::vector<QueueEntry>,
        CompareQueueEntry
    > openQueue;

    /*
     * Diese Sets dienen der eindeutigen Visualisierung.
     * Die Priority-Queue darf intern veraltete Einträge enthalten.
     */
    std::unordered_set<Node*> openNodes;
    std::unordered_set<Node*> closedNodes;

    std::size_t insertionOrder = 0;
    int currentStep = 0;
    bool targetFound = false;

    nodeStart->gCost = 0.0f;
    nodeStart->hCost = 0.0f;
    nodeStart->fCost = 0.0f;

    openQueue.push({
        nodeStart,
        nodeStart->gCost,
        insertionOrder++
        });

    openNodes.insert(nodeStart);

    while (!openQueue.empty())
    {
        const QueueEntry entry = openQueue.top();
        openQueue.pop();

        Node* nodeCurrent = entry.node;

        // Bereits endgültig verarbeitete Einträge überspringen
        if (nodeCurrent->bVisited)
            continue;

        /*
         * Ein Queue-Eintrag ist veraltet, wenn zwischenzeitlich
         * ein kürzerer Weg zum Knoten gefunden wurde.
         */
        if (entry.gCost > nodeCurrent->gCost + epsilon)
            continue;

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

                if (nodeNeighbour->bVisited)
                    continue;

                // Im 2D-Modus keine 3D-Nachbarn verwenden
                if (!draw3D && nodeNeighbour->z != 0)
                    continue;

                const float newGCost =
                    nodeCurrent->gCost +
                    edgeCost(nodeCurrent, nodeNeighbour);

                if (newGCost + epsilon <
                    nodeNeighbour->gCost)
                {
                    nodeNeighbour->parent = nodeCurrent;
                    nodeNeighbour->gCost = newGCost;

                    // Dijkstra besitzt keine Heuristik.
                    nodeNeighbour->hCost = 0.0f;
                    nodeNeighbour->fCost = newGCost;

                    openQueue.push({
                        nodeNeighbour,
                        newGCost,
                        insertionOrder++
                        });

                    openNodes.insert(nodeNeighbour);
                }
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

    // Kürzesten Pfad rekonstruieren
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