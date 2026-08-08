#version 450
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragPosition;
layout(location = 2) flat in ivec3 fragId;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor.xyz, 1.0);
}