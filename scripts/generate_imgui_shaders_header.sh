#!/bin/bash
#
# Script to generate ImGui shaders header using shader_translate CLI
#
# Usage: ./generate_imgui_shaders_header.sh [output_file]
#
# Requires: shader_translate CLI (built from libs/shader_translate)
#

set -e

# Default output file
OUTPUT_FILE="${1:-src/imgui_shaders.h}"

# Find shader_translate CLI
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Look for shader_translate in common build locations
SHADER_TRANSLATE=""
for path in \
    "$PROJECT_DIR/build/libs/shader_translate/shader_translate_cli" \
    "$PROJECT_DIR/cmake-build-debug/libs/shader_translate/shader_translate_cli" \
    "$PROJECT_DIR/cmake-build-release/libs/shader_translate/shader_translate_cli" \
    "$(which shader_translate_cli 2>/dev/null || true)"
do
    if [[ -x "$path" ]]; then
        SHADER_TRANSLATE="$path"
        break
    fi
done

if [[ -z "$SHADER_TRANSLATE" ]]; then
    echo "Error: shader_translate CLI not found."
    echo "Please build the project first: cd build && cmake --build ."
    exit 1
fi

echo "Using shader_translate: $SHADER_TRANSLATE"

# Create temporary shader files
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

# Create ImGui vertex shader (GLSL 450)
cat > "$TEMP_DIR/imgui.vert" << 'VERT_EOF'
#version 450 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

layout(std140, binding = 0) uniform Matrices {
    mat4 ProjectionMatrix;
};

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec4 vColor;

void main() {
    vUV = aUV;
    vColor = aColor;
    gl_Position = ProjectionMatrix * vec4(aPos, 0.0, 1.0);
}
VERT_EOF

# Create ImGui fragment shader (GLSL 450)
cat > "$TEMP_DIR/imgui.frag" << 'FRAG_EOF'
#version 450 core

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vColor;

layout(binding = 1) uniform sampler2D colorMap;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vColor * texture(colorMap, vUV);
}
FRAG_EOF

echo "Generating ImGui shaders header..."

# Run shader_translate with all targets
"$SHADER_TRANSLATE" \
    --all \
    --prefix "g_ImGui" \
    --no-420pack \
    -v \
    -o "$OUTPUT_FILE" \
    "$TEMP_DIR/imgui.vert" \
    "$TEMP_DIR/imgui.frag"

echo ""
echo "✓ Generated $OUTPUT_FILE successfully!"
echo ""
echo "Shaders included:"
echo "  - SPIR-V binary arrays"
echo "  - OpenGL GLSL 4.1+ (cross-compiled)"
echo "  - OpenGL ES GLSL 3.0 (cross-compiled)"
echo "  - HLSL SM 5.0 (cross-compiled)"
echo "  - Metal 2.1 (cross-compiled)"
