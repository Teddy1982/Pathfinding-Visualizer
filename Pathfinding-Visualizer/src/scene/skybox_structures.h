#pragma once

#include <stdint.h>
#include "glm/glm.hpp"

// Vertexstruktur für Skybox
struct SkyboxVertex {
	glm::vec4 position;
};

// Vertexstruktur für Skybox-Wände
struct FaceRegion {
	uint32_t x;
	uint32_t y;
};
