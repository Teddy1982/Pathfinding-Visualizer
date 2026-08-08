#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera  {
public:
	Camera(glm::vec3 position, float pitch, float yaw);

	glm::mat4 getViewMatrix() const;
	glm::mat4 getProjectionMatrix(float aspect) const;

	void move(glm::vec3 direction);
	void rotate(float deltaYaw, float deltaPitch);

	glm::vec3 getTarget() const { return target; };
	glm::vec3 getPosition() const { return position; };

private:
	glm::vec3 position;
	glm::vec3 target;
	glm::vec3 front, up, right;
	float yaw, pitch;

	void updateCameraVectors();
};