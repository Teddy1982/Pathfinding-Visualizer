#include "grid_visualizer.h"
#include "../vulkan/renderData.h"
#include "../pathfinding/algoLogic.h"
#include "../application/app_state.h"
#include "../scene/geometry_builder.h"

#include "../globals/global_constants.h"

#include "../tools/logger.h"
#include <iostream>

#include <stdexcept>

GridVisualizer::GridVisualizer(RenderData& rData, AlgoLogic& logicLeft, AlgoLogic& logicRight, GeometryBuilder& geoBuilder, AppState& state) : renderData(rData), algoLogicLeft(logicLeft), algoLogicRight(logicRight), geometryBuilder(geoBuilder), appState(state) {}

void GridVisualizer::updateCubeColorsLeft(std::array<int, 3> coords, std::array<float, 4> scale, std::array<float, 4> color) {
	auto idx = toIndex(coords[0], coords[1], coords[2]);

	appState.cubeColorsLeft[idx].color[0] = color[0];
	appState.cubeColorsLeft[idx].color[1] = color[1];
	appState.cubeColorsLeft[idx].color[2] = color[2];
	appState.cubeColorsLeft[idx].color[3] = color[3];

	appState.cubeColorsLeft[idx].scale[0] = scale[0];
	appState.cubeColorsLeft[idx].scale[1] = scale[1];
	appState.cubeColorsLeft[idx].scale[2] = scale[2];
	appState.cubeColorsLeft[idx].scale[3] = scale[3];
}

void GridVisualizer::updateCubeColorsRight(std::array<int, 3> coords, std::array<float, 4> scale, std::array<float, 4> color) {
	auto idx = toIndex(coords[0], coords[1], coords[2]);

	appState.cubeColorsRight[idx].color[0] = color[0];
	appState.cubeColorsRight[idx].color[1] = color[1];
	appState.cubeColorsRight[idx].color[2] = color[2];
	appState.cubeColorsRight[idx].color[3] = color[3];

	appState.cubeColorsRight[idx].scale[0] = scale[0];
	appState.cubeColorsRight[idx].scale[1] = scale[1];
	appState.cubeColorsRight[idx].scale[2] = scale[2];
	appState.cubeColorsRight[idx].scale[3] = scale[3];
}

void GridVisualizer::drawPathLeft() {
	for (int i = 0; i < appState.pathLeft.size(); i++) {
		auto idx = toIndex(appState.pathLeft[i].x, appState.pathLeft[i].y, appState.pathLeft[i].z);

		updateCubeColorsLeft({ appState.pathLeft[i].x, appState.pathLeft[i].y, appState.pathLeft[i].z }, HALF_SCALE, YELLOW);
	}
}

void GridVisualizer::drawPathRight() {
	for (int i = 0; i < appState.pathRight.size(); i++) {
		auto idx = toIndex(appState.pathRight[i].x, appState.pathRight[i].y, appState.pathRight[i].z);

		updateCubeColorsRight({ appState.pathRight[i].x, appState.pathRight[i].y, appState.pathRight[i].z }, HALF_SCALE, YELLOW);
	}
}

void GridVisualizer::updateVisualizationLeft(int stepValue)
{
    appState.visualStepLeft += stepValue;
    appState.stepValueLeft = 0;

    if (appState.visualStepLeft < 0) {
        appState.visualStepLeft = 0;
    }

    if (appState.visualStepLeft >= algoLogicLeft.nodeSteps.size())
    {
        appState.visualStepLeft = static_cast<int>(algoLogicLeft.nodeSteps.size());

        drawPathLeft();
        updateStorageBufferLeft();

        geometryBuilder.rebuildPathGeometry(appState.pathLeft, true);

        updateConnectionBuffer(true);

        if (!appState.pathLeft.empty()) {
            appState.showPathLeft = true;
        }

        return;
    }

    if (!geometryBuilder.connectionIndicesLeft.empty()) {
        geometryBuilder.connectionVerticesLeft.clear();
        geometryBuilder.connectionIndicesLeft.clear();
        updateConnectionBuffer(true);
    }

    const NodeStep& step =
        algoLogicLeft.nodeSteps[appState.visualStepLeft];


    for (int x = 0; x < appState.xSize; ++x) {
        for (int y = 0; y < appState.ySize; ++y) {

            if (!appState.draw3D) {
                if (algoLogicLeft.array3D[toIndex(x, y, 0)] != OBSTACLE_NODE)
                {
                    updateCubeColorsLeft({ x, y, 0 }, ZERO_SCALE, CLEAR);
                }
            }
            else {
                for (int z = 0; z < appState.zSize; ++z) {
                    if (algoLogicLeft.array3D[toIndex(x, y, z)] != OBSTACLE_NODE)
                    {
                        updateCubeColorsLeft({ x, y, z }, ZERO_SCALE, CLEAR);
                    }
                }
            }
        }
    }

    updateCubeColorsLeft({ algoLogicLeft.endNodeCoords3D.x, algoLogicLeft.endNodeCoords3D.y, appState.draw3D ? algoLogicLeft.endNodeCoords3D.z : 0 }, FULL_SCALE, PINK);

    for (const auto& p : step.open) {
        updateCubeColorsLeft({ p.x, p.y, appState.draw3D ? p.z : 0 }, HALF_SCALE, GREEN);
    }

    for (const auto& p : step.closed) {
        updateCubeColorsLeft({ p.x, p.y, appState.draw3D ? p.z : 0 }, HALF_SCALE, RED);
    }

    int currentZ = appState.draw3D ? step.current.z : 0;

    uint32_t currentIndex = toIndex(step.current.x, step.current.y, currentZ);

    updateCubeColorsLeft({ step.current.x, step.current.y, currentZ }, HALF_SCALE, WHITE);
}

void GridVisualizer::updateVisualizationRight(int stepValue)
{
	appState.visualStepRight += stepValue;
	appState.stepValueRight = 0;

	if (appState.visualStepRight < 0) {
		appState.visualStepRight = 0;
	}

	if (appState.visualStepRight >= algoLogicRight.nodeSteps.size())
	{
		appState.visualStepRight = algoLogicRight.nodeSteps.size();

		drawPathRight();
		updateStorageBufferRight();

		geometryBuilder.rebuildPathGeometry(appState.pathRight, false);
		updateConnectionBuffer(false);

		if (!appState.pathRight.empty()) {
			appState.showPathRight = true;
		}

		return;
	}

	if (geometryBuilder.connectionIndicesRight.size() > 0) {
		geometryBuilder.connectionVerticesRight.clear();
		geometryBuilder.connectionIndicesRight.clear();
		updateConnectionBuffer(false);
	}

	const NodeStep& step = algoLogicRight.nodeSteps[appState.visualStepRight];

	for (int x = 0; x < appState.xSize; x++) {
		for (int y = 0; y < appState.ySize; y++) {
			if (!appState.draw3D) {
				if (algoLogicRight.array3D[toIndex(x, y, 0)] != OBSTACLE_NODE)
				{
					updateCubeColorsRight({ x, y, 0 }, ZERO_SCALE, CLEAR);
				}
				continue;
			}
			for (int z = 0; z < appState.zSize; z++) {
				if (algoLogicRight.array3D[toIndex(x, y, z)] != OBSTACLE_NODE)
				{
					updateCubeColorsRight({ x, y, z }, ZERO_SCALE, CLEAR);
				}
			}
		}
	}

	updateCubeColorsRight({ algoLogicRight.endNodeCoords3D.x, algoLogicRight.endNodeCoords3D.y, algoLogicRight.endNodeCoords3D.z }, FULL_SCALE, PINK);

	for (const auto& p : step.open) {
		updateCubeColorsRight({ p.x, p.y, p.z }, HALF_SCALE, GREEN);
	}

	for (const auto& p : step.closed) {
		updateCubeColorsRight({ p.x, p.y, p.z }, HALF_SCALE, RED);
	}

	updateCubeColorsRight({ step.current.x, step.current.y, step.current.z }, HALF_SCALE, WHITE);
}

uint32_t GridVisualizer::toIndex(int x, int y, int z) {
	return x + y * appState.xSize + z * appState.xSize * appState.ySize;
}

void GridVisualizer::updateStorageBufferLeft() {
	if (appState.cubeColorsLeft.size() == 0) {
		return;
	}
	void* data;
	vkMapMemory(renderData.vkInst.device, renderData.storageBuffer.memory, 0,
		sizeof(CubeState) * appState.cubeColorsLeft.size(), 0, &data);
	memcpy(data, appState.cubeColorsLeft.data(), sizeof(CubeState) * appState.cubeColorsLeft.size());
	vkUnmapMemory(renderData.vkInst.device, renderData.storageBuffer.memory);
}

void GridVisualizer::updateStorageBufferRight() {
	if (appState.cubeColorsRight.size() == 0) {
		return;
	}
	void* data;
	vkMapMemory(renderData.vkInst.device, renderData.storageBuffer.memoryRight, 0,
		sizeof(CubeState) * appState.cubeColorsRight.size(), 0, &data);
	memcpy(data, appState.cubeColorsRight.data(), sizeof(CubeState) * appState.cubeColorsRight.size());
	vkUnmapMemory(renderData.vkInst.device, renderData.storageBuffer.memoryRight);
}

void GridVisualizer::updateConnectionBuffer(bool isLeftViewport)
{
    void* data = nullptr;

    if (isLeftViewport) {
        const VkDeviceSize vertexSize = sizeof(ConnectionVertex) * geometryBuilder.connectionVerticesLeft.size();

        if (vertexSize > 0) {
            if (vkMapMemory(renderData.vkInst.device, renderData.vertexBuffer.connectionVMemory, 0, vertexSize, 0, &data) != VK_SUCCESS) {
                throw std::runtime_error("Failed to map left connection vertex memory!");
            }
            std::memcpy(data, geometryBuilder.connectionVerticesLeft.data(), static_cast<std::size_t>(vertexSize));
            vkUnmapMemory(renderData.vkInst.device, renderData.vertexBuffer.connectionVMemory);
        }
    }
    else {
        const VkDeviceSize vertexSize = sizeof(ConnectionVertex) * geometryBuilder.connectionVerticesRight.size();

        if (vertexSize > 0) {
            if (vkMapMemory(renderData.vkInst.device, renderData.vertexBuffer.connectionVMemoryRight, 0, vertexSize, 0, &data) != VK_SUCCESS) {
                throw std::runtime_error(
                    "Failed to map right connection vertex memory!"
                );
            }
            std::memcpy(data, geometryBuilder.connectionVerticesRight.data(), static_cast<std::size_t>(vertexSize));
			vkUnmapMemory(renderData.vkInst.device, renderData.vertexBuffer.connectionVMemoryRight);
        }
    }

    if (isLeftViewport) {
        const VkDeviceSize indexSize = sizeof(uint32_t) * geometryBuilder.connectionIndicesLeft.size();
        
		if (indexSize > 0) {
            if (vkMapMemory(renderData.vkInst.device, renderData.vertexBuffer.connectionIMemory, 0, indexSize, 0, &data) != VK_SUCCESS) {
                throw std::runtime_error("Failed to map left connection index memory!");
            }
			std::memcpy(data, geometryBuilder.connectionIndicesLeft.data(), static_cast<std::size_t>(indexSize));
			vkUnmapMemory(renderData.vkInst.device, renderData.vertexBuffer.connectionIMemory);
        }
    }
    else {
        const VkDeviceSize indexSize = sizeof(uint32_t) * geometryBuilder.connectionIndicesRight.size();
		
		if (indexSize > 0) {
            if (vkMapMemory(renderData.vkInst.device, renderData.vertexBuffer.connectionIMemoryRight, 0, indexSize, 0, &data) != VK_SUCCESS) {
                throw std::runtime_error("Failed to map right connection index memory!");
            }
			std::memcpy(data, geometryBuilder.connectionIndicesRight.data(), static_cast<std::size_t>(indexSize));
			vkUnmapMemory(renderData.vkInst.device, renderData.vertexBuffer.connectionIMemoryRight);
        }
    }
}
