#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inId;

layout(location = 0) out vec3 fragColor;


layout(set = 0, binding = 0) uniform UBO {
	mat4 proj;	// 64 bytes (4x16)
	mat4 view;	// 64 bytes (4x16)

	vec3 cameraPosition; // 12 bytes
	float _pad1; // 4 bytes padding (ensures next vec3 is 16-byte aligned)

	vec3 cameraTarget;	// 12 bytes
	float _pad2; // 4 bytes padding (ensures next float is correctly placed)

    vec3 id;
    float _pad3;
} ubo;


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

void main() {
    mat4 modelMatrix = mat4(1.0);
    vec4 worldPos = modelMatrix * vec4(inPosition, 1.0);

    gl_Position = ubo.proj * ubo.view * worldPos;

    vec3 color = vec3(0.0, 0.0, 0.0);
    
    if(ubo.id.xyz == inId.xyz) {
        color = vec3(1.0, 1.0, 1.0);
    }

    fragColor = color;
}