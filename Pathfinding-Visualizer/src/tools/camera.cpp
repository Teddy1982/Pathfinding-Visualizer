#include "camera.h"

Camera* camera= new Camera(glm::vec3(0, 1.8f, 3), 0.0f, 90.0f);

Camera::Camera(glm::vec3 position, float pitch, float yaw) : position(position), pitch(pitch), yaw(yaw)
{
	updateCameraVectors();
}

// gibt die Matrix der Kamerasicht zurück
glm::mat4 Camera::getViewMatrix() const {
	return glm::lookAt(position, position + front, up);
}

// gibt die Matrix der gewählten Projektion zurück
glm::mat4 Camera::getProjectionMatrix(float aspect) const {
	return glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
}

// Funktion für Änderungen bei Kamerabewegungen
void Camera::move(glm::vec3 direction) {
	glm::vec3 forward = glm::normalize(front);
	glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
	glm::vec3 up = glm::normalize(glm::cross(right, forward));

	position += forward * direction.z;
	position += right * direction.x;
	position += up * (direction.y * -1.0f);
}

// Funktion für Änderungen bei Kamerarotationen
void Camera::rotate(float deltaYaw, float deltaPitch) {
	yaw += deltaYaw;
	pitch += deltaPitch;
	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;
	updateCameraVectors();
}

// Funktion für Aktualisierungen des Kameravektors
void Camera::updateCameraVectors() {
	glm::vec3 newFront;
	newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	newFront.y = sin(glm::radians(pitch));
	newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(newFront);
	right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
	up = glm::normalize(glm::cross(-right, front));
}
