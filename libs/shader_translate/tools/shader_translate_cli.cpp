/**
 * @file shader_translate_cli.cpp
 * @brief Command-line tool for shader cross-compilation
 *
 * Usage:
 *   shader_translate [options] <vertex.glsl> <fragment.glsl>
 *   shader_translate --help
 */

#include "shader_translate/shader_translate.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>

using namespace shader_translate;

struct CliOptions {
    std::string vertex_path;
    std::string fragment_path;
    std::string output_path = "shaders.h";
    std::string prefix = "g_";
    std::vector<TargetLanguage> targets;
    bool all_targets = false;
    bool help = false;
    bool verbose = false;
    ShaderOptions shader_opts;
};

void print_usage(const char* program) {
    std::cout << R"(
shader_translate - Cross-platform shader compiler

Usage:
  )" << program
              << R"( [options] <vertex.glsl> <fragment.glsl>

Options:
  -o, --output <file>       Output header file (default: shaders.h)
  -t, --target <lang>       Target language: spirv, glsl, glsl_es, hlsl, metal
                            Can be specified multiple times
  --all                     Generate all target languages
  --prefix <name>           Variable prefix (default: g_)
  
  --glsl-version <ver>      GLSL version (default: 410)
  --glsl-es-version <ver>   GLSL ES version (default: 300)
  --hlsl-model <ver>        HLSL shader model (default: 50)
  --metal-version <ver>     Metal version (default: 20100)
  
  --no-420pack              Disable GL_ARB_shading_language_420pack
  --no-decoration-binding   Disable Metal decoration binding
  
  -v, --verbose             Verbose output
  -h, --help                Show this help

Examples:
  # Compile to all targets
  )" << program
              << R"( --all -o shaders.h vertex.glsl fragment.glsl

  # Compile to specific targets
  )" << program
              << R"( -t metal -t glsl -o shaders.h vertex.glsl fragment.glsl

  # Use custom prefix
  )" << program
              << R"( --all --prefix g_ImGui -o imgui_shaders.h imgui.vert imgui.frag
)";
}

TargetLanguage parse_target(const std::string& str) {
    if (str == "spirv" || str == "spv")
        return TargetLanguage::SPIRV;
    if (str == "glsl" || str == "gl")
        return TargetLanguage::GLSL;
    if (str == "glsl_es" || str == "gles" || str == "es")
        return TargetLanguage::GLSL_ES;
    if (str == "hlsl" || str == "dx")
        return TargetLanguage::HLSL;
    if (str == "metal" || str == "msl")
        return TargetLanguage::Metal;
    std::cerr << "Unknown target: " << str << "\n";
    exit(1);
}

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file: " << path << "\n";
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void write_file(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot write to file: " << path << "\n";
        exit(1);
    }
    file << content;
}

CliOptions parse_args(int argc, char* argv[]) {
    CliOptions opts;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            opts.help = true;
        } else if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
        } else if (arg == "--all") {
            opts.all_targets = true;
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            opts.output_path = argv[++i];
        } else if ((arg == "-t" || arg == "--target") && i + 1 < argc) {
            opts.targets.push_back(parse_target(argv[++i]));
        } else if (arg == "--prefix" && i + 1 < argc) {
            opts.prefix = argv[++i];
        } else if (arg == "--glsl-version" && i + 1 < argc) {
            opts.shader_opts.glsl_version = std::stoi(argv[++i]);
        } else if (arg == "--glsl-es-version" && i + 1 < argc) {
            opts.shader_opts.glsl_es_version = std::stoi(argv[++i]);
        } else if (arg == "--hlsl-model" && i + 1 < argc) {
            opts.shader_opts.hlsl_shader_model = std::stoi(argv[++i]);
        } else if (arg == "--metal-version" && i + 1 < argc) {
            opts.shader_opts.metal_version = std::stoi(argv[++i]);
        } else if (arg == "--no-420pack") {
            opts.shader_opts.enable_420pack = false;
        } else if (arg == "--no-decoration-binding") {
            opts.shader_opts.metal_decoration_binding = false;
        } else if (arg[0] != '-') {
            if (opts.vertex_path.empty()) {
                opts.vertex_path = arg;
            } else if (opts.fragment_path.empty()) {
                opts.fragment_path = arg;
            }
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            exit(1);
        }
    }

    return opts;
}

int main(int argc, char* argv[]) {
    CliOptions opts = parse_args(argc, argv);

    if (opts.help) {
        print_usage(argv[0]);
        return 0;
    }

    if (opts.vertex_path.empty() || opts.fragment_path.empty()) {
        std::cerr << "Error: Both vertex and fragment shader paths required.\n";
        print_usage(argv[0]);
        return 1;
    }

    if (opts.targets.empty() && !opts.all_targets) {
        std::cerr << "Error: No target specified. Use --all or -t <target>.\n";
        return 1;
    }

    if (opts.all_targets) {
        opts.targets = { TargetLanguage::SPIRV, TargetLanguage::GLSL, TargetLanguage::GLSL_ES, TargetLanguage::HLSL,
                         TargetLanguage::Metal };
    }

    // Initialize library
    if (!initialize()) {
        std::cerr << "Error: Failed to initialize shader_translate library.\n";
        return 1;
    }

    // Read source files
    std::string vertSource = read_file(opts.vertex_path);
    std::string fragSource = read_file(opts.fragment_path);

    if (opts.verbose) {
        std::cout << "Compiling: " << opts.vertex_path << " + " << opts.fragment_path << "\n";
        std::cout << "Output: " << opts.output_path << "\n";
        std::cout << "Prefix: " << opts.prefix << "\n";
    }

    // Generate header content
    std::ostringstream header;
    header << "// Auto-generated shader header\n";
    header << "// Generated by shader_translate CLI\n";
    header << "// Source: " << opts.vertex_path << " + " << opts.fragment_path << "\n";
    header << "// Do not edit manually!\n\n";
    header << "#pragma once\n\n";
    header << "#ifndef " << opts.prefix << "SHADERS_H\n";
    header << "#define " << opts.prefix << "SHADERS_H\n\n";
    header << "#include <cstdint>\n\n";

    bool any_error = false;

    for (TargetLanguage target : opts.targets) {
        if (opts.verbose) {
            std::cout << "  Compiling to " << target_language_name(target) << "...\n";
        }

        CompiledShader vertShader = compile(vertSource, ShaderType::Vertex, target, opts.shader_opts);
        CompiledShader fragShader = compile(fragSource, ShaderType::Fragment, target, opts.shader_opts);

        if (!vertShader.success) {
            std::cerr << "Error compiling vertex shader to " << target_language_name(target) << ":\n"
                      << vertShader.error_message << "\n";
            any_error = true;
            continue;
        }

        if (!fragShader.success) {
            std::cerr << "Error compiling fragment shader to " << target_language_name(target) << ":\n"
                      << fragShader.error_message << "\n";
            any_error = true;
            continue;
        }

        // Add section header
        std::string langName = target_language_name(target);
        header << "// ==============================================================================\n";
        header << "// " << langName << " Shaders\n";
        header << "// ==============================================================================\n\n";

        // Get suffix for variable names
        std::string suffix;
        switch (target) {
            case TargetLanguage::SPIRV:
                suffix = "SPIRV";
                break;
            case TargetLanguage::GLSL:
                suffix = "GLSL";
                break;
            case TargetLanguage::GLSL_ES:
                suffix = "GLSL_ES";
                break;
            case TargetLanguage::HLSL:
                suffix = "HLSL";
                break;
            case TargetLanguage::Metal:
                suffix = "Metal";
                break;
        }

        if (target == TargetLanguage::SPIRV) {
            // Binary output
            auto write_binary = [&](const std::string& name, const CompiledShader& shader) {
                const auto& spirv = std::get<std::vector<uint32_t>>(shader.data);
                const uint8_t* bytes = reinterpret_cast<const uint8_t*>(spirv.data());
                size_t size = spirv.size() * sizeof(uint32_t);

                header << "static const unsigned char " << opts.prefix << name << "_" << suffix << "[] = {\n";
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
                header << "static const size_t " << opts.prefix << name << "_" << suffix << "_Size = " << size
                       << ";\n\n";
            };
            write_binary("VertexShader", vertShader);
            write_binary("FragmentShader", fragShader);
        } else {
            // String output
            auto write_string = [&](const std::string& name, const CompiledShader& shader) {
                const auto& code = std::get<std::string>(shader.data);
                header << "static const char* " << opts.prefix << name << "_" << suffix << " = R\"(\n";
                header << code;
                header << ")\";\n\n";
            };
            write_string("VertexShader", vertShader);
            write_string("FragmentShader", fragShader);
        }
    }

    header << "#endif // " << opts.prefix << "SHADERS_H\n";

    // Write output file
    write_file(opts.output_path, header.str());

    if (opts.verbose || !any_error) {
        std::cout << "Generated: " << opts.output_path << "\n";
    }

    finalize();

    return any_error ? 1 : 0;
}
