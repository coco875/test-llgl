// LLGL/SDL Test
// 2/16/25

#include <memory>

#include <LLGL/LLGL.h>

#ifdef LLGL_OS_LINUX
#include <GL/glx.h>
#endif

#include "imgui.h"

#include <filesystem>
#include <fstream>

#include "sdl_llgl.h"
#include "imgui_llgl.h"
#include "LLGL/Utils/TypeNames.h"
#include "LLGL/Utils/VertexFormat.h"
#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>

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

void print_info(LLGL::RenderSystemPtr& llgl_render, LLGL::SwapChain* llgl_swapChain) {
    // Print renderer information
    const auto& info = llgl_render->GetRendererInfo();

    LLGL::Log::Printf("Renderer:             %s\n"
                      "Device:               %s\n"
                      "Vendor:               %s\n"
                      "Shading Language:     %s\n"
                      "Swap Chain Format:    %s\n"
                      "Depth/Stencil Format: %s\n"
                      "Resolution:           %u x %u\n"
                      "Samples:              %u\n",
                      info.rendererName.c_str(), info.deviceName.c_str(), info.vendorName.c_str(),
                      info.shadingLanguageName.c_str(), LLGL::ToString(llgl_swapChain->GetColorFormat()),
                      LLGL::ToString(llgl_swapChain->GetDepthStencilFormat()), llgl_swapChain->GetResolution().width,
                      llgl_swapChain->GetResolution().height, llgl_swapChain->GetSamples());
}

#ifdef _WIN32
int SDL_main(int argc, char** argv) {
#else
#if defined(__cplusplus) && defined(PLATFORM_IOS)
extern "C"
#endif
    int
    main(int argc, char* argv[]) {
#endif
    LLGL::Log::RegisterCallbackStd();

    int rendererID = LLGL::RendererID::OpenGL;

#ifdef LLGL_OS_LINUX
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");
#endif

    // Init SDL
    SDL_Init(SDL_INIT_VIDEO);

    const uint32_t window_width = 800;
    const uint32_t window_height = 600;

    LLGL::SwapChainDescriptor swapChainDesc;
    swapChainDesc.resolution = { window_width, window_height };
    swapChainDesc.resizable = true;
    // swapChainDesc.samples = 4;

    LLGL::RenderSystemDescriptor desc;
    auto surface = std::make_shared<SDLSurface>(swapChainDesc.resolution, "LLGL SwapChain", rendererID, desc);
    LLGL::Report report;
    auto llgl_renderer = LLGL::RenderSystem::Load(desc, &report);

    // Create SDL window and LLGL swap-chain
    if (!llgl_renderer) {
        auto a = report.GetText();
        LLGL::Log::Errorf("Failed to load \"%s\" module. Falling back to \"Null\" device.\n", "OpenGL");
        LLGL::Log::Errorf("Reason for failure: %s", report.HasErrors() ? report.GetText() : "Unknown\n");
        llgl_renderer = LLGL::RenderSystem::Load("Null");
        if (!llgl_renderer) {
            LLGL::Log::Errorf("Failed to load \"Null\" module. Exiting.\n");
            return 1;
        }
    }

    auto llgl_swapChain = llgl_renderer->CreateSwapChain(swapChainDesc, surface);

    print_info(llgl_renderer, llgl_swapChain);

    LLGL::Log::Printf("glsl version: %s\n", glslang::GetGlslVersionString());

    // Vertex data structure
    struct Vertex {
        float position[2];
        uint8_t color[4];
    };

    // Vertex data (3 vertices for our triangle)
    const float s = 0.5f;

    Vertex vertices[] = {
        { { 0, s }, { 255, 0, 0, 255 } },   // 1st vertex: center-top, red
        { { s, -s }, { 0, 255, 0, 255 } },  // 2nd vertex: right-bottom, green
        { { -s, -s }, { 0, 0, 255, 255 } }, // 3rd vertex: left-bottom, blue
    };

    // Vertex format
    LLGL::VertexFormat vertexFormat;

    // Append 2D float vector for position attribute
    vertexFormat.AppendAttribute({ "position", LLGL::Format::RG32Float });

    // Append 3D unsigned byte vector for color
    vertexFormat.AppendAttribute({ "color", LLGL::Format::RGBA8UNorm });

    // Update stride in case out vertex structure is not 4-byte aligned
    vertexFormat.SetStride(sizeof(Vertex));

    // Create vertex buffer
    LLGL::BufferDescriptor vertexBufferDesc;
    {
        vertexBufferDesc.size = sizeof(vertices); // Size (in bytes) of the vertex buffer
        vertexBufferDesc.bindFlags =
            LLGL::BindFlags::VertexBuffer; // Enables the buffer to be bound to a vertex buffer slot
        vertexBufferDesc.vertexAttribs = vertexFormat.attributes; // Vertex format layout
    }
    LLGL::Buffer* vertexBuffer = llgl_renderer->CreateBuffer(vertexBufferDesc, vertices);

    // Create shaders
    LLGL::Shader* vertShader = nullptr;
    LLGL::Shader* fragShader = nullptr;

    const auto& languages = llgl_renderer->GetRenderingCaps().shadingLanguages;

    LLGL::ShaderDescriptor vertShaderDesc, fragShaderDesc;

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

    vertShaderGlslang.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
    vertShaderGlslang.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_4);

    fragShaderGlslang.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
    fragShaderGlslang.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_4);

    TBuiltInResource builtInResources = InitResources();
    vertShaderGlslang.parse(&builtInResources, 120, true, EShMsgDefault);
    fragShaderGlslang.parse(&builtInResources, 120, true, EShMsgDefault);

    // glslang::TProgram program;
    // program.addShader(&vertShaderGlslang);
    // program.addShader(&fragShaderGlslang);

    // program.link(EShMsgDefault);

    glslang::FinalizeProcess();

    if (std::find(languages.begin(), languages.end(), LLGL::ShadingLanguage::GLSL) != languages.end()) {
        if (std::find(languages.begin(), languages.end(), LLGL::ShadingLanguage::GLSL_140) != languages.end()) {
#ifdef __APPLE__
            vertShaderDesc = { LLGL::ShaderType::Vertex, "../shader/test.vert" };
            fragShaderDesc = { LLGL::ShaderType::Fragment, "../shader/test.frag" };
#else
            vertShaderDesc = { LLGL::ShaderType::Vertex, "Example.vert" };
            fragShaderDesc = { LLGL::ShaderType::Fragment, "Example.frag" };
#endif
        } else {
            vertShaderDesc = { LLGL::ShaderType::Vertex, "Example.120.vert" };
            fragShaderDesc = { LLGL::ShaderType::Fragment, "Example.120.frag" };
        }
    } else if (std::find(languages.begin(), languages.end(), LLGL::ShadingLanguage::SPIRV) != languages.end()) {
        // vertShaderDesc = LLGL::ShaderDescFromFile(LLGL::ShaderType::Vertex, "Example.450core.vert.spv");
        // fragShaderDesc = LLGL::ShaderDescFromFile(LLGL::ShaderType::Fragment, "Example.450core.frag.spv");
    } else if (std::find(languages.begin(), languages.end(), LLGL::ShadingLanguage::HLSL) != languages.end()) {
        vertShaderDesc = { LLGL::ShaderType::Vertex, "Example.hlsl", "VS", "vs_4_0" };
        fragShaderDesc = { LLGL::ShaderType::Fragment, "Example.hlsl", "PS", "ps_4_0" };
    } else if (std::find(languages.begin(), languages.end(), LLGL::ShadingLanguage::Metal) != languages.end()) {
        vertShaderDesc = { LLGL::ShaderType::Vertex, "Example.metal", "VS", "1.1" };
        fragShaderDesc = { LLGL::ShaderType::Fragment, "Example.metal", "PS", "1.1" };
        vertShaderDesc.flags |= LLGL::ShaderCompileFlags::DefaultLibrary;
        fragShaderDesc.flags |= LLGL::ShaderCompileFlags::DefaultLibrary;
    }

    // Specify vertex attributes for vertex shader
    vertShaderDesc.vertex.inputAttribs = vertexFormat.attributes;

    vertShader = llgl_renderer->CreateShader(vertShaderDesc);
    fragShader = llgl_renderer->CreateShader(fragShaderDesc);

    for (LLGL::Shader* shader : { vertShader, fragShader }) {
        if (const LLGL::Report* report = shader->GetReport())
            LLGL::Log::Errorf("%s", report->GetText());
    }

    // Create graphics pipeline
    LLGL::PipelineState* pipeline = nullptr;
    LLGL::PipelineCache* pipelineCache = nullptr;

#if ENABLE_CACHED_PSO

    // Try to read PSO cache from file
    const std::string cacheFilename = "GraphicsPSO." + rendererModule + ".cache";
    bool hasInitialCache = false;

    LLGL::Blob pipelineCacheBlob = LLGL::Blob::CreateFromFile(cacheFilename);
    if (pipelineCacheBlob) {
        LLGL::Log::Printf("Pipeline cache restored: %zu bytes\n", pipelineCacheBlob.GetSize());
        hasInitialCache = true;
    }

    pipelineCache = renderer->CreatePipelineCache(pipelineCacheBlob);

#endif

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    {
        pipelineDesc.vertexShader = vertShader;
        pipelineDesc.fragmentShader = fragShader;
        pipelineDesc.renderPass = llgl_swapChain->GetRenderPass();
#if ENABLE_MULTISAMPLING
        pipelineDesc.rasterizer.multiSampleEnabled = (swapChainDesc.samples > 1);
#endif
    }

    // Create and cache graphics PSO
    std::uint64_t psoStartTime = LLGL::Timer::Tick();
    pipeline = llgl_renderer->CreatePipelineState(pipelineDesc, pipelineCache);
    std::uint64_t psoEndTime = LLGL::Timer::Tick();

#if ENABLE_CACHED_PSO

    const double psoTime =
        static_cast<double>(psoEndTime - psoStartTime) / static_cast<double>(LLGL::Timer::Frequency()) * 1000.0;
    LLGL::Log::Printf("PSO creation time: %f ms\n", psoTime);

    if (!hasInitialCache) {
        if (LLGL::Blob psoCache = pipelineCache->GetBlob()) {
            LLGL::Log::Printf("Pipeline cache created: %zu bytes", psoCache.GetSize());

            // Store PSO cache to file
            std::ofstream file{ cacheFilename, std::ios::out | std::ios::binary };
            file.write(reinterpret_cast<const char*>(psoCache.GetData()),
                       static_cast<std::streamsize>(psoCache.GetSize()));
        }
    }

#endif

    // Link shader program and check for errors
    // if (const LLGL::Report* report = pipeline->GetReport()) {
    //     if (report->HasErrors()) {
    //         LLGL::Log::Errorf("%s\n", report->GetText());
    //         exit(1);
    //     }
    // }

    auto llgl_cmdBuffer = llgl_renderer->CreateCommandBuffer(LLGL::CommandBufferFlags::ImmediateSubmit);

    InitImGui(*surface, llgl_renderer, llgl_swapChain, llgl_cmdBuffer);

    while (surface->ProcessEvents(llgl_swapChain)) {
        // Rendering
        llgl_cmdBuffer->Begin();
        {
            // Set viewport and scissor rectangle
            llgl_cmdBuffer->SetViewport(llgl_swapChain->GetResolution());

            // Set vertex buffer
            // llgl_cmdBuffer->SetVertexBuffer(*vertexBuffer);

            llgl_cmdBuffer->BeginRenderPass(*llgl_swapChain);
            {
                llgl_cmdBuffer->Clear(LLGL::ClearFlags::Color, LLGL::ClearValue{ 0.0f, 0.2f, 0.2f, 1.0f });

                // Set graphics pipeline
                // llgl_cmdBuffer->SetPipelineState(*pipeline);

                // Draw triangle with 3 vertices
                // llgl_cmdBuffer->Draw(3, 0);

                // GUI Rendering with ImGui library
                // Start the Dear ImGui frame
                NewFrameImGui(llgl_renderer, llgl_cmdBuffer);
                ImGui::NewFrame();

                // Show ImGui's demo window
                ImGui::ShowDemoWindow();

                // GUI Rendering
                ImGui::Render();
                RenderImGui(ImGui::GetDrawData(), llgl_renderer, llgl_cmdBuffer);
            }
            llgl_cmdBuffer->EndRenderPass();
        }
        llgl_cmdBuffer->End();

        // Present result on screen
        llgl_swapChain->Present();
    }

    ShutdownImGui(llgl_renderer);
    LLGL::RenderSystem::Unload(std::move(llgl_renderer));
    SDL_Quit();

    return 0;
}