#version 460

//shader input
layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;

//output write
layout (location = 0) out vec4 outFragColor;

void main() {
    vec3 normal = inColor;
    vec3 lightDir = normalize(vec3(-1, -1, -1));
    float d = max(dot(normal, -lightDir), 0.0f);

    vec3 albedo = vec3(1.0, 0.0, 1.0);
    outFragColor = vec4(inColor, 1.0f);
}
