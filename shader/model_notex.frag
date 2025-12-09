// GLSL shader version 4.50 (for Vulkan)
#version 450 core

// Fragment input from the vertex shader
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPosition;

// Fragment output color
layout(location = 0) out vec4 outColor;

// Simple directional light
const vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const vec3 ambientColor = vec3(0.3, 0.3, 0.3);
const vec3 baseColor = vec3(0.8, 0.8, 0.8);

void main() {
    vec3 normal = normalize(fragNormal);

    // Diffuse lighting
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Combine lighting with base color
    vec3 result = (ambientColor + diffuse) * baseColor;
    outColor = vec4(result, 1.0);
}
