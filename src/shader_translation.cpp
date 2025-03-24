#include <filesystem>
#include <fstream>

#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/SpvTools.h>

#include <LLGL/LLGL.h>

#include "spirv.hpp"
#include "spirv_glsl.hpp"
#include "spirv_hlsl.hpp"
#include "spirv_msl.hpp"
#include "shader_translation.h"

static TBuiltInResource InitResources() {
    TBuiltInResource Resources;

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

void glslang_spirv_cross_test() {
    glslang::InitializeProcess();

    glslang::TShader vertShaderGlslang(EShLangVertex);
    glslang::TShader fragShaderGlslang(EShLangFragment);

    std::filesystem::path shaderPath = "../shader";
    std::filesystem::path vertShaderPath = shaderPath / "test.vert";
    std::filesystem::path fragShaderPath = shaderPath / "test.frag";

    std::ifstream vertShaderFile(vertShaderPath);
    std::ifstream fragShaderFile(fragShaderPath);

    std::string vertShaderSource((std::istreambuf_iterator<char>(vertShaderFile)), std::istreambuf_iterator<char>());
    std::string fragShaderSource((std::istreambuf_iterator<char>(fragShaderFile)), std::istreambuf_iterator<char>());

    // vertShaderGlslang.addSourceText(vertShaderSource.c_str(), vertShaderSource.size());
    // fragShaderGlslang.addSourceText(fragShaderSource.c_str(), fragShaderSource.size());

    char* vertShaderSourceC = vertShaderSource.data();
    char* fragShaderSourceC = fragShaderSource.data();

    vertShaderGlslang.setStrings(&vertShaderSourceC, 1);
    fragShaderGlslang.setStrings(&fragShaderSourceC, 1);

    vertShaderGlslang.setSourceFile("../shader/test.vert");
    fragShaderGlslang.setSourceFile("../shader/test.frag");

    vertShaderGlslang.setEnvInput(glslang::EShSourceGlsl, EShLangVertex, glslang::EShClientVulkan, 100);
    fragShaderGlslang.setEnvInput(glslang::EShSourceGlsl, EShLangFragment, glslang::EShClientVulkan, 100);

    vertShaderGlslang.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);
    vertShaderGlslang.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);

    fragShaderGlslang.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);
    fragShaderGlslang.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);

    TBuiltInResource builtInResources = InitResources();
    vertShaderGlslang.parse(&builtInResources, 120, true, EShMsgDefault);
    fragShaderGlslang.parse(&builtInResources, 120, true, EShMsgDefault);

    glslang::TProgram program;
    program.addShader(&vertShaderGlslang);
    program.addShader(&fragShaderGlslang);

    program.link(EShMsgDefault);

    if (program.getIntermediate(EShLangVertex) == nullptr || program.getIntermediate(EShLangFragment) == nullptr) {
        LLGL::Log::Errorf("Failed to link shaders\n");
        exit(1);
    }

    const char* vertShaderGlslangLog = vertShaderGlslang.getInfoLog();
    const char* fragShaderGlslangLog = fragShaderGlslang.getInfoLog();

    if (vertShaderGlslangLog != nullptr && *vertShaderGlslangLog != '\0') {
        LLGL::Log::Printf(vertShaderGlslangLog);
        throw std::runtime_error("Failed to compile vertex shader");
    }

    if (fragShaderGlslangLog != nullptr && *fragShaderGlslangLog != '\0') {
        LLGL::Log::Printf(fragShaderGlslangLog);
        throw std::runtime_error("Failed to compile fragment shader");
    }

    auto vertShaderGlslangIntermediate = program.getIntermediate(EShLangVertex);
    auto fragShaderGlslangIntermediate = program.getIntermediate(EShLangFragment);

    spv::SpvBuildLogger logger;
    std::vector<uint32_t> spirvSourceVert;
    std::vector<uint32_t> spirvSourceFrag;

    glslang::SpvOptions spvOptions;
    spvOptions.validate = false;
    spvOptions.disableOptimizer = true;
    spvOptions.optimizeSize = false;

    glslang::GlslangToSpv(*vertShaderGlslangIntermediate, spirvSourceVert, &logger, &spvOptions);
    glslang::GlslangToSpv(*fragShaderGlslangIntermediate, spirvSourceFrag, &logger, &spvOptions);

    glslang::FinalizeProcess();

    spirv_cross::CompilerGLSL::Options scoptions;
    scoptions.version = 120;
    scoptions.es = false;
    spirv_cross::CompilerGLSL glslVert(spirvSourceVert);
    glslVert.set_common_options(scoptions);
    LLGL::Log::Printf("GLSL:\n%s\n", glslVert.compile().c_str());
    spirv_cross::CompilerGLSL glslFrag(spirvSourceFrag);
    glslFrag.set_common_options(scoptions);
    LLGL::Log::Printf("GLSL:\n%s\n", glslFrag.compile().c_str());

    spirv_cross::CompilerHLSL::Options hlslOptions;
    hlslOptions.shader_model = 500;
    spirv_cross::CompilerHLSL hlslVert(spirvSourceVert);
    hlslVert.set_hlsl_options(hlslOptions);
    LLGL::Log::Printf("HLSL:\n%s\n", hlslVert.compile().c_str());
    spirv_cross::CompilerHLSL hlslFrag(spirvSourceFrag);
    hlslFrag.set_hlsl_options(hlslOptions);
    LLGL::Log::Printf("HLSL:\n%s\n", hlslFrag.compile().c_str());

    spirv_cross::CompilerMSL mslVert(spirvSourceVert);
    LLGL::Log::Printf("MSL:\n%s\n", mslVert.compile().c_str());
    spirv_cross::CompilerMSL mslFrag(spirvSourceFrag);
    LLGL::Log::Printf("MSL:\n%s\n", mslFrag.compile().c_str());
}