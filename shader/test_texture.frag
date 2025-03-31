// GLSL shader version 4.50 (for Vulkan)
#version 450 core

layout(binding = 1) uniform texture2D colorMap;
layout(binding = 2) uniform sampler samplerState;

// Fragment input from the vertex shader
layout(location = 0) in vec3 vertexColor;
layout(location = 1) in vec2 vTexCoord;

// Fragment output color
layout(location = 0) out vec4 fragColor;

// Fragment shader main function
void main() {
    vec4 color = texture(sampler2D(colorMap, samplerState), vTexCoord);
    fragColor = vec4(vertexColor, 1) * color;
}