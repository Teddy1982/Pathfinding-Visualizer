#include "algoLogic.h"
#include "../application/app_state.h"

int AlgoLogic::GetIndex3D(int x, int y, int z)
{
    return x + y * xSize + z * xSize * ySize;
}

void AlgoLogic::initArray(int xSiz, int ySiz, int zSiz)
{
    xSize = xSiz;
    ySize = ySiz;
    zSize = zSiz;
    
    array3D.clear();
    array3D.resize(xSize * ySize * zSize);
    
    std::fill(array3D.begin(), array3D.end(), 0);

    startNodeCoords3D = glm::ivec3(-1, -1, -1);
    endNodeCoords3D = glm::ivec3(-1, -1, -1);
}

void AlgoLogic::setStartNode(int x, int y, int z)
{
    for (int i = 0; i < array3D.size(); i++) {
        if (array3D[i] == START_NODE) {
            array3D[i] = 0;
        }
    }
    startNodeCoords3D = glm::ivec3(x, y, z);
}

void AlgoLogic::setEndNode(int x, int y, int z)
{
    for (int i = 0; i < array3D.size(); i++) {
        if (array3D[i] == END_NODE) {
            array3D[i] = 0;
        }
    }

    endNodeCoords3D = glm::ivec3(x, y, z);
}

void AlgoLogic::setObstacleNode(int x, int y, int z)
{
    array3D.at(GetIndex3D(x, y, z)) = OBSTACLE_NODE;
}

void AlgoLogic::eraseNodeValue(int x, int y, int z)
{
    if (array3D.at(GetIndex3D(x, y, z)) == START_NODE) {
        startNodeCoords3D = glm::ivec3(-1, -1, -1);
    }
    if (array3D.at(GetIndex3D(x, y, z)) == END_NODE) {
        endNodeCoords3D = glm::ivec3(-1, -1, -1);
    }
    array3D.at(GetIndex3D(x, y, z)) = 0;
}

void AlgoLogic::NodesInit(int searchDirections, bool draw3D)
{
    int gridZ = zSize;

    if (!draw3D) {
        gridZ = 1;
    }

    nodes.resize(xSize * ySize * gridZ);

    for (int x = 0; x < xSize; x++) {
        for (int y = 0; y < ySize; y++) {
            for (int z = 0; z < gridZ; z++) {
                int idx = GetIndex3D(x, y, z);

                nodes[idx].bObstacle = array3D.at(GetIndex3D(x, y, z)) == OBSTACLE_NODE;
                nodes[idx].bVisited = false;
                nodes[idx].step = -1;

                nodes[idx].gCost = std::numeric_limits<float>::infinity();
                nodes[idx].hCost = 0.0f;
                nodes[idx].fCost = std::numeric_limits<float>::infinity();

                nodes[idx].x = x;
                nodes[idx].y = y;
                nodes[idx].z = z;

                nodes[idx].parent = nullptr;
                nodes[idx].vecNeighbours.clear();
            }
        }
    }

    for (int x = 0; x < xSize; x++) {
        for (int y = 0; y < ySize; y++) {
            for (int z = 0; z < gridZ; z++) {
                int idx = GetIndex3D(x, y, z);

                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dz = -1; dz <= 1; dz++) {

                            if (dx == 0 && dy == 0 && dz == 0)
                                continue;

                            if (searchDirections == SEARCH_6_DIRECTIONS || searchDirections == SEARCH_4_DIRECTIONS)
                            {
                                int steps =
                                    std::abs(dx) +
                                    std::abs(dy) +
                                    std::abs(dz);

                                if (steps != 1)
                                    continue;
                            }

                            int nx = x + dx;
                            int ny = y + dy;
                            int nz = z + dz;

                            if (nx < 0 || nx >= xSize) continue;
                            if (ny < 0 || ny >= ySize) continue;
                            if (nz < 0 || nz >= gridZ) continue;

                            nodes[idx].vecNeighbours.push_back(
                                &nodes[GetIndex3D(nx, ny, nz)]
                            );
                        }
                    }
                }
            }
        }
    }
}
