#include "ida_star.h"
#include "algoLogic.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <unordered_set>
#include <vector>
#include <chrono>

IDA_Star::IDA_Star()
    : PathfindingAlgorithm("IDA-Star")
{
    isGCost = true;
    isFCost = true;
    isHCost = true;
    isBound = true;
}

std::vector<glm::ivec3> IDA_Star::search(
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

    // Aktive Knoten zurücksetzen
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
                node->bound = infinity;
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

    auto isReachable = [&]() -> bool
        {
            std::queue<Node*> queue;
            std::unordered_set<Node*> discovered;

            queue.push(nodeStart);
            discovered.insert(nodeStart);

            while (!queue.empty())
            {
                Node* nodeCurrent = queue.front();
                queue.pop();

                if (nodeCurrent == nodeEnd)
                    return true;

                for (Node* nodeNeighbour :
                    nodeCurrent->vecNeighbours)
                {
                    if (nodeNeighbour == nullptr)
                        continue;

                    if (nodeNeighbour->bObstacle)
                        continue;

                    if (!draw3D && nodeNeighbour->z != 0)
                        continue;

                    if (discovered.find(nodeNeighbour) !=
                        discovered.end())
                    {
                        continue;
                    }

                    discovered.insert(nodeNeighbour);
                    queue.push(nodeNeighbour);
                }
            }

            return false;
        };

    if (!isReachable()) {
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
            if (searchDirections == SEARCH_4_DIRECTIONS ||
                searchDirections == SEARCH_6_DIRECTIONS)
            {
                return manhattanDistance(node, nodeEnd);
            }

            return euclideanDistance(node, nodeEnd);
        };

    nodeStart->gCost = 0.0f;
    nodeStart->hCost = heuristic(nodeStart);
    nodeStart->fCost =
        nodeStart->gCost + nodeStart->hCost;

    float bound = nodeStart->fCost;

    int currentStep = 0;
    bool targetFound = false;

    /*
     * Enthält ausschließlich die Knoten des aktuellen
     * Rekursionspfades. Dadurch werden Zyklen verhindert.
     */
    std::unordered_set<Node*> currentPath;

    std::function<float(Node*, float)> depthLimitedSearch;

    depthLimitedSearch =
        [&](Node* nodeCurrent, float currentBound) -> float
        {
            nodeCurrent->hCost = heuristic(nodeCurrent);
            nodeCurrent->fCost =
                nodeCurrent->gCost + nodeCurrent->hCost;
            nodeCurrent->bound = currentBound;

            /*
             * Der Knoten überschreitet das aktuelle Limit.
             * Sein fCost ist ein Kandidat für das nächste Limit.
             */
            if (nodeCurrent->fCost >
                currentBound + epsilon)
            {
                return nodeCurrent->fCost;
            }

            currentPath.insert(nodeCurrent);
            nodeCurrent->bVisited = true;
            nodeCurrent->step = currentStep++;

            NodeStep snapshot;

            snapshot.current = glm::ivec3(
                nodeCurrent->x,
                nodeCurrent->y,
                nodeCurrent->z
            );

            /*
             * IDA* besitzt keine explizite Open-/Closedlist.
             *
             * Zur Visualisierung kann der aktuelle Rekursionspfad
             * als "closed" dargestellt werden. Fachlich handelt es
             * sich dabei jedoch nicht um eine permanente Closedlist.
             */
            for (Node* node : currentPath)
            {
                snapshot.closed.emplace_back(
                    node->x,
                    node->y,
                    node->z
                );
            }

            logic.nodeSteps.push_back(std::move(snapshot));

            if (nodeCurrent == nodeEnd)
            {
                targetFound = true;
                return nodeCurrent->fCost;
            }

            float minimumExceededCost = infinity;

            /*
             * Nachbarn optional nach ihrem erwarteten fCost
             * sortieren. Das verändert die Korrektheit nicht,
             * kann die Suche aber deutlich beschleunigen.
             */
            std::vector<Node*> neighbours;

            for (Node* nodeNeighbour :
                nodeCurrent->vecNeighbours)
            {
                if (nodeNeighbour == nullptr)
                    continue;

                if (nodeNeighbour->bObstacle)
                    continue;

                if (!draw3D && nodeNeighbour->z != 0)
                    continue;

                if (currentPath.find(nodeNeighbour) !=
                    currentPath.end())
                {
                    continue;
                }

                neighbours.push_back(nodeNeighbour);
            }

            std::sort(
                neighbours.begin(),
                neighbours.end(),
                [&](const Node* lhs, const Node* rhs)
                {
                    const float lhsG =
                        nodeCurrent->gCost +
                        euclideanDistance(nodeCurrent, lhs);

                    const float rhsG =
                        nodeCurrent->gCost +
                        euclideanDistance(nodeCurrent, rhs);

                    return lhsG + heuristic(lhs) <
                        rhsG + heuristic(rhs);
                }
            );

            for (Node* nodeNeighbour : neighbours)
            {
                const float oldGCost =
                    nodeNeighbour->gCost;
                const float oldHCost =
                    nodeNeighbour->hCost;
                const float oldFCost =
                    nodeNeighbour->fCost;
                const float oldBound =
                    nodeNeighbour->bound;
                const int oldStep =
                    nodeNeighbour->step;
                Node* oldParent =
                    nodeNeighbour->parent;

                nodeNeighbour->parent = nodeCurrent;
                nodeNeighbour->gCost =
                    nodeCurrent->gCost +
                    euclideanDistance(
                        nodeCurrent,
                        nodeNeighbour
                    );

                nodeNeighbour->hCost =
                    heuristic(nodeNeighbour);

                nodeNeighbour->fCost =
                    nodeNeighbour->gCost +
                    nodeNeighbour->hCost;

                nodeNeighbour->bound = currentBound;

                const float result =
                    depthLimitedSearch(
                        nodeNeighbour,
                        currentBound
                    );

                /*
                 * Bei Erfolg dürfen die parent-Zeiger entlang
                 * des Lösungspfades nicht zurückgesetzt werden.
                 */
                if (targetFound)
                    return result;

                minimumExceededCost =
                    std::min(
                        minimumExceededCost,
                        result
                    );

                // Zustand für den nächsten Suchzweig zurücksetzen
                nodeNeighbour->gCost = oldGCost;
                nodeNeighbour->hCost = oldHCost;
                nodeNeighbour->fCost = oldFCost;
                nodeNeighbour->bound = oldBound;
                nodeNeighbour->step = oldStep;
                nodeNeighbour->parent = oldParent;
                nodeNeighbour->bVisited = false;
            }

            currentPath.erase(nodeCurrent);
            nodeCurrent->bVisited = false;

            return minimumExceededCost;
        };

    while (!targetFound)
    {
        currentPath.clear();

        /*
         * Der Besuchszustand gilt immer nur innerhalb einer
         * einzelnen tiefenbegrenzten Iteration.
         */
        for (Node* node : activeNodes)
        {
            node->bVisited = false;
            node->step = -1;
            node->parent = nullptr;
            node->gCost = infinity;
            node->hCost = 0.0f;
            node->fCost = infinity;
            node->bound = bound;
        }

        nodeStart->gCost = 0.0f;
        nodeStart->hCost = heuristic(nodeStart);
        nodeStart->fCost =
            nodeStart->gCost + nodeStart->hCost;
        nodeStart->bound = bound;
        nodeStart->parent = nullptr;

        const float nextBound =
            depthLimitedSearch(nodeStart, bound);

        if (targetFound)
            break;

        /*
         * Es existiert kein weiterer erreichbarer Knoten,
         * der ein größeres Limit erzeugen könnte.
         */
        if (!std::isfinite(nextBound)) {
            auto end = std::chrono::steady_clock::now();
            logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
            return path;
        }

        /*
         * Sicherheitsprüfung gegen eine durch numerische
         * Probleme stagnierende Grenze.
         */
        if (nextBound <= bound + epsilon) {
            auto end = std::chrono::steady_clock::now();
            logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
            return path;
        }

        bound = nextBound;
    }

    // Lösungspfad rekonstruieren
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