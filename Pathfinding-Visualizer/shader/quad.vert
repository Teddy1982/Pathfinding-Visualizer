#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in ivec3 inId;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragPosition;
layout(location = 2) flat out ivec3 fragId;

layout(set = 0, binding = 0) uniform UBO {
    mat4 proj;
    mat4 view;

    vec3 cameraPosition;
    float xSize;

    vec3 cameraTarget;
    float ySize;
} ubo;

struct CubeState {
    vec4 color;
    vec4 scale;
};

layout(std430, set = 0, binding = 1) readonly buffer CubeStateBuffer {
    CubeState states[];
} csb;

uint toIndex(ivec3 id) {
    uint gridSizeX = uint(round(ubo.xSize));
    return uint(id.x) + uint(id.y) * gridSizeX;
}

void main()
{
    uint nodeIndex = toIndex(inId);

    vec3 scale = csb.states[nodeIndex].scale.xyz;

    // Mittelpunkt der jeweiligen Grid-Zelle
    vec3 center = vec3(
        float(inId.x),
        float(inId.y),
        0.0
    );

    // Quad um seinen Mittelpunkt skalieren
    vec3 scaledPosition =
        center + (inPosition - center) * scale;

    // Z konstant halten
    scaledPosition.z = 0.0;

    vec4 worldPos = vec4(scaledPosition, 1.0);

    gl_Position = ubo.proj * ubo.view * worldPos;
    fragColor = csb.states[nodeIndex].color;
    fragPosition = inPosition;
    fragId = inId;
}