#version 460

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform sampler2D albedoTex;

void main() {
    vec3 normal = inColor;
    vec3 lightDir = normalize(vec3(1, -1, 1));
    float d = max(dot(normal, -lightDir), 0.0f);

    vec4 albedo = texture(albedoTex, inUV);
    outFragColor = albedo * d;
}
