#version 460 core
layout (location = 0) in vec3 texCoord;

layout (location = 0) out vec4 FragColor;

layout(set = 0, binding = 2) uniform samplerCube skyboxSampler;

void main() {

  FragColor = texture(skyboxSampler, texCoord);
}
