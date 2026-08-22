#include "floyd_warshall.h"
#include "algoLogic.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <unordered_set>

Floyd_Warshall::Floyd_Warshall()
    : PathfindingAlgorithm("Floyd-Warshall")
{
    isGCost = true;
}

std::vector<glm::ivec3> Floyd_Warshall::search(
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

    const bool useManhattanCosts =
        searchDirections == SEARCH_4_DIRECTIONS ||
        searchDirections == SEARCH_6_DIRECTIONS;

    logic.heuristicMode = useManhattanCosts
        ? MANHATTAN_DISTANCE
        : EUCLID_DISTANCE;

    std::vector<Node*> activeNodes;

    // Aktive, nicht blockierte Knoten sammeln
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
                node->parent = nullptr;
                node->gCost = infinity;
                node->hCost = 0.0f;
                node->fCost = infinity;

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

    const int nodeCount =
        static_cast<int>(activeNodes.size());

    if (nodeCount == 0) {
        auto end = std::chrono::steady_clock::now();
        logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
        return path;
    }

    /*
     * Zuordnung zwischen Node-Zeigern und kompakten
     * Floyd-Warshall-Matrixindizes.
     */
    std::unordered_map<Node*, int> nodeToIndex;

    for (int i = 0; i < nodeCount; ++i)
        nodeToIndex[activeNodes[i]] = i;

    const auto startIt = nodeToIndex.find(nodeStart);
    const auto endIt = nodeToIndex.find(nodeEnd);

    if (startIt == nodeToIndex.end() ||
        endIt == nodeToIndex.end())
    {
        auto end = std::chrono::steady_clock::now();
        logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
        return path;
    }

    const int startIndex = startIt->second;
    const int endIndex = endIt->second;

    auto manhattanDistance = [](const Node* first, const Node* second) -> float
        {
            return static_cast<float>(
                std::abs(first->x - second->x) +
                std::abs(first->y - second->y) +
                std::abs(first->z - second->z)
                );
        };

    auto euclideanDistance = [](const Node* first, const Node* second) -> float
        {
            const float dx =
                static_cast<float>(first->x - second->x);
            const float dy =
                static_cast<float>(first->y - second->y);
            const float dz =
                static_cast<float>(first->z - second->z);

            return std::sqrt(
                dx * dx +
                dy * dy +
                dz * dz
            );
        };

    auto edgeCost = [&](const Node* first, const Node* second) -> float
        {
            if (useManhattanCosts)
                return manhattanDistance(first, second);

            return euclideanDistance(first, second);
        };

    std::vector<std::vector<float>> dist(
        nodeCount,
        std::vector<float>(nodeCount, infinity)
    );

    /*
     * next[i][j] enthält den nächsten Matrixindex,
     * wenn man von i in Richtung j läuft.
     */
    std::vector<std::vector<int>> next(
        nodeCount,
        std::vector<int>(nodeCount, -1)
    );

    // Distanz von jedem Knoten zu sich selbst
    for (int i = 0; i < nodeCount; ++i)
    {
        dist[i][i] = 0.0f;
        next[i][i] = i;
    }

    // Direkte Kanten eintragen
    for (int i = 0; i < nodeCount; ++i)
    {
        Node* nodeCurrent = activeNodes[i];

        for (Node* nodeNeighbour :
            nodeCurrent->vecNeighbours)
        {
            if (nodeNeighbour == nullptr ||
                nodeNeighbour->bObstacle)
            {
                continue;
            }

            // Im 2D-Modus keine Nachbarn anderer Ebenen
            if (!draw3D && nodeNeighbour->z != 0)
                continue;

            const auto neighbourIt =
                nodeToIndex.find(nodeNeighbour);

            if (neighbourIt == nodeToIndex.end())
                continue;

            const int j = neighbourIt->second;
            const float cost =
                edgeCost(nodeCurrent, nodeNeighbour);

            // Relevant, falls mehrere Kanten existieren
            if (cost < dist[i][j])
            {
                dist[i][j] = cost;
                next[i][j] = j;
            }
        }
    }

    std::unordered_set<Node*>closedNodes;
    closedNodes.reserve(nodeCount);

    int currentStep = 0;

    /*
     * Floyd-Warshall:
     * k muss die äußerste Schleife sein.
     */
    for (int k = 0; k < nodeCount; ++k)
    {
        for (int i = 0; i < nodeCount; ++i)
        {
            if (!std::isfinite(dist[i][k]))
                continue;

            for (int j = 0; j < nodeCount; ++j)
            {
                if (!std::isfinite(dist[k][j]))
                    continue;

                const float newDistance =
                    dist[i][k] + dist[k][j];

                if (newDistance + epsilon < dist[i][j])
                {
                    dist[i][j] = newDistance;
                    next[i][j] = next[i][k];
                }
            }
        }

        Node* intermediateNode = activeNodes[k];

        intermediateNode->bVisited = true;
        intermediateNode->step = currentStep++;
        closedNodes.insert(intermediateNode);

        /*
         * Kosten vom ausgewählten Startknoten für die
         * Darstellung aktualisieren.
         *
         * Diese Kosten können sich in späteren k-Iterationen
         * noch weiter verbessern.
         */

        for (int i = 0; i < nodeCount; ++i)
        {
            Node* node = activeNodes[i];
            node->gCost = dist[startIndex][i];
            node->hCost = 0.0f;
            node->fCost = dist[startIndex][i];
        }

        NodeStep snapshot;

        snapshot.current = glm::ivec3(
            intermediateNode->x,
            intermediateNode->y,
            intermediateNode->z
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
         * snapshot.closed enthält die Knoten, die bereits
         * als Zwischenknoten verarbeitet wurden.
         * Eine klassische Open-List besitzt Floyd-Warshall nicht.
         */
        logic.nodeSteps.push_back(std::move(snapshot));
    }

    /*
     * Bei dist[i][i] < 0 existiert ein negativer Zyklus,
     * der von i aus erreichbar ist.
     */
    for (int i = 0; i < nodeCount; ++i)
    {
        if (dist[i][i] < -epsilon)
        {
            // Kein wohldefinierter kürzester Pfad.
            auto end = std::chrono::steady_clock::now();
            logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();

            return path;
        }
    }

    if (next[startIndex][endIndex] == -1) {
        auto end = std::chrono::steady_clock::now();
        logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
        return path;
    }

    // Pfad rekonstruieren
    int currentIndex = startIndex;

    path.emplace_back(
        activeNodes[currentIndex]->x,
        activeNodes[currentIndex]->y,
        activeNodes[currentIndex]->z
    );

    /*
     * Sicherheitsbegrenzung gegen eine fehlerhafte oder
     * zyklische next-Matrix.
     */
    int reconstructionSteps = 0;

    while (currentIndex != endIndex)
    {
        currentIndex = next[currentIndex][endIndex];

        if (currentIndex < 0 ||
            currentIndex >= nodeCount)
        {
            path.clear();
            auto end = std::chrono::steady_clock::now();
            logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();
            return path;
        }

        Node* currentNode = activeNodes[currentIndex];

        path.emplace_back(
            currentNode->x,
            currentNode->y,
            currentNode->z
        );

        ++reconstructionSteps;

        if (reconstructionSteps > nodeCount)
        {
            path.clear();

            auto end = std::chrono::steady_clock::now();
            logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();

            return path;
        }
    }

    /*
     * Parent anhand der rekonstruierten Matrixindizes setzen.
     * Das ist für Floyd-Warshall selbst nicht erforderlich,
     * kann aber von deiner Oberfläche verwendet werden.
     */
    currentIndex = startIndex;
    Node* previousNode = nullptr;
    int parentSteps = 0;

    while (true)
    {
        if (currentIndex < 0 || currentIndex >= nodeCount)
        {
            path.clear();
            break;
        }

        Node* currentNode = activeNodes[currentIndex];
        currentNode->parent = previousNode;

        if (currentIndex == endIndex)
            break;

        previousNode = currentNode;
        currentIndex = next[currentIndex][endIndex];

        if (++parentSteps > nodeCount)
        {
            path.clear();
            break;
        }
    }

    auto end = std::chrono::steady_clock::now();
    logic.runtime = std::chrono::duration<double, std::milli>(end - start).count();

    return path;
}