// GLSL shader version 4.50 (for Vulkan)
#version 450 core

// Vertex attributes
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

// Uniform buffer for transformation matrices
layout(std140, binding = 0) uniform Matrices {
    mat4 model;
    mat4 view;
    mat4 projection;
};

// Vertex output to the fragment shader
layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPosition;

out gl_PerVertex {
    vec4 gl_Position;
};

// Vertex shader main function
void main() {
    vec4 worldPos = model * vec4(position, 1.0);
    gl_Position = projection * view * worldPos;
    
    // Transform normal to world space
    fragNormal = mat3(transpose(inverse(model))) * normal;
    fragTexCoord = texCoord;
    fragPosition = worldPos.xyz;
}
