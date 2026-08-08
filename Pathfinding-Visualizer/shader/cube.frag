#version 450
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPosition;
layout(location = 3) flat in ivec3 fragId;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 baseColor = fragColor.xyz;
    
    vec3 N = normalize(fragNormal);

    // Testweise feste Lichtposition
    vec3 lightPos = vec3(0.0, 100.0, -100.0);
    vec3 L = normalize(lightPos - fragPosition);

    float diff = max(dot(N, L), 0.0);

    float ambientStrength = 0.25;
    vec3 ambient = ambientStrength * baseColor.xyz;
    vec3 diffuse = diff * baseColor.xyz;

    vec3 result = ambient + diffuse;

    outColor = vec4(result, 1.0);
}