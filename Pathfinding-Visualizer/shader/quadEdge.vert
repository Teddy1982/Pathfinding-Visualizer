#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in ivec3 inId;

layout(location = 0) out vec3 fragColor;

layout(set = 0, binding = 0) uniform UBO {
    mat4 proj;
    mat4 view;

    vec3 cameraPosition;
    float _pad1;

    vec3 cameraTarget;
    float _pad2;

    vec3 id;
    float _pad3;
} ubo;

void main()
{
    gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);

    // Standardfarbe der Grid-Kanten: schwarz
    fragColor = vec3(0.0, 0.0, 0.0);

    // Die Kanten der aktuell ausgewählten Zelle hervorheben
    ivec3 selectedId = ivec3(ubo.id);

    if (selectedId == inId) {
        fragColor = vec3(1.0, 1.0, 1.0);
    }
}