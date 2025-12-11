/**
 * @file shader_translate.h
 * @brief Cross-platform shader translation library
 *
 * This library provides functions to compile GLSL shaders to SPIR-V
 * and cross-compile them to various target languages (HLSL, Metal, GLSL/ES).
 *
 * Independent of LLGL - can be used standalone.
 */

#ifndef SHADER_TRANSLATE_H
#define SHADER_TRANSLATE_H

#include <string>
#include <vector>
#include <cstdint>
#include <variant>

namespace shader_translate {

/**
 * @brief Shader type enumeration
 */
enum class ShaderType { Vertex, Fragment, Compute, Geometry, TessControl, TessEvaluation };

/**
 * @brief Target shading language for cross-compilation
 */
enum class TargetLanguage {
    SPIRV,   ///< SPIR-V binary
    GLSL,    ///< OpenGL GLSL (desktop)
    GLSL_ES, ///< OpenGL ES GLSL
    HLSL,    ///< DirectX HLSL
    Metal    ///< Apple Metal Shading Language
};

/**
 * @brief Options for shader compilation and cross-compilation
 */
struct ShaderOptions {
    // GLSL options
    int glsl_version = 410;      ///< Target GLSL version (e.g., 410, 450)
    bool glsl_es = false;        ///< Target GLSL ES
    int glsl_es_version = 300;   ///< Target GLSL ES version (e.g., 300, 310)
    bool enable_420pack = false; ///< Enable GL_ARB_shading_language_420pack

    // HLSL options
    int hlsl_shader_model = 50; ///< HLSL shader model (e.g., 50 for SM5.0)

    // Metal options
    int metal_version = 20100;            ///< Metal version (e.g., 20100 for 2.1)
    bool metal_decoration_binding = true; ///< Preserve GLSL binding decorations in Metal

    // SPIR-V options
    bool spirv_validate = false; ///< Validate SPIR-V output
    bool spirv_optimize = false; ///< Optimize SPIR-V

    // General options
    bool debug_info = false; ///< Include debug info
};

/**
 * @brief Result of shader compilation
 */
struct CompiledShader {
    std::variant<std::string, std::vector<uint32_t>> data; ///< Shader code or binary
    std::string error_message;                             ///< Error message if failed
    bool success = false;                                  ///< Whether compilation succeeded
    TargetLanguage target;                                 ///< Target language
    ShaderType type;                                       ///< Shader type
};

/**
 * @brief Initialize the shader translation library
 * @return true if initialization succeeded
 *
 * Must be called before any other functions.
 * Thread-safe for initialization.
 */
bool initialize();

/**
 * @brief Finalize the shader translation library
 *
 * Call when done with all shader operations.
 */
void finalize();

/**
 * @brief Compile GLSL source to SPIR-V
 *
 * @param source GLSL source code (Vulkan-style, version 450)
 * @param type Shader type (vertex, fragment, etc.)
 * @param options Compilation options
 * @return CompiledShader with SPIR-V binary or error
 */
CompiledShader compile_to_spirv(const std::string& source, ShaderType type, const ShaderOptions& options = {});

/**
 * @brief Cross-compile SPIR-V to target language
 *
 * @param spirv SPIR-V binary
 * @param type Shader type
 * @param target Target language
 * @param options Cross-compilation options
 * @return CompiledShader with target code or error
 */
CompiledShader cross_compile(const std::vector<uint32_t>& spirv, ShaderType type, TargetLanguage target,
                             const ShaderOptions& options = {});

/**
 * @brief Compile GLSL directly to target language
 *
 * Convenience function that combines compile_to_spirv and cross_compile.
 *
 * @param source GLSL source code
 * @param type Shader type
 * @param target Target language
 * @param options Compilation options
 * @return CompiledShader with target code or error
 */
CompiledShader compile(const std::string& source, ShaderType type, TargetLanguage target,
                       const ShaderOptions& options = {});

/**
 * @brief Generate C/C++ header with embedded shaders
 *
 * @param vertex_shader Compiled vertex shader
 * @param fragment_shader Compiled fragment shader
 * @param prefix Variable name prefix (e.g., "g_ImGui")
 * @param include_spirv Whether to include SPIR-V binary arrays
 * @return Generated header content as string
 */
std::string generate_c_header(const CompiledShader& vertex_shader, const CompiledShader& fragment_shader,
                              const std::string& prefix = "g_", bool include_spirv = true);

/**
 * @brief Get target language name as string
 */
const char* target_language_name(TargetLanguage target);

/**
 * @brief Get shader type name as string
 */
const char* shader_type_name(ShaderType type);

} // namespace shader_translate

#endif // SHADER_TRANSLATE_H
