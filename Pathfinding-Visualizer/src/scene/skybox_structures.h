#pragma once

#include <stdint.h>
#include "glm/glm.hpp"


struct SkyboxVertex {
	glm::vec4 position;
};


struct FaceRegion {
	uint32_t x;
	uint32_t y;
};
