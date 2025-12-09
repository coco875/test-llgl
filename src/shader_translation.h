#ifndef SHADER_TRANSLATION_H
#define SHADER_TRANSLATION_H

#include <LLGL/LLGL.h>
#include <LLGL/Utils/VertexFormat.h>
#include <variant>
#include <vector>
#include <string>

void glslang_spirv_cross_test();

void generate_shader(LLGL::ShaderDescriptor& vertShaderDesc, LLGL::ShaderDescriptor& fragShaderDesc,
                     const std::vector<LLGL::ShadingLanguage>& languages, LLGL::VertexFormat& vertexFormat,
                     std::string name_shader, std::variant<std::string, std::vector<uint32_t>>& vertShader,
                     std::variant<std::string, std::vector<uint32_t>>& fragShader);

void generate_shader_from_string(LLGL::ShaderDescriptor& vertShaderDesc, LLGL::ShaderDescriptor& fragShaderDesc,
                                 const std::vector<LLGL::ShadingLanguage>& languages, LLGL::VertexFormat& vertexFormat,
                                 std::string vertShaderSource, std::string fragShaderSource,
                                 std::variant<std::string, std::vector<uint32_t>>& vertShader,
                                 std::variant<std::string, std::vector<uint32_t>>& fragShader);

#endif