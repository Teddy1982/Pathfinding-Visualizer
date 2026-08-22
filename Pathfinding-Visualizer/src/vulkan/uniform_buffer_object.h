#pragma once

#include "glm/glm.hpp"

//UniformBufferObject-Struktur
struct UniformBufferObject {
	glm::mat4 proj;
	glm::mat4 view;
	glm::vec3 cameraPosition;
	float xSize;
	glm::vec3 cameraTarget;
	float ySize;
	glm::vec3 id;
	float _pad3;
};