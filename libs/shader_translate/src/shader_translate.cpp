/**
 * @file shader_translate.cpp
 * @brief Implementation of cross-platform shader translation library
 */

#include "shader_translate/shader_translate.h"

#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/SPIRV/SpvTools.h>

#include "spirv_cross/spirv.hpp"
#include "spirv_cross/spirv_glsl.hpp"
#include "spirv_cross/spirv_hlsl.hpp"
#include "spirv_cross/spirv_msl.hpp"

#include <atomic>
#include <sstream>
#include <iomanip>

namespace shader_translate {

// Global initialization state
static std::atomic<bool> g_initialized{ false };

/**
 * @brief Initialize default resource limits for glslang
 */
static TBuiltInResource InitDefaultResources() {
    TBuiltInResource Resources{};

    Resources.maxLights = 32;
    Resources.maxClipPlanes = 6;
    Resources.maxTextureUnits = 32;
    Resources.maxTextureCoords = 32;
    Resources.maxVertexAttribs = 64;
    Resources.maxVertexUniformComponents = 4096;
    Resources.maxVaryingFloats = 64;
    Resources.maxVertexTextureImageUnits = 32;
    Resources.maxCombinedTextureImageUnits = 80;
    Resources.maxTextureImageUnits = 32;
    Resources.maxFragmentUniformComponents = 4096;
    Resources.maxDrawBuffers = 32;
    Resources.maxVertexUniformVectors = 128;
    Resources.maxVaryingVectors = 8;
    Resources.maxFragmentUniformVectors = 16;
    Resources.maxVertexOutputVectors = 16;
    Resources.maxFragmentInputVectors = 15;
    Resources.minProgramTexelOffset = -8;
    Resources.maxProgramTexelOffset = 7;
    Resources.maxClipDistances = 8;
    Resources.maxComputeWorkGroupCountX = 65535;
    Resources.maxComputeWorkGroupCountY = 65535;
    Resources.maxComputeWorkGroupCountZ = 65535;
    Resources.maxComputeWorkGroupSizeX = 1024;
    Resources.maxComputeWorkGroupSizeY = 1024;
    Resources.maxComputeWorkGroupSizeZ = 64;
    Resources.maxComputeUniformComponents = 1024;
    Resources.maxComputeTextureImageUnits = 16;
    Resources.maxComputeImageUniforms = 8;
    Resources.maxComputeAtomicCounters = 8;
    Resources.maxComputeAtomicCounterBuffers = 1;
    Resources.maxVaryingComponents = 60;
    Resources.maxVertexOutputComponents = 64;
    Resources.maxGeometryInputComponents = 64;
    Resources.maxGeometryOutputComponents = 128;
    Resources.maxFragmentInputComponents = 128;
    Resources.maxImageUnits = 8;
    Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
    Resources.maxCombinedShaderOutputResources = 8;
    Resources.maxImageSamples = 0;
    Resources.maxVertexImageUniforms = 0;
    Resources.maxTessControlImageUniforms = 0;
    Resources.maxTessEvaluationImageUniforms = 0;
    Resources.maxGeometryImageUniforms = 0;
    Resources.maxFragmentImageUniforms = 8;
    Resources.maxCombinedImageUniforms = 8;
    Resources.maxGeometryTextureImageUnits = 16;
    Resources.maxGeometryOutputVertices = 256;
    Resources.maxGeometryTotalOutputComponents = 1024;
    Resources.maxGeometryUniformComponents = 1024;
    Resources.maxGeometryVaryingComponents = 64;
    Resources.maxTessControlInputComponents = 128;
    Resources.maxTessControlOutputComponents = 128;
    Resources.maxTessControlTextureImageUnits = 16;
    Resources.maxTessControlUniformComponents = 1024;
    Resources.maxTessControlTotalOutputComponents = 4096;
    Resources.maxTessEvaluationInputComponents = 128;
    Resources.maxTessEvaluationOutputComponents = 128;
    Resources.maxTessEvaluationTextureImageUnits = 16;
    Resources.maxTessEvaluationUniformComponents = 1024;
    Resources.maxTessPatchComponents = 120;
    Resources.maxPatchVertices = 32;
    Resources.maxTessGenLevel = 64;
    Resources.maxViewports = 16;
    Resources.maxVertexAtomicCounters = 0;
    Resources.maxTessControlAtomicCounters = 0;
    Resources.maxTessEvaluationAtomicCounters = 0;
    Resources.maxGeometryAtomicCounters = 0;
    Resources.maxFragmentAtomicCounters = 8;
    Resources.maxCombinedAtomicCounters = 8;
    Resources.maxAtomicCounterBindings = 1;
    Resources.maxVertexAtomicCounterBuffers = 0;
    Resources.maxTessControlAtomicCounterBuffers = 0;
    Resources.maxTessEvaluationAtomicCounterBuffers = 0;
    Resources.maxGeometryAtomicCounterBuffers = 0;
    Resources.maxFragmentAtomicCounterBuffers = 1;
    Resources.maxCombinedAtomicCounterBuffers = 1;
    Resources.maxAtomicCounterBufferSize = 16384;
    Resources.maxTransformFeedbackBuffers = 4;
    Resources.maxTransformFeedbackInterleavedComponents = 64;
    Resources.maxCullDistances = 8;
    Resources.maxCombinedClipAndCullDistances = 8;
    Resources.maxSamples = 4;
    Resources.maxMeshOutputVerticesNV = 256;
    Resources.maxMeshOutputPrimitivesNV = 512;
    Resources.maxMeshWorkGroupSizeX_NV = 32;
    Resources.maxMeshWorkGroupSizeY_NV = 1;
    Resources.maxMeshWorkGroupSizeZ_NV = 1;
    Resources.maxTaskWorkGroupSizeX_NV = 32;
    Resources.maxTaskWorkGroupSizeY_NV = 1;
    Resources.maxTaskWorkGroupSizeZ_NV = 1;
    Resources.maxMeshViewCountNV = 4;

    Resources.limits.nonInductiveForLoops = 1;
    Resources.limits.whileLoops = 1;
    Resources.limits.doWhileLoops = 1;
    Resources.limits.generalUniformIndexing = 1;
    Resources.limits.generalAttributeMatrixVectorIndexing = 1;
    Resources.limits.generalVaryingIndexing = 1;
    Resources.limits.generalSamplerIndexing = 1;
    Resources.limits.generalVariableIndexing = 1;
    Resources.limits.generalConstantMatrixVectorIndexing = 1;

    return Resources;
}

/**
 * @brief Convert ShaderType to glslang EShLanguage
 */
static EShLanguage shader_type_to_glslang(ShaderType type) {
    switch (type) {
        case ShaderType::Vertex:
            return EShLangVertex;
        case ShaderType::Fragment:
            return EShLangFragment;
        case ShaderType::Compute:
            return EShLangCompute;
        case ShaderType::Geometry:
            return EShLangGeometry;
        case ShaderType::TessControl:
            return EShLangTessControl;
        case ShaderType::TessEvaluation:
            return EShLangTessEvaluation;
        default:
            return EShLangVertex;
    }
}

bool initialize() {
    bool expected = false;
    if (g_initialized.compare_exchange_strong(expected, true)) {
        glslang::InitializeProcess();
        return true;
    }
    return true; // Already initialized
}

void finalize() {
    bool expected = true;
    if (g_initialized.compare_exchange_strong(expected, false)) {
        glslang::FinalizeProcess();
    }
}

CompiledShader compile_to_spirv(const std::string& source, ShaderType type, const ShaderOptions& options) {
    CompiledShader result;
    result.type = type;
    result.target = TargetLanguage::SPIRV;

    if (!g_initialized) {
        result.error_message = "Library not initialized. Call initialize() first.";
        return result;
    }

    EShLanguage lang = shader_type_to_glslang(type);
    glslang::TShader shader(lang);

    const char* sourcePtr = source.c_str();
    shader.setStrings(&sourcePtr, 1);

    shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, 100);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
    shader.setTextureSamplerTransformMode(EShTextureSamplerTransformMode::EShTexSampTransKeep);

    TBuiltInResource resources = InitDefaultResources();

    if (!shader.parse(&resources, 120, true, EShMsgDefault)) {
        result.error_message = shader.getInfoLog();
        return result;
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(EShMsgDefault)) {
        result.error_message = program.getInfoLog();
        return result;
    }

    auto* intermediate = program.getIntermediate(lang);
    if (!intermediate) {
        result.error_message = "Failed to get intermediate representation";
        return result;
    }

    spv::SpvBuildLogger logger;
    std::vector<uint32_t> spirv;

    glslang::SpvOptions spvOptions;
    spvOptions.validate = options.spirv_validate;
    spvOptions.disableOptimizer = !options.spirv_optimize;
    spvOptions.optimizeSize = false;

    glslang::GlslangToSpv(*intermediate, spirv, &logger, &spvOptions);

    std::string logMessages = logger.getAllMessages();
    if (!logMessages.empty() && logMessages.find("error") != std::string::npos) {
        result.error_message = logMessages;
        return result;
    }

    result.data = spirv;
    result.success = true;
    return result;
}

CompiledShader cross_compile(const std::vector<uint32_t>& spirv, ShaderType type, TargetLanguage target,
                             const ShaderOptions& options) {
    CompiledShader result;
    result.type = type;
    result.target = target;

    try {
        switch (target) {
            case TargetLanguage::SPIRV: {
                result.data = spirv;
                result.success = true;
                break;
            }

            case TargetLanguage::GLSL: {
                spirv_cross::CompilerGLSL compiler(spirv);
                spirv_cross::CompilerGLSL::Options glslOptions;
                glslOptions.version = options.glsl_version;
                glslOptions.es = false;
                glslOptions.enable_420pack_extension = options.enable_420pack;
                compiler.set_common_options(glslOptions);

                // Handle combined samplers for OpenGL
                compiler.build_combined_image_samplers();
                auto& samplers = compiler.get_combined_image_samplers();
                for (const auto& sampler : samplers) {
                    compiler.set_name(sampler.combined_id, compiler.get_name(sampler.image_id));
                    if (compiler.has_decoration(sampler.image_id, spv::DecorationDescriptorSet)) {
                        uint32_t set = compiler.get_decoration(sampler.image_id, spv::DecorationDescriptorSet);
                        compiler.set_decoration(sampler.combined_id, spv::DecorationDescriptorSet, set);
                    }
                    if (compiler.has_decoration(sampler.image_id, spv::DecorationBinding)) {
                        uint32_t binding = compiler.get_decoration(sampler.image_id, spv::DecorationBinding);
                        compiler.set_decoration(sampler.combined_id, spv::DecorationBinding, binding);
                    }
                }

                result.data = compiler.compile();
                result.success = true;
                break;
            }

            case TargetLanguage::GLSL_ES: {
                spirv_cross::CompilerGLSL compiler(spirv);
                spirv_cross::CompilerGLSL::Options glslOptions;
                glslOptions.version = options.glsl_es_version;
                glslOptions.es = true;
                glslOptions.enable_420pack_extension = false;
                compiler.set_common_options(glslOptions);

                compiler.build_combined_image_samplers();
                auto& samplers = compiler.get_combined_image_samplers();
                for (const auto& sampler : samplers) {
                    compiler.set_name(sampler.combined_id, compiler.get_name(sampler.image_id));
                }

                result.data = compiler.compile();
                result.success = true;
                break;
            }

            case TargetLanguage::HLSL: {
                spirv_cross::CompilerHLSL compiler(spirv);
                spirv_cross::CompilerHLSL::Options hlslOptions;
                hlslOptions.shader_model = options.hlsl_shader_model;
                compiler.set_hlsl_options(hlslOptions);

                result.data = compiler.compile();
                result.success = true;
                break;
            }

            case TargetLanguage::Metal: {
                spirv_cross::CompilerMSL compiler(spirv);
                spirv_cross::CompilerMSL::Options mslOptions;
                mslOptions.set_msl_version(options.metal_version / 10000, (options.metal_version / 100) % 100,
                                           options.metal_version % 100);
                mslOptions.enable_decoration_binding = options.metal_decoration_binding;
                compiler.set_msl_options(mslOptions);

                result.data = compiler.compile();
                result.success = true;
                break;
            }
        }
    } catch (const spirv_cross::CompilerError& e) { result.error_message = e.what(); } catch (const std::exception& e) {
        result.error_message = e.what();
    }

    return result;
}

CompiledShader compile(const std::string& source, ShaderType type, TargetLanguage target,
                       const ShaderOptions& options) {
    // First compile to SPIR-V
    CompiledShader spirvResult = compile_to_spirv(source, type, options);
    if (!spirvResult.success) {
        spirvResult.target = target;
        return spirvResult;
    }

    // If target is SPIR-V, we're done
    if (target == TargetLanguage::SPIRV) {
        return spirvResult;
    }

    // Cross-compile to target
    const auto& spirv = std::get<std::vector<uint32_t>>(spirvResult.data);
    return cross_compile(spirv, type, target, options);
}

std::string generate_c_header(const CompiledShader& vertex_shader, const CompiledShader& fragment_shader,
                              const std::string& prefix, bool include_spirv) {
    std::ostringstream header;

    header << "// Auto-generated shader header\n";
    header << "// Generated by shader_translate library\n";
    header << "// Do not edit manually!\n\n";
    header << "#pragma once\n\n";
    header << "#ifndef " << prefix << "SHADERS_H\n";
    header << "#define " << prefix << "SHADERS_H\n\n";
    header << "#include <cstdint>\n\n";

    auto write_string_shader = [&](const std::string& name, const CompiledShader& shader) {
        if (!shader.success || !std::holds_alternative<std::string>(shader.data)) {
            return;
        }
        const auto& code = std::get<std::string>(shader.data);
        header << "static const char* " << prefix << name << " = R\"(\n";
        header << code;
        header << ")\";\n\n";
    };

    auto write_binary_shader = [&](const std::string& name, const CompiledShader& shader) {
        if (!shader.success || !std::holds_alternative<std::vector<uint32_t>>(shader.data)) {
            return;
        }
        const auto& spirv = std::get<std::vector<uint32_t>>(shader.data);
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(spirv.data());
        size_t size = spirv.size() * sizeof(uint32_t);

        header << "static const unsigned char " << prefix << name << "[] = {\n";
        for (size_t i = 0; i < size; i++) {
            if (i % 12 == 0)
                header << "    ";
            header << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
            if (i < size - 1)
                header << ", ";
            if ((i + 1) % 12 == 0)
                header << "\n";
        }
        header << std::dec;
        if (size % 12 != 0)
            header << "\n";
        header << "};\n";
        header << "static const size_t " << prefix << name << "_Size = " << size << ";\n\n";
    };

    // Detect shader language and generate appropriate output
    std::string langSuffix;
    switch (vertex_shader.target) {
        case TargetLanguage::SPIRV:
            langSuffix = "SPIRV";
            break;
        case TargetLanguage::GLSL:
            langSuffix = "GLSL";
            break;
        case TargetLanguage::GLSL_ES:
            langSuffix = "GLSL_ES";
            break;
        case TargetLanguage::HLSL:
            langSuffix = "HLSL";
            break;
        case TargetLanguage::Metal:
            langSuffix = "Metal";
            break;
    }

    if (vertex_shader.target == TargetLanguage::SPIRV) {
        write_binary_shader("VertexShader_" + langSuffix, vertex_shader);
        write_binary_shader("FragmentShader_" + langSuffix, fragment_shader);
    } else {
        write_string_shader("VertexShader_" + langSuffix, vertex_shader);
        write_string_shader("FragmentShader_" + langSuffix, fragment_shader);
    }

    header << "#endif // " << prefix << "SHADERS_H\n";

    return header.str();
}

const char* target_language_name(TargetLanguage target) {
    switch (target) {
        case TargetLanguage::SPIRV:
            return "SPIR-V";
        case TargetLanguage::GLSL:
            return "GLSL";
        case TargetLanguage::GLSL_ES:
            return "GLSL ES";
        case TargetLanguage::HLSL:
            return "HLSL";
        case TargetLanguage::Metal:
            return "Metal";
        default:
            return "Unknown";
    }
}

const char* shader_type_name(ShaderType type) {
    switch (type) {
        case ShaderType::Vertex:
            return "Vertex";
        case ShaderType::Fragment:
            return "Fragment";
        case ShaderType::Compute:
            return "Compute";
        case ShaderType::Geometry:
            return "Geometry";
        case ShaderType::TessControl:
            return "TessControl";
        case ShaderType::TessEvaluation:
            return "TessEvaluation";
        default:
            return "Unknown";
    }
}

} // namespace shader_translate
