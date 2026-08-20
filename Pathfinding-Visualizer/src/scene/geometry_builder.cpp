#include "geometry_builder.h"

void GeometryBuilder::createUnitCube(int xId, int yId, int zId) {

	// Cube vertex positions
	float positions[8][3] = {
		{-0.49f, -0.49f, -0.49f}, {0.49f, -0.49f, -0.49f}, {0.49f, 0.49f, -0.49f}, {-0.49f, 0.49f, -0.49f}, // Back
		{-0.49f, -0.49f, 0.49f}, {0.49f, -0.49f, 0.49f}, {0.49f, 0.49f, 0.49f}, {-0.49f, 0.49f, 0.49f} // Front
	};

	// Normals for each face
	float normals[6][3] = {
		{0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}
	};

	// UV coordinates (w is always0)
	float colors[4][3] = {
		{0.2f, 0.2f, 0.2f}, {0.2f, 0.2f, 0.2f}, {0.2f, 0.2f, 0.2f}, {0.2f, 0.2f, 0.2f}
	};

	// Faces defined by vertex indices
	struct Face {
		int indices[4];
		int normalIndex;
	} faces[6] = {
		{{0, 1, 2, 3}, 0}, // Back
		{{5, 4, 7, 6}, 1}, // Front
		{{4, 0, 3, 7}, 2}, // Left
		{{1, 5, 6, 2}, 3}, // Right
		{{4, 5, 1, 0}, 4}, // Bottom
		{{3, 2, 6, 7}, 5}  // Top
	};

	for (const auto& face : faces) {
		for (int i = 0; i < 4; ++i) {
			cubeVertices.push_back({
				{positions[face.indices[i]][0] + xId, positions[face.indices[i]][1] + yId, positions[face.indices[i]][2] + zId},
				{colors[i][0], colors[i][1], colors[i][2]},
				{normals[face.normalIndex][0], normals[face.normalIndex][1], normals[face.normalIndex][2]},
				{xId, yId, zId}
				});
		}
		// Generate two triangles per face
		cubeIndices.push_back(static_cast<uint32_t>(cubeVertices.size() - 4));
		cubeIndices.push_back(static_cast<uint32_t>(cubeVertices.size() - 3));
		cubeIndices.push_back(static_cast<uint32_t>(cubeVertices.size() - 2));
		cubeIndices.push_back(static_cast<uint32_t>(cubeVertices.size() - 4));
		cubeIndices.push_back(static_cast<uint32_t>(cubeVertices.size() - 2));
		cubeIndices.push_back(static_cast<uint32_t>(cubeVertices.size() - 1));
	}
};

void GeometryBuilder::createCubeEdges(float xId, float yId, float zId) {
	float positions[8][3] = {
		{-0.495f, -0.495f, -0.495f}, {0.495f, -0.495f, -0.495f}, {0.495f, 0.495f, -0.495f}, {-0.495f, 0.495f, -0.495f}, // Back
		{-0.495f, -0.495f, 0.495f}, {0.495f, -0.495f, 0.495f}, {0.495f, 0.495f, 0.495f}, {-0.495f, 0.495f, 0.495f} // Front
	};

	uint32_t edgeBase = static_cast<uint32_t>(cubeEdgeVertices.size());

	for (int i = 0; i < 8; ++i) {
		cubeEdgeVertices.push_back({
			{positions[i][0] + xId, positions[i][1] + yId, positions[i][2] + zId},
			{xId, yId, zId}
			});
	}

	uint32_t cubeEdges[12][2] = {
		{0,1}, {1,2}, {2,3}, {3,0},
		{4,5}, {5,6}, {6,7}, {7,4},
		{0,4}, {1,5}, {2,6}, {3,7}
	};

	for (const auto& edge : cubeEdges) {
		cubeEdgeIndices.push_back(edgeBase + edge[0]);
		cubeEdgeIndices.push_back(edgeBase + edge[1]);
	}
};

void GeometryBuilder::createPathConnection(glm::vec3 a, glm::vec3 b, float thickness, bool isLeftViewport) {
	glm::vec3 forward = b - a;
	float length = glm::length(forward);

	if (length < 0.0001f) {
		return;
	}

	forward = glm::normalize(forward);

	// Hilfsvektor wählen, der möglichst nicht parallel zu forward ist
	glm::vec3 helper = glm::vec3(0.0f, 1.0f, 0.0f);
	if (std::abs(glm::dot(forward, helper)) > 0.99f) {
		helper = glm::vec3(1.0f, 0.0f, 0.0f);
	}

	glm::vec3 right = glm::normalize(glm::cross(forward, helper));
	glm::vec3 up = glm::normalize(glm::cross(right, forward));

	float halfThickness = thickness * 0.5f;

	// Quader-Ecken:
	// vorne = bei A, hinten = bei B
	glm::vec3 r = right * halfThickness;
	glm::vec3 u = up * halfThickness;

	glm::vec3 p0 = a - r - u;
	glm::vec3 p1 = a + r - u;
	glm::vec3 p2 = a + r + u;
	glm::vec3 p3 = a - r + u;

	glm::vec3 p4 = b - r - u;
	glm::vec3 p5 = b + r - u;
	glm::vec3 p6 = b + r + u;
	glm::vec3 p7 = b - r + u;

	uint32_t inds[] = {
		// Seite A
		0, 1, 2,  0, 2, 3,
		// Seite B
		4, 6, 5,  4, 7, 6,
		// Seitenflächen
		0, 4, 5,  0, 5, 1,
		1, 5, 6,  1, 6, 2,
		2, 6, 7,  2, 7, 3,
		3, 7, 4,  3, 4, 0
	};

	if (isLeftViewport) {

		uint32_t base = static_cast<uint32_t>(connectionVerticesLeft.size());

		connectionVerticesLeft.push_back({ p0.x, p0.y, p0.z });
		connectionVerticesLeft.push_back({ p1.x, p1.y, p1.z });
		connectionVerticesLeft.push_back({ p2.x, p2.y, p2.z });
		connectionVerticesLeft.push_back({ p3.x, p3.y, p3.z });
		connectionVerticesLeft.push_back({ p4.x, p4.y, p4.z });
		connectionVerticesLeft.push_back({ p5.x, p5.y, p5.z });
		connectionVerticesLeft.push_back({ p6.x, p6.y, p6.z });
		connectionVerticesLeft.push_back({ p7.x, p7.y, p7.z });

		for (uint32_t i : inds) {
			connectionIndicesLeft.push_back(base + i);
		}
	}
	else {
		uint32_t base = static_cast<uint32_t>(connectionVerticesRight.size());

		connectionVerticesRight.push_back({ p0.x, p0.y, p0.z });
		connectionVerticesRight.push_back({ p1.x, p1.y, p1.z });
		connectionVerticesRight.push_back({ p2.x, p2.y, p2.z });
		connectionVerticesRight.push_back({ p3.x, p3.y, p3.z });
		connectionVerticesRight.push_back({ p4.x, p4.y, p4.z });
		connectionVerticesRight.push_back({ p5.x, p5.y, p5.z });
		connectionVerticesRight.push_back({ p6.x, p6.y, p6.z });
		connectionVerticesRight.push_back({ p7.x, p7.y, p7.z });

		for (uint32_t i : inds) {
			connectionIndicesRight.push_back(base + i);
		}
	}
}

void GeometryBuilder::createSkybox() {
	skyboxVertices = {
		// 0
		{ glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f) },

		// 1
		{ glm::vec4(1.0f, -1.0f, -1.0f, 1.0f) },

		// 2
		{ glm::vec4(1.0f,  1.0f, -1.0f, 1.0f) },

		// 3
		{ glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f) },

		// 4
		{ glm::vec4(-1.0f, -1.0f,  1.0f, 1.0f) },

		// 5
		{ glm::vec4(1.0f, -1.0f,  1.0f, 1.0f) },

		// 6
		{ glm::vec4(1.0f,  1.0f,  1.0f, 1.0f) },

		// 7
		{ glm::vec4(-1.0f,  1.0f,  1.0f, 1.0f) }
	};

	skyboxIndices = {
		// front  z = +1
		4, 6, 7,
		4, 5, 6,

		// back   z = -1
		0, 3, 2,
		0, 2, 1,

		// left   x = -1
		4, 7, 3,
		4, 3, 0,

		// right  x = +1
		5, 2, 6,
		5, 1, 2,

		// top    y = +1
		6, 3, 7,
		6, 2, 3,

		// bottom y = -1
		5, 4, 0,
		5, 0, 1
	};
}

void GeometryBuilder::rebuildPathGeometry(const std::vector<glm::ivec3>& path, bool isLeftViewport) {
	if (isLeftViewport) {
		connectionVerticesLeft.clear();
		connectionIndicesLeft.clear();
	}
	else {
		connectionVerticesRight.clear();
		connectionIndicesRight.clear();
	}

	for (size_t i = 0; i + 1 < path.size(); ++i) {
		glm::ivec3 a = glm::ivec3(path[i]);
		glm::ivec3 b = glm::ivec3(path[i + 1]);
		createPathConnection(a, b, 0.12f, isLeftViewport);
	}
}

// 2D-VERTEXSTRUCTS
//########################################################################################################

void GeometryBuilder::createUnitQuad(int xId, int yId)
{
	constexpr float halfSize = 0.49f;

	float positions[4][3] = {
		{-halfSize, -halfSize, 0.0f},
		{ halfSize, -halfSize, 0.0f},
		{ halfSize,  halfSize, 0.0f},
		{-halfSize,  halfSize, 0.0f}
	};

	uint32_t baseIndex =
		static_cast<uint32_t>(quadVertices.size());

	for (int i = 0; i < 4; ++i) {
		quadVertices.push_back({
			{
				positions[i][0] + xId,
				positions[i][1] + yId,
				0.0f
			},
			{
				xId,
				yId,
				0
			}
			});
	}

	quadIndices.insert(
		quadIndices.end(),
		{
			baseIndex + 0,
			baseIndex + 1,
			baseIndex + 2,

			baseIndex + 0,
			baseIndex + 2,
			baseIndex + 3
		}
	);
}
void GeometryBuilder::createQuadEdges(float xId, float yId) {
	constexpr float halfSize = 0.495f;

	float positions[4][3] = {
		{-halfSize, -halfSize, 0.0f},
		{ halfSize, -halfSize, 0.0f},
		{ halfSize,  halfSize, 0.0f},
		{-halfSize,  halfSize, 0.0f}
	};

	uint32_t edgeBase = static_cast<uint32_t>(quadEdgeVertices.size());

	for (int i = 0; i < 4; ++i) {
		quadEdgeVertices.push_back({
			{positions[i][0] + xId, positions[i][1] + yId, 0.0f},
			{static_cast<int>(xId), static_cast<int>(yId), 0}
			});
	}

	uint32_t quadEdges[4][2] = {
		{0,1}, {1,2}, {2,3}, {3,0}
	};

	for (const auto& edge : quadEdges) {
		quadEdgeIndices.push_back(edgeBase + edge[0]);
		quadEdgeIndices.push_back(edgeBase + edge[1]);
	}
};
