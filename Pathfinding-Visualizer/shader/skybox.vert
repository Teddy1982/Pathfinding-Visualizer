#version 460 core

layout(location = 0) in vec4 aPos;
layout(location = 0) out vec3 texCoord;

layout(set = 0, binding = 0) uniform UBO {
	mat4 proj;	// 64 bytes (4x16)
	mat4 view;	// 64 bytes (4x16)

	vec3 cameraPosition; // 12 bytes
	float _pad1; // 4 bytes padding (ensures next vec3 is 16-byte aligned)

	vec3 cameraTarget;	// 12 bytes
	float _pad2; // 4 bytes padding (ensures next float is correctly placed)
} ubo;

void main() {
    texCoord = aPos.xyz;

    mat4 viewNoTranslation = mat4(mat3(ubo.view));
    vec4 pos = ubo.proj * viewNoTranslation * aPos;

    gl_Position = pos.xyww;
}