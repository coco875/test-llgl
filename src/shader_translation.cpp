#include <filesystem>
#include <fstream>
#include <variant>

#include <shader_translate/shader_translate.h>

#include <LLGL/LLGL.h>
#include <LLGL/Utils/VertexFormat.h>
#include "shader_translation.h"

extern LLGL::RenderSystemPtr llgl_renderer;

namespace {
bool g_initialized = false;

void ensure_initialized() {
    if (!g_initialized) {
        shader_translate::initialize();
        g_initialized = true;
    }
}
} // namespace

void glslang_spirv_cross_test() {
    ensure_initialized();

    std::filesystem::path shaderPath = "../shader";
    std::filesystem::path vertShaderPath = shaderPath / "test.vert";
    std::filesystem::path fragShaderPath = shaderPath / "test.frag";

    std::string vertShaderSource;
    std::string fragShaderSource;
    std::ifstream shaderVertFile(vertShaderPath);
    if (!shaderVertFile.is_open()) {
        LLGL::Log::Printf("Failed to open shader file");
        throw std::runtime_error("Failed to open shader file");
    }
    vertShaderSource = std::string((std::istreambuf_iterator<char>(shaderVertFile)), std::istreambuf_iterator<char>());

    std::ifstream shaderFragFile(fragShaderPath);
    if (!shaderFragFile.is_open()) {
        LLGL::Log::Printf("Failed to open shader file");
        throw std::runtime_error("Failed to open shader file");
    }
    fragShaderSource = std::string((std::istreambuf_iterator<char>(shaderFragFile)), std::istreambuf_iterator<char>());

    shader_translate::ShaderOptions options;
    options.glsl_version = 120;

    // Test GLSL
    auto glslVert = shader_translate::compile(vertShaderSource, shader_translate::ShaderType::Vertex,
                                              shader_translate::TargetLanguage::GLSL, options);
    if (glslVert.success) {
        LLGL::Log::Printf("GLSL:\n%s\n", std::get<std::string>(glslVert.data).c_str());
    }

    auto glslFrag = shader_translate::compile(fragShaderSource, shader_translate::ShaderType::Fragment,
                                              shader_translate::TargetLanguage::GLSL, options);
    if (glslFrag.success) {
        LLGL::Log::Printf("GLSL:\n%s\n", std::get<std::string>(glslFrag.data).c_str());
    }

    // Test HLSL
    options.hlsl_shader_model = 50;
    auto hlslVert = shader_translate::compile(vertShaderSource, shader_translate::ShaderType::Vertex,
                                              shader_translate::TargetLanguage::HLSL, options);
    if (hlslVert.success) {
        LLGL::Log::Printf("HLSL:\n%s\n", std::get<std::string>(hlslVert.data).c_str());
    }

    auto hlslFrag = shader_translate::compile(fragShaderSource, shader_translate::ShaderType::Fragment,
                                              shader_translate::TargetLanguage::HLSL, options);
    if (hlslFrag.success) {
        LLGL::Log::Printf("HLSL:\n%s\n", std::get<std::string>(hlslFrag.data).c_str());
    }

    // Test Metal
    auto mslVert = shader_translate::compile(vertShaderSource, shader_translate::ShaderType::Vertex,
                                             shader_translate::TargetLanguage::Metal, options);
    if (mslVert.success) {
        LLGL::Log::Printf("MSL:\n%s\n", std::get<std::string>(mslVert.data).c_str());
    }

    auto mslFrag = shader_translate::compile(fragShaderSource, shader_translate::ShaderType::Fragment,
                                             shader_translate::TargetLanguage::Metal, options);
    if (mslFrag.success) {
        LLGL::Log::Printf("MSL:\n%s\n", std::get<std::string>(mslFrag.data).c_str());
    }
}

bool is_glsl(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    bool isGLSL = false;

    version = 0;

    for (const auto& language : languages) {
        if (language == LLGL::ShadingLanguage::GLSL) {
            isGLSL = true;
        } else if (static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::GLSL)) {
            int v = static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            if (v > version) {
                version = v;
            }
        }
    }
    return isGLSL;
}

bool is_glsles(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    bool isGLSLES = false;

    version = 0;

    for (const auto& language : languages) {
        if (language == LLGL::ShadingLanguage::ESSL) {
            isGLSLES = true;
        } else if (static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::ESSL)) {
            int v = static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            if (v > version) {
                version = v;
            }
        }
    }
    return isGLSLES;
}

bool is_hlsl(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    bool isHLSL = false;

    version = 0;

    for (const auto& language : languages) {
        if (language == LLGL::ShadingLanguage::HLSL) {
            isHLSL = true;
        } else if (static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::HLSL)) {
            int v = static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            if (v > version) {
                version = v;
            }
        }
    }
    return isHLSL;
}

bool is_metal(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    bool isMetal = false;

    version = 0;

    for (const auto& language : languages) {
        if (language == LLGL::ShadingLanguage::Metal) {
            isMetal = true;
        } else if (static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::Metal)) {
            int v = static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            if (v > version) {
                version = v;
            }
        }
    }
    return isMetal;
}

bool is_spirv(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    bool isSPIRV = false;

    version = 0;

    for (const auto& language : languages) {
        if (language == LLGL::ShadingLanguage::SPIRV) {
            isSPIRV = true;
        } else if (static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::SPIRV)) {
            int v = static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            if (v > version) {
                version = v;
            }
        }
    }
    return isSPIRV;
}

void generate_shader_from_string(LLGL::ShaderDescriptor& vertShaderDesc, LLGL::ShaderDescriptor& fragShaderDesc,
                                 const std::vector<LLGL::ShadingLanguage>& languages, LLGL::VertexFormat& vertexFormat,
                                 std::string vertShaderSource, std::string fragShaderSource,
                                 std::variant<std::string, std::vector<uint32_t>>& vertShader,
                                 std::variant<std::string, std::vector<uint32_t>>& fragShader) {
    ensure_initialized();

    shader_translate::ShaderOptions options;
    int version = 0;

    if (is_glsl(languages, version)) {
        options.glsl_version = version;
        options.glsl_es = false;
#ifdef __APPLE__
        options.enable_420pack = false;
#endif

        auto vertResult = shader_translate::compile(vertShaderSource, shader_translate::ShaderType::Vertex,
                                                    shader_translate::TargetLanguage::GLSL, options);
        if (!vertResult.success) {
            printf("Failed to compile vertex shader: %s\n", vertResult.error_message.c_str());
            exit(1);
        }
        vertShader = std::get<std::string>(vertResult.data);
        vertShaderDesc = { LLGL::ShaderType::Vertex, std::get<std::string>(vertShader).c_str() };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        printf("GLSL:\n%s\n", std::get<std::string>(vertShader).c_str());

        auto fragResult = shader_translate::compile(fragShaderSource, shader_translate::ShaderType::Fragment,
                                                    shader_translate::TargetLanguage::GLSL, options);
        if (!fragResult.success) {
            printf("Failed to compile fragment shader: %s\n", fragResult.error_message.c_str());
            exit(1);
        }
        fragShader = std::get<std::string>(fragResult.data);
        fragShaderDesc = { LLGL::ShaderType::Fragment, std::get<std::string>(fragShader).c_str() };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        printf("GLSL:\n%s\n", std::get<std::string>(fragShader).c_str());

    } else if (is_glsles(languages, version)) {
        options.glsl_es = true;
        options.glsl_es_version = version;
#ifdef __APPLE__
        options.enable_420pack = false;
#endif

        auto vertResult = shader_translate::compile(vertShaderSource, shader_translate::ShaderType::Vertex,
                                                    shader_translate::TargetLanguage::GLSL_ES, options);
        if (!vertResult.success) {
            printf("Failed to compile vertex shader: %s\n", vertResult.error_message.c_str());
            exit(1);
        }
        vertShader = std::get<std::string>(vertResult.data);
        vertShaderDesc = { LLGL::ShaderType::Vertex, std::get<std::string>(vertShader).c_str() };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        printf("GLSL ES:\n%s\n", std::get<std::string>(vertShader).c_str());

        auto fragResult = shader_translate::compile(fragShaderSource, shader_translate::ShaderType::Fragment,
                                                    shader_translate::TargetLanguage::GLSL_ES, options);
        if (!fragResult.success) {
            printf("Failed to compile fragment shader: %s\n", fragResult.error_message.c_str());
            exit(1);
        }
        fragShader = std::get<std::string>(fragResult.data);
        fragShaderDesc = { LLGL::ShaderType::Fragment, std::get<std::string>(fragShader).c_str() };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        printf("GLSL ES:\n%s\n", std::get<std::string>(fragShader).c_str());

    } else if (is_spirv(languages, version)) {
        auto vertResult = shader_translate::compile(vertShaderSource, shader_translate::ShaderType::Vertex,
                                                    shader_translate::TargetLanguage::SPIRV, options);
        if (!vertResult.success) {
            printf("Failed to compile vertex shader: %s\n", vertResult.error_message.c_str());
            exit(1);
        }
        auto& spirvVert = std::get<std::vector<uint32_t>>(vertResult.data);
        vertShader = spirvVert;
        char* vertShaderBinary = (char*) malloc(spirvVert.size() * sizeof(uint32_t));
        memcpy(vertShaderBinary, spirvVert.data(), spirvVert.size() * sizeof(uint32_t));
        vertShaderDesc = { LLGL::ShaderType::Vertex, vertShaderBinary };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::BinaryBuffer;
        vertShaderDesc.sourceSize = spirvVert.size() * sizeof(uint32_t);

        auto fragResult = shader_translate::compile(fragShaderSource, shader_translate::ShaderType::Fragment,
                                                    shader_translate::TargetLanguage::SPIRV, options);
        if (!fragResult.success) {
            printf("Failed to compile fragment shader: %s\n", fragResult.error_message.c_str());
            exit(1);
        }
        auto& spirvFrag = std::get<std::vector<uint32_t>>(fragResult.data);
        fragShader = spirvFrag;
        char* fragShaderBinary = (char*) malloc(spirvFrag.size() * sizeof(uint32_t));
        memcpy(fragShaderBinary, spirvFrag.data(), spirvFrag.size() * sizeof(uint32_t));
        fragShaderDesc = { LLGL::ShaderType::Fragment, fragShaderBinary };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::BinaryBuffer;
        fragShaderDesc.sourceSize = spirvFrag.size() * sizeof(uint32_t);

        printf("SPIRV:\n");

    } else if (is_hlsl(languages, version)) {
        options.hlsl_shader_model = version / 10;

        int semanticIndex = 0;
        for (auto& attribute : vertexFormat.attributes) {
            if (attribute.name.compare("position") != 0) {
                attribute.name = "TEXCOORD";
                attribute.semanticIndex = semanticIndex++;
            }
        }

        auto vertResult = shader_translate::compile(vertShaderSource, shader_translate::ShaderType::Vertex,
                                                    shader_translate::TargetLanguage::HLSL, options);
        if (!vertResult.success) {
            printf("Failed to compile vertex shader: %s\n", vertResult.error_message.c_str());
            exit(1);
        }
        vertShader = std::get<std::string>(vertResult.data);
        vertShaderDesc = { LLGL::ShaderType::Vertex, std::get<std::string>(vertShader).c_str() };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        vertShaderDesc.entryPoint = "main";
        vertShaderDesc.profile = "vs_5_0";
        printf("HLSL:\n%s\n", std::get<std::string>(vertShader).c_str());

        auto fragResult = shader_translate::compile(fragShaderSource, shader_translate::ShaderType::Fragment,
                                                    shader_translate::TargetLanguage::HLSL, options);
        if (!fragResult.success) {
            printf("Failed to compile fragment shader: %s\n", fragResult.error_message.c_str());
            exit(1);
        }
        fragShader = std::get<std::string>(fragResult.data);
        fragShaderDesc = { LLGL::ShaderType::Fragment, std::get<std::string>(fragShader).c_str() };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        fragShaderDesc.entryPoint = "main";
        fragShaderDesc.profile = "ps_5_0";
        printf("HLSL:\n%s\n", std::get<std::string>(fragShader).c_str());

    } else if (is_metal(languages, version)) {
        options.metal_decoration_binding = true;

        auto vertResult = shader_translate::compile(vertShaderSource, shader_translate::ShaderType::Vertex,
                                                    shader_translate::TargetLanguage::Metal, options);
        if (!vertResult.success) {
            printf("Failed to compile vertex shader: %s\n", vertResult.error_message.c_str());
            exit(1);
        }
        vertShader = std::get<std::string>(vertResult.data);
        vertShaderDesc = { LLGL::ShaderType::Vertex, std::get<std::string>(vertShader).c_str() };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        vertShaderDesc.entryPoint = "main0";
        vertShaderDesc.profile = "2.1";
        printf("MSL:\n%s\n", std::get<std::string>(vertShader).c_str());

        auto fragResult = shader_translate::compile(fragShaderSource, shader_translate::ShaderType::Fragment,
                                                    shader_translate::TargetLanguage::Metal, options);
        if (!fragResult.success) {
            printf("Failed to compile fragment shader: %s\n", fragResult.error_message.c_str());
            exit(1);
        }
        fragShader = std::get<std::string>(fragResult.data);
        fragShaderDesc = { LLGL::ShaderType::Fragment, std::get<std::string>(fragShader).c_str() };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        fragShaderDesc.entryPoint = "main0";
        fragShaderDesc.profile = "2.1";
        printf("MSL:\n%s\n", std::get<std::string>(fragShader).c_str());

    } else {
        printf("Unsupported shader language\n");
        exit(1);
    }
}

void generate_shader(LLGL::ShaderDescriptor& vertShaderDesc, LLGL::ShaderDescriptor& fragShaderDesc,
                     const std::vector<LLGL::ShadingLanguage>& languages, LLGL::VertexFormat& vertexFormat,
                     std::string name_shader, std::variant<std::string, std::vector<uint32_t>>& vertShader,
                     std::variant<std::string, std::vector<uint32_t>>& fragShader) {
#ifdef WIN32
    std::filesystem::path shaderPath = "../../shader";
#else
    std::filesystem::path shaderPath = "../shader";
#endif
    std::filesystem::path vertShaderPath = shaderPath / (name_shader + ".vert");
    std::filesystem::path fragShaderPath = shaderPath / (name_shader + ".frag");

    std::string vertShaderSource;
    std::string fragShaderSource;
    std::ifstream shaderVertFile(vertShaderPath);
    if (!shaderVertFile.is_open()) {
        LLGL::Log::Printf("Failed to open shader file");
        throw std::runtime_error("Failed to open shader file");
    }
    vertShaderSource = std::string((std::istreambuf_iterator<char>(shaderVertFile)), std::istreambuf_iterator<char>());
    std::ifstream shaderFragFile(fragShaderPath);
    if (!shaderFragFile.is_open()) {
        LLGL::Log::Printf("Failed to open shader file");
        throw std::runtime_error("Failed to open shader file");
    }
    fragShaderSource = std::string((std::istreambuf_iterator<char>(shaderFragFile)), std::istreambuf_iterator<char>());

    generate_shader_from_string(vertShaderDesc, fragShaderDesc, languages, vertexFormat, vertShaderSource,
                                fragShaderSource, vertShader, fragShader);
}