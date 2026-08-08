#include "breadth_first.h"
#include "algoLogic.h"

#include <algorithm>
#include <limits>
#include <queue>
#include <unordered_set>
#include <chrono>

Breadth_First::Breadth_First()
    : PathfindingAlgorithm("Breadth-First")
{
}

std::vector<glm::ivec3> Breadth_First::search(
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

    // FIFO-Queue = Openlist des Breadth-First Search
    std::queue<Node*> openQueue;

    // Separat gespeichert, damit bVisited "entdeckt" bedeuten kann.
    std::unordered_set<Node*> closedNodes;

    nodeStart->bVisited = true;
    nodeStart->gCost = 0.0f;
    nodeStart->fCost = 0.0f;

    openQueue.push(nodeStart);

    int currentStep = 0;
    bool targetFound = false;

    while (!openQueue.empty())
    {
        Node* nodeCurrent = openQueue.front();
        openQueue.pop();

        closedNodes.insert(nodeCurrent);

        nodeCurrent->step = currentStep++;

        if (nodeCurrent == nodeEnd)
        {
            targetFound = true;
        }
        else
        {
            for (Node* nodeNeighbour : nodeCurrent->vecNeighbours)
            {
                if (nodeNeighbour == nullptr)
                    continue;

                if (nodeNeighbour->bObstacle)
                    continue;

                // Im 2D-Modus keine 3D-Nachbarn verwenden
                if (!draw3D && nodeNeighbour->z != 0)
                    continue;

                /*
                 * bVisited bedeutet hier "bereits entdeckt".
                 * Der Knoten wird schon beim Einfügen markiert,
                 * damit er nicht mehrfach in die Queue gelangt.
                 */
                if (nodeNeighbour->bVisited)
                    continue;

                nodeNeighbour->bVisited = true;
                nodeNeighbour->parent = nodeCurrent;

                /*
                 * BFS optimiert die Anzahl der Kanten/Schritte.
                 * Jede Kante hat deshalb Kosten von 1.
                 */
                nodeNeighbour->gCost =
                    nodeCurrent->gCost + 1.0f;

                nodeNeighbour->hCost = 0.0f;
                nodeNeighbour->fCost = nodeNeighbour->gCost;

                openQueue.push(nodeNeighbour);
            }
        }

        NodeStep snapshot;

        snapshot.current = glm::ivec3(
            nodeCurrent->x,
            nodeCurrent->y,
            nodeCurrent->z
        );

        // Closedlist: bereits aus der Queue entfernte Knoten
        for (Node* node : closedNodes)
        {
            snapshot.closed.emplace_back(
                node->x,
                node->y,
                node->z
            );
        }

        // Openlist: aktuell in der FIFO-Queue wartende Knoten
        std::queue<Node*> queueCopy = openQueue;

        while (!queueCopy.empty())
        {
            Node* node = queueCopy.front();
            queueCopy.pop();

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

    // Kürzesten Pfad anhand der Elternknoten rekonstruieren
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