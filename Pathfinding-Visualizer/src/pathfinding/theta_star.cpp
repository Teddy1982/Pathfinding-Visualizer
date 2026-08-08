#include <cstddef>
#include <queue>
#include <unordered_set>
#include <vector>
#include <chrono>

#include "theta_star.h"
#include "algoLogic.h"

Theta_Star::Theta_Star() : PathfindingAlgorithm("Theta-Star") {
    isGCost = true;
    isFCost = true;
    isHCost = true;
}

std::vector<glm::ivec3> Theta_Star::search(AlgoLogic& logic, int searchDirections, bool draw3D)
{
    auto start = std::chrono::steady_clock::now();

    std::vector<glm::ivec3> path;

    if (logic.startNodeCoords3D.x < 0 || logic.endNodeCoords3D.x < 0) {
        auto end = std::chrono::steady_clock::now();
        logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
        return path;
    }

    logic.NodesInit(searchDirections, draw3D);
    logic.nodeSteps.clear();

    int gridZ = draw3D ? logic.zSize : 1;
    const float INF = std::numeric_limits<float>::infinity();

    for (int x = 0; x < logic.xSize; x++) {
        for (int y = 0; y < logic.ySize; y++) {
            for (int z = 0; z < gridZ; z++) {
                int idx = logic.GetIndex3D(x, y, z);

                logic.nodes[idx].bVisited = false;
                logic.nodes[idx].step = -1;
                logic.nodes[idx].gCost = INF;
                logic.nodes[idx].hCost = 0.0f;
                logic.nodes[idx].fCost = INF;
                logic.nodes[idx].parent = nullptr;
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

    auto distance =
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

    auto heuristic = [&](Node* a, Node* b) -> float
        {
            return distance(a, b);
        };

    auto lineOfSight = [&](Node* a, Node* b) -> bool
        {
            int x0 = a->x;
            int y0 = a->y;
            int z0 = a->z;

            int x1 = b->x;
            int y1 = b->y;
            int z1 = b->z;

            int dx = std::abs(x1 - x0);
            int dy = std::abs(y1 - y0);
            int dz = std::abs(z1 - z0);

            int xs = (x1 > x0) ? 1 : -1;
            int ys = (y1 > y0) ? 1 : -1;
            int zs = (z1 > z0) ? 1 : -1;

            int x = x0;
            int y = y0;
            int z = z0;

            if (dx >= dy && dx >= dz)
            {
                int p1 = 2 * dy - dx;
                int p2 = 2 * dz - dx;

                while (x != x1)
                {
                    x += xs;

                    if (p1 >= 0) {
                        y += ys;
                        p1 -= 2 * dx;
                    }

                    if (p2 >= 0) {
                        z += zs;
                        p2 -= 2 * dx;
                    }

                    p1 += 2 * dy;
                    p2 += 2 * dz;

                    if (x < 0 || x >= logic.xSize) return false;
                    if (y < 0 || y >= logic.ySize) return false;
                    if (z < 0 || z >= gridZ) return false;

                    if (logic.nodes[logic.GetIndex3D(x, y, z)].bObstacle)
                        return false;
                }
            }
            else if (dy >= dx && dy >= dz)
            {
                int p1 = 2 * dx - dy;
                int p2 = 2 * dz - dy;

                while (y != y1)
                {
                    y += ys;

                    if (p1 >= 0) {
                        x += xs;
                        p1 -= 2 * dy;
                    }

                    if (p2 >= 0) {
                        z += zs;
                        p2 -= 2 * dy;
                    }

                    p1 += 2 * dx;
                    p2 += 2 * dz;

                    if (x < 0 || x >= logic.xSize) return false;
                    if (y < 0 || y >= logic.ySize) return false;
                    if (z < 0 || z >= gridZ) return false;

                    if (logic.nodes[logic.GetIndex3D(x, y, z)].bObstacle)
                        return false;
                }
            }
            else
            {
                int p1 = 2 * dy - dz;
                int p2 = 2 * dx - dz;

                while (z != z1)
                {
                    z += zs;

                    if (p1 >= 0) {
                        y += ys;
                        p1 -= 2 * dz;
                    }

                    if (p2 >= 0) {
                        x += xs;
                        p2 -= 2 * dz;
                    }

                    p1 += 2 * dy;
                    p2 += 2 * dx;

                    if (x < 0 || x >= logic.xSize) return false;
                    if (y < 0 || y >= logic.ySize) return false;
                    if (z < 0 || z >= gridZ) return false;

                    if (logic.nodes[logic.GetIndex3D(x, y, z)].bObstacle)
                        return false;
                }
            }

            return true;
        };

    struct QueueEntry
    {
        Node* node;
        float fCost;
        float hCost;
        std::size_t insertionOrder;
    };

    struct CompareQueueEntry
    {
        bool operator()(
            const QueueEntry& lhs,
            const QueueEntry& rhs) const
        {
            if (lhs.fCost != rhs.fCost)
                return lhs.fCost > rhs.fCost;

            if (lhs.hCost != rhs.hCost)
                return lhs.hCost > rhs.hCost;

            return lhs.insertionOrder > rhs.insertionOrder;
        }
    };

    std::priority_queue<
        QueueEntry,
        std::vector<QueueEntry>,
        CompareQueueEntry
    > openQueue;

    std::unordered_set<Node*> openNodes;
    std::unordered_set<Node*> closedNodes;

    constexpr float epsilon = 0.00001f;

    std::size_t insertionOrder = 0;
    int currentStep = 0;
    bool targetFound = false;

    nodeStart->gCost = 0.0f;
    nodeStart->hCost = heuristic(nodeStart, nodeEnd);
    nodeStart->fCost =
        nodeStart->gCost + nodeStart->hCost;

    /*
     * Beim Startknoten zeigt parent auf den Startknoten selbst.
     * Dadurch funktioniert der typische Theta*-Relaxierungsschritt.
     */
    nodeStart->parent = nodeStart;

    openQueue.push({
        nodeStart,
        nodeStart->fCost,
        nodeStart->hCost,
        insertionOrder++
        });

    openNodes.insert(nodeStart);

    while (!openQueue.empty())
    {
        const QueueEntry entry = openQueue.top();
        openQueue.pop();

        Node* nodeCurrent = entry.node;

        // Bereits geschlossene Duplikate ignorieren
        if (nodeCurrent->bVisited)
            continue;

        // Veraltete Queue-Einträge ignorieren
        if (entry.fCost > nodeCurrent->fCost + epsilon)
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

                if (!draw3D && nodeNeighbour->z != 0)
                    continue;

                Node* candidateParent = nodeCurrent;
                float candidateGCost =
                    nodeCurrent->gCost +
                    distance(nodeCurrent, nodeNeighbour);

                /*
                 * Theta*-Abkürzung:
                 * Kann der Elternknoten von nodeCurrent den
                 * Nachbarn direkt sehen, wird nodeCurrent
                 * im resultierenden Pfad übersprungen.
                 */
                if (nodeCurrent->parent != nullptr &&
                    lineOfSight(
                        nodeCurrent->parent,
                        nodeNeighbour
                    ))
                {
                    const float directGCost =
                        nodeCurrent->parent->gCost +
                        distance(
                            nodeCurrent->parent,
                            nodeNeighbour
                        );

                    if (directGCost < candidateGCost)
                    {
                        candidateParent =
                            nodeCurrent->parent;
                        candidateGCost =
                            directGCost;
                    }
                }

                if (candidateGCost + epsilon <
                    nodeNeighbour->gCost)
                {
                    nodeNeighbour->parent =
                        candidateParent;

                    nodeNeighbour->gCost =
                        candidateGCost;

                    nodeNeighbour->hCost =
                        heuristic(
                            nodeNeighbour,
                            nodeEnd
                        );

                    nodeNeighbour->fCost =
                        nodeNeighbour->gCost +
                        nodeNeighbour->hCost;

                    openQueue.push({
                        nodeNeighbour,
                        nodeNeighbour->fCost,
                        nodeNeighbour->hCost,
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

    Node* node = nodeEnd;

    while (true)
    {
        path.emplace_back(
            node->x,
            node->y,
            node->z
        );

        if (node == nodeStart)
            break;

        if (node->parent == nullptr ||
            node->parent == node)
        {
            path.clear();
            auto end = std::chrono::steady_clock::now();
            logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
            return path;
        }

        node = node->parent;
    }

    std::reverse(path.begin(), path.end());

    auto end = std::chrono::steady_clock::now();
    logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();

    return path;
}