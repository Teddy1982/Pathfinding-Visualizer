#include "fringe_search.h"
#include "algoLogic.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <list>
#include <unordered_set>
#include <vector>
#include <chrono>

Fringe_Search::Fringe_Search()
    : PathfindingAlgorithm("Fringe-Search")
{
    isGCost = true;
    isFCost = true;
    isHCost = true;
    isFLimit = true;
}

std::vector<glm::ivec3> Fringe_Search::search(
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

    // Aktive Rasterknoten zurücksetzen
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
                node.fLimit = infinity;
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
            if (searchDirections == SEARCH_4_DIRECTIONS ||
                searchDirections == SEARCH_6_DIRECTIONS)
            {
                return manhattanDistance(node, nodeEnd);
            }

            return euclideanDistance(node, nodeEnd);
        };

    /*
     * Entfernt alle alten Vorkommen eines Knotens.
     * Das ist nötig, wenn ein günstigerer Weg gefunden wurde.
     */
    auto removeNode = [](std::list<Node*>& list, Node* node)
        {
            list.remove(node);
        };

    std::list<Node*> listNow;
    std::list<Node*> listLater;

    nodeStart->gCost = 0.0f;
    nodeStart->hCost = heuristic(nodeStart);
    nodeStart->fCost =
        nodeStart->gCost + nodeStart->hCost;

    float fLimit = nodeStart->fCost;
    nodeStart->fLimit = fLimit;

    listNow.push_back(nodeStart);

    std::unordered_set<Node*> closedNodes;

    int currentStep = 0;
    bool targetFound = false;

    while (!listNow.empty())
    {
        float nextFLimit = infinity;

        /*
         * bVisited dient hier nur der Darstellung und nicht
         * dazu, eine erneute Expansion zu verhindern.
         */
        for (int x = 0; x < logic.xSize; ++x)
        {
            for (int y = 0; y < logic.ySize; ++y)
            {
                for (int z = 0; z < gridZ; ++z)
                {
                    Node& node =
                        logic.nodes[
                            logic.GetIndex3D(x, y, z)
                        ];

                    node.bVisited = false;
                }
            }
        }

        while (!listNow.empty())
        {
            Node* nodeCurrent = listNow.front();
            listNow.pop_front();

            nodeCurrent->hCost = heuristic(nodeCurrent);
            nodeCurrent->fCost =
                nodeCurrent->gCost + nodeCurrent->hCost;
            nodeCurrent->fLimit = fLimit;

            /*
             * Der Knoten überschreitet das aktuelle Limit
             * und wird für die nächste Iteration zurückgestellt.
             */
            if (nodeCurrent->fCost > fLimit + epsilon)
            {
                nextFLimit =
                    std::min(nextFLimit, nodeCurrent->fCost);

                removeNode(listLater, nodeCurrent);
                listLater.push_back(nodeCurrent);

                continue;
            }

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

                    const float newGCost =
                        nodeCurrent->gCost +
                        euclideanDistance(
                            nodeCurrent,
                            nodeNeighbour
                        );

                    /*
                     * Nur ein tatsächlich besserer Weg führt
                     * zu einer neuen Aufnahme in die Fringe.
                     */
                    if (newGCost + epsilon <
                        nodeNeighbour->gCost)
                    {
                        nodeNeighbour->parent = nodeCurrent;
                        nodeNeighbour->gCost = newGCost;
                        nodeNeighbour->hCost =
                            heuristic(nodeNeighbour);
                        nodeNeighbour->fCost =
                            nodeNeighbour->gCost +
                            nodeNeighbour->hCost;
                        nodeNeighbour->fLimit = fLimit;

                        /*
                         * Alten Eintrag entfernen. Der Knoten
                         * kann sich in listNow oder listLater
                         * befinden.
                         */
                        removeNode(listNow, nodeNeighbour);
                        removeNode(listLater, nodeNeighbour);

                        /*
                         * Knoten innerhalb des aktuellen Limits
                         * sofort bearbeiten. Die Einfügung vorne
                         * erzeugt das für Fringe Search typische
                         * tiefenorientierte Verhalten.
                         */
                        if (nodeNeighbour->fCost <=
                            fLimit + epsilon)
                        {
                            listNow.push_front(nodeNeighbour);
                        }
                        else
                        {
                            listLater.push_back(nodeNeighbour);

                            nextFLimit = std::min(
                                nextFLimit,
                                nodeNeighbour->fCost
                            );
                        }
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

            /*
             * Fringe Search besitzt keine permanente Closedlist.
             * Die Openlist besteht aus listNow und listLater.
             */
            std::unordered_set<Node*> snapshotOpen;

            for (Node* node : listNow)
            {
                if (!node->bObstacle)
                    snapshotOpen.insert(node);
            }

            for (Node* node : listLater)
            {
                if (!node->bObstacle)
                    snapshotOpen.insert(node);
            }

            for (Node* node : closedNodes)
            {
                if (!snapshotOpen.contains(node))
                {
                    snapshot.closed.emplace_back(
                        node->x,
                        node->y,
                        node->z
                    );
                }
            }
            logic.nodeSteps.push_back(std::move(snapshot));

            if (targetFound)
                break;
        }

        if (targetFound)
            break;

        if (!std::isfinite(nextFLimit))
            break;

        // Nächste Iteration mit dem kleinsten überschrittenen fCost
        fLimit = nextFLimit;

        listNow.swap(listLater);
        listLater.clear();
    }

    if (!targetFound) {
        auto end = std::chrono::steady_clock::now();
        logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
        return path;
    }

    // Gefundenen Pfad rekonstruieren
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