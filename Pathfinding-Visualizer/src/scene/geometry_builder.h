#pragma once

#include <vector>
#include "glm/glm.hpp"

#include "cube_vertex.h"
#include "cube_edge_vertex.h"
#include "connection_vertex.h"
#include "skybox_structures.h"
#include "quad_vertex.h"
#include "quad_edge_vertex.h"

class GeometryBuilder {
public:
	std::vector<CubeVertex> cubeVertices;
	std::vector<uint32_t> cubeIndices;

	std::vector<CubeEdgeVertex> cubeEdgeVertices;
	std::vector<uint32_t> cubeEdgeIndices;

	std::vector<ConnectionVertex> connectionVerticesLeft;
	std::vector<ConnectionVertex> connectionVerticesRight;

	std::vector<uint32_t> connectionIndicesLeft;
	std::vector<uint32_t> connectionIndicesRight;

	std::vector<QuadVertex> quadVertices;
	std::vector<uint32_t> quadIndices;

	std::vector<QuadEdgeVertex > quadEdgeVertices;
	std::vector<uint32_t> quadEdgeIndices;

	std::vector<SkyboxVertex> skyboxVertices;
	std::vector<uint32_t> skyboxIndices;
	std::array<FaceRegion, 6> skyboxFaces = { {
		{2, 1}, // +X
		{0, 1}, // -X
		{1, 0}, // +Y
		{1, 2}, // -Y
		{1, 1}, // +Z
		{3, 1}  // -Z
	} };

	void createUnitCube(int xId, int yId, int zId);
	void createCubeEdges(float xId, float yId, float zId);
	void createUnitQuad(int xId, int yId);
	void createQuadEdges(float xId, float yId);
	void createPathConnection(glm::vec3 a, glm::vec3 b, float thickness, bool isLeftViewport);
	void createSkybox();
	void rebuildPathGeometry(const std::vector<glm::ivec3>& path, bool isLeftViewport);
};