#pragma once

#include <vector>
#include <array>

struct RenderData;
class AlgoLogic;
class GeometryBuilder;
struct AppState;

class GridVisualizer {
public:

	GeometryBuilder& geometryBuilder;

	GridVisualizer(RenderData& rData, AlgoLogic& logicLeft, AlgoLogic& logicRight, GeometryBuilder& geoBuilder, AppState& state);
	uint32_t toIndex(int x, int y, int z);
	
	void updateCubeColorsLeft(std::array<int, 3> coords, std::array<float, 4> scale, std::array<float, 4> color);
	void updateCubeColorsRight(std::array<int, 3> coords, std::array<float, 4> scale, std::array<float, 4> color);
	void drawPathLeft();
	void drawPathRight();
	void updateVisualizationLeft(int stepValue);
	void updateVisualizationRight(int stepValue);
	void updateStorageBufferLeft();
	void updateStorageBufferRight();
	void updateConnectionBuffer(bool isLeftViewport);

private:
	RenderData& renderData;
	AlgoLogic& algoLogicLeft;
	AlgoLogic& algoLogicRight;
	AppState& appState;
};
