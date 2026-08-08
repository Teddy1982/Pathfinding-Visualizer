#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in ivec3 inId;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPosition;
layout(location = 3) flat out ivec3 fragId;


layout(set = 0, binding = 0) uniform UBO {
	mat4 proj;	// 64 bytes (4x16)
	mat4 view;	// 64 bytes (4x16)

	vec3 cameraPosition; // 12 bytes
	float xSize; // 4 bytes padding (ensures next vec3 is 16-byte aligned)

	vec3 cameraTarget;	// 12 bytes
	float ySize; // 4 bytes padding (ensures next float is correctly placed)
} ubo;

uint gridSizeX = uint(round(ubo.xSize));
uint gridSizeY = uint(round(ubo.ySize));

struct CubeState {
    vec4 color;
    vec4 scale;
};

layout(std430, binding = 1) readonly buffer CubeStateBuffer {
    CubeState states[];
} csb;

mat4 lookAt(vec3 eye, vec3 center, vec3 up) {
    vec3 f = normalize(center - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);

    return mat4(
        vec4(s,  0.0),
        vec4(u,  0.0),
        vec4(-f, 0.0),
        vec4(-dot(s, eye), -dot(u, eye), dot(f, eye), 1.0)
    );
}

mat4 perspective(float fovy, float aspect, float near, float far) {
    float tanHalfFovy = tan(fovy * 0.5);

    return mat4(
        vec4(1.0 / (aspect * tanHalfFovy), 0.0, 0.0, 0.0),
        vec4(0.0, 1.0 / tanHalfFovy, 0.0, 0.0),
        vec4(0.0, 0.0, far / (near - far), -1.0),
        vec4(0.0, 0.0, -(far * near) / (far - near), 0.0)
    );
}

uint toIndex(uvec3 id) {
    return id.x + id.y * gridSizeX + id.z * gridSizeX * gridSizeY;
}

mat4 makeScale(vec3 s) {
    return mat4(
        vec4(s.x, 0.0, 0.0, 0.0),
        vec4(0.0, s.y, 0.0, 0.0),
        vec4(0.0, 0.0, s.z, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );
}

void main() {
    uint cubeId = toIndex(inId);

    vec3 scale = csb.states[cubeId].scale.xyz;

    // Mittelpunkt des Würfels
    vec3 center = vec3(inId);

    // Skalierung um den Mittelpunkt
    vec3 scaledPosition = center + (inPosition - center) * scale;

    vec4 worldPos = vec4(scaledPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;

    fragColor = csb.states[cubeId].color;
    fragNormal = normalize(inNormal);
    fragPosition = worldPos.xyz;
    fragId = inId;
}