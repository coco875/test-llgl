// LLGL/SDL Test
// 2/16/25

#include <memory>
#include <variant>

#include <LLGL/LLGL.h>

#include <stb_image.h>

#ifdef LLGL_OS_LINUX
#include <GL/glx.h>
#endif

#include "imgui.h"

#include "sdl_llgl.h"
#include "imgui_llgl.h"
#include "LLGL/Utils/TypeNames.h"
#include "LLGL/Utils/VertexFormat.h"

#include <glslang/Public/ShaderLang.h>

#include "shader_translation.h"

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

LLGL::PipelineState* create_pipeline(LLGL::RenderSystemPtr& llgl_renderer, LLGL::SwapChain* llgl_swapChain,
                                     std::vector<LLGL::ShadingLanguage> languages, LLGL::VertexFormat& vertexFormat,
                                     std::string name, LLGL::PipelineLayout* pipelineLayout = nullptr) {
    LLGL::ShaderDescriptor vertShaderDesc, fragShaderDesc;

    std::variant<std::string, std::vector<uint32_t>> vertShaderSourceC, fragShaderSourceC;
    generate_shader(vertShaderDesc, fragShaderDesc, languages, vertexFormat, name, vertShaderSourceC,
                    fragShaderSourceC);

    // Specify vertex attributes for vertex shader
    vertShaderDesc.vertex.inputAttribs = vertexFormat.attributes;

    LLGL::Shader* vertShader = llgl_renderer->CreateShader(vertShaderDesc);
    LLGL::Shader* fragShader = llgl_renderer->CreateShader(fragShaderDesc);

    for (LLGL::Shader* shader : { vertShader, fragShader }) {
        if (const LLGL::Report* report = shader->GetReport())
            LLGL::Log::Errorf("%s", report->GetText());
    }

    // Create graphics pipeline
    LLGL::PipelineState* pipeline = nullptr;
    LLGL::PipelineCache* pipelineCache = nullptr;

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    {
        pipelineDesc.vertexShader = vertShader;
        pipelineDesc.fragmentShader = fragShader;
        pipelineDesc.renderPass = llgl_swapChain->GetRenderPass();
        pipelineDesc.pipelineLayout = pipelineLayout;
    }

    // Create graphics PSO
    pipeline = llgl_renderer->CreatePipelineState(pipelineDesc, pipelineCache);

    // Link shader program and check for errors
    if (const LLGL::Report* report = pipeline->GetReport()) {
        if (report->HasErrors()) {
            const char* a = report->GetText();
            LLGL::Log::Errorf("%s\n", a);
            throw std::runtime_error("Failed to link shader program");
        }
    }
    return pipeline;
}

LLGL::Texture* LoadTexture(const std::string& filename, LLGL::RenderSystemPtr& llgl_render) {
    // Load image data from file (using STBI library, see http://nothings.org/stb_image.h)
    int width, height, channels;
    void* imageData = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    if (imageData == nullptr) {
        throw std::runtime_error("Failed to load image data");
    }

    LLGL::ImageView imageView(LLGL::ImageFormat::RGBA, // Image format
                              LLGL::DataType::UInt8,   // Data type
                              imageData,               // Image data
                              width * height * 4       // Image data size
    );

    {
        // Create texture
        LLGL::TextureDescriptor texDesc;
        {
            // Texture type: 2D
            texDesc.type = LLGL::TextureType::Texture2D;

            // Texture hardware format: RGBA with normalized 8-bit unsigned char type
            texDesc.format = LLGL::Format::RGBA8UNorm; // BGRA8UNorm

            // Texture size
            texDesc.extent = { static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), 1 };

            // Generate all MIP-map levels for this texture
            texDesc.miscFlags = LLGL::MiscFlags::GenerateMips;
        }
        return llgl_render->CreateTexture(texDesc, &imageView);
    }
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
    desc.flags |= LLGL::RenderSystemFlags::DebugDevice;
    LLGL::Report report;
    auto llgl_renderer = LLGL::RenderSystem::Load(desc, &report);

    // Create SDL window and LLGL swap-chain
    if (!llgl_renderer) {
        auto a = report.GetText();
        LLGL::Log::Errorf("Failed to load \"%s\" module. Falling back to \"Null\" device.\n", desc.moduleName.c_str());
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

    const auto& languages = llgl_renderer->GetRenderingCaps().shadingLanguages;

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

    LLGL::PipelineState* pipeline = create_pipeline(llgl_renderer, llgl_swapChain, languages, vertexFormat, "test");

    struct VertexTex {
        float position[2];
        uint8_t color[4];
        float texCoord[2];
    };

    // Vertex data (3 vertices for our triangle)
    // const float s = 0.5f;

    VertexTex verticesTex[] = {
        { { 0, s + 0.3 }, { 255, 0, 0, 255 }, { 0.5, 0.0 } },   // 1st vertex: center-top, red
        { { s, -s + 0.3 }, { 0, 255, 0, 255 }, { 0.0, 1.0 } },  // 2nd vertex: right-bottom, green
        { { -s, -s + 0.3 }, { 0, 0, 255, 255 }, { 1.0, 1.0 } }, // 3rd vertex: left-bottom, blue
    };

    // Vertex format
    LLGL::VertexFormat vertexTexFormat;

    vertexTexFormat.AppendAttribute({ "position", LLGL::Format::RG32Float });
    vertexTexFormat.AppendAttribute({ "color", LLGL::Format::RGBA8UNorm });
    vertexTexFormat.AppendAttribute({ "texCoord", LLGL::Format::RG32Float });

    // Update stride in case out vertex structure is not 4-byte aligned
    vertexTexFormat.SetStride(sizeof(VertexTex));

    // Create vertex buffer
    LLGL::BufferDescriptor vertexTexBufferDesc;
    {
        vertexTexBufferDesc.size = sizeof(verticesTex); // Size (in bytes) of the vertex buffer
        vertexTexBufferDesc.bindFlags =
            LLGL::BindFlags::VertexBuffer; // Enables the buffer to be bound to a vertex buffer slot
        vertexTexBufferDesc.vertexAttribs = vertexTexFormat.attributes; // Vertex format layout
    }
    LLGL::Buffer* vertexTexBuffer = llgl_renderer->CreateBuffer(vertexTexBufferDesc, verticesTex);

    std::vector<std::uint32_t> indices = {
        0, 1, 2, // Triangle
    };
    LLGL::BufferDescriptor bufferDesc;
    {
        bufferDesc.size = indices.size() * sizeof(std::uint32_t);
        bufferDesc.format = LLGL::Format::R32UInt;
        bufferDesc.bindFlags = LLGL::BindFlags::IndexBuffer;
    }
    bufferDesc.debugName = "IndexBuffer";
    LLGL::Buffer* indexBuffer = llgl_renderer->CreateBuffer(bufferDesc, &indices[0]);

    LLGL::PipelineLayoutDescriptor layoutDesc;
    {
        layoutDesc.bindings = {
            LLGL::BindingDescriptor{ "colorMap", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled,
                                     LLGL::StageFlags::FragmentStage, 0 },
            LLGL::BindingDescriptor{ "samplerState", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage,
                                     1 },
        };
        layoutDesc.combinedTextureSamplers = { LLGL::CombinedTextureSamplerDescriptor{ "colorMap", "colorMap",
                                                                                       "samplerState", 2 } };
    }
    LLGL::PipelineLayout* pipelineLayout = llgl_renderer->CreatePipelineLayout(layoutDesc);

    LLGL::PipelineState* pipeline2 =
        create_pipeline(llgl_renderer, llgl_swapChain, languages, vertexTexFormat, "test_texture", pipelineLayout);

    // Load texture
    LLGL::Texture* texture = LoadTexture("../random.png", llgl_renderer);

    LLGL::SamplerDescriptor anisotropySamplerDesc;
    { anisotropySamplerDesc.maxAnisotropy = 8; }
    LLGL::Sampler* sampler = llgl_renderer->CreateSampler(anisotropySamplerDesc);

    auto llgl_cmdBuffer = llgl_renderer->CreateCommandBuffer(LLGL::CommandBufferFlags::ImmediateSubmit);

    InitImGui(*surface, llgl_renderer, llgl_swapChain, llgl_cmdBuffer);

    while (surface->ProcessEvents(llgl_swapChain)) {
        // Rendering
        llgl_cmdBuffer->Begin();
        {
            // Set viewport and scissor rectangle
            llgl_cmdBuffer->SetViewport(llgl_swapChain->GetResolution());

            // Set vertex buffer

            llgl_cmdBuffer->BeginRenderPass(*llgl_swapChain);
            {
                llgl_cmdBuffer->Clear(LLGL::ClearFlags::Color, LLGL::ClearValue{ 0.0f, 0.2f, 0.2f, 1.0f });

                llgl_cmdBuffer->SetVertexBuffer(*vertexBuffer);

                // Set graphics pipeline
                llgl_cmdBuffer->SetPipelineState(*pipeline);

                // Draw triangle with 3 vertices
                llgl_cmdBuffer->Draw(3, 0);

                llgl_cmdBuffer->SetVertexBuffer(*vertexTexBuffer);
                llgl_cmdBuffer->SetIndexBuffer(*indexBuffer);

                llgl_cmdBuffer->SetPipelineState(*pipeline2);

                llgl_cmdBuffer->SetResource(0, *texture);
                llgl_cmdBuffer->SetResource(1, *sampler);
                llgl_cmdBuffer->DrawIndexed(6, 0);

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