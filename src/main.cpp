// LLGL/SDL Test
// 2/16/25

#include <memory>
#include <variant>

#include <LLGL/LLGL.h>

#include <stb_image.h>

#ifdef LLGL_OS_LINUX
#include <GL/glx.h>
#endif

#include <SDL2/SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"

#include "sdl_llgl.h"
#include "imgui_llgl.h"
#include "LLGL/Utils/TypeNames.h"
#include "LLGL/Utils/VertexFormat.h"

#include <glslang/Public/ShaderLang.h>

#include "shader_translation.h"
#include "math_types.h"
#include "camera.h"
#include "model_loader.h"
#include "primitives.h"

LLGL::RenderSystemPtr llgl_renderer;

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
                                     std::string name, LLGL::PipelineLayout* pipelineLayout = nullptr,
                                     bool enableDepthTest = false, LLGL::CullMode cullMode = LLGL::CullMode::Disabled) {
    LLGL::ShaderDescriptor vertShaderDesc, fragShaderDesc;

    std::variant<std::string, std::vector<uint32_t>> vertShaderSourceC, fragShaderSourceC;
    generate_shader(vertShaderDesc, fragShaderDesc, languages, vertexFormat, name, vertShaderSourceC,
                    fragShaderSourceC);

    // Specify vertex attributes for vertex shader
    vertShaderDesc.vertex.inputAttribs = vertexFormat.attributes;

    LLGL::Shader* vertShader = llgl_renderer->CreateShader(vertShaderDesc);
    LLGL::Shader* fragShader = llgl_renderer->CreateShader(fragShaderDesc);

    for (LLGL::Shader* shader : { vertShader, fragShader }) {
        if (const LLGL::Report* report = shader->GetReport()) {
            LLGL::Log::Errorf("%s", report->GetText());
        }
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

        // Depth testing for 3D rendering
        if (enableDepthTest) {
            pipelineDesc.depth.testEnabled = true;
            pipelineDesc.depth.writeEnabled = true;
            pipelineDesc.depth.compareOp = LLGL::CompareOp::Less;
        }

        // Culling - use counter-clockwise as front face (OpenGL default)
        pipelineDesc.rasterizer.cullMode = cullMode;
        pipelineDesc.rasterizer.frontCCW = true;
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

LLGL::Buffer* create_uniform_buffer(LLGL::RenderSystemPtr& llgl_renderer, std::size_t size) {
    LLGL::BufferDescriptor uniformBufferDesc;
    uniformBufferDesc.size = size;
    uniformBufferDesc.bindFlags = LLGL::BindFlags::ConstantBuffer;
    uniformBufferDesc.cpuAccessFlags = LLGL::CPUAccessFlags::Write;
    uniformBufferDesc.miscFlags = LLGL::MiscFlags::DynamicUsage;
    uniformBufferDesc.debugName = "MatricesBuffer";
    return llgl_renderer->CreateBuffer(uniformBufferDesc);
}

LLGL::PipelineLayout* create_texture_pipeline_layout(LLGL::RenderSystemPtr& llgl_renderer) {
    LLGL::PipelineLayoutDescriptor modelLayoutDesc;
    {
        modelLayoutDesc.bindings = {
            LLGL::BindingDescriptor{ "Matrices", LLGL::ResourceType::Buffer, LLGL::BindFlags::ConstantBuffer,
                                     LLGL::StageFlags::VertexStage, 0 },
            LLGL::BindingDescriptor{ "colorMap", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled,
                                     LLGL::StageFlags::FragmentStage, 1 },
            LLGL::BindingDescriptor{ "samplerState", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage,
                                     2 },
        };
        modelLayoutDesc.combinedTextureSamplers = { LLGL::CombinedTextureSamplerDescriptor{ "colorMap", "colorMap",
                                                                                            "samplerState", 3 } };
    }
    return llgl_renderer->CreatePipelineLayout(modelLayoutDesc);
}

LLGL::PipelineLayout* create_no_texture_pipeline_layout(LLGL::RenderSystemPtr& llgl_renderer) {
    LLGL::PipelineLayoutDescriptor modelNoTexLayoutDesc;
    {
        modelNoTexLayoutDesc.bindings = {
            LLGL::BindingDescriptor{ "Matrices", LLGL::ResourceType::Buffer, LLGL::BindFlags::ConstantBuffer,
                                     LLGL::StageFlags::VertexStage, 0 },
        };
    }
    return llgl_renderer->CreatePipelineLayout(modelNoTexLayoutDesc);
}

LLGL::Texture* create_white_texture(LLGL::RenderSystemPtr& llgl_renderer) {
    uint8_t whitePixel[] = { 255, 255, 255, 255 };
    LLGL::ImageView whiteImageView(LLGL::ImageFormat::RGBA, LLGL::DataType::UInt8, whitePixel, 4);
    LLGL::TextureDescriptor whiteTexDesc;
    whiteTexDesc.type = LLGL::TextureType::Texture2D;
    whiteTexDesc.format = LLGL::Format::RGBA8UNorm;
    whiteTexDesc.extent = { 1, 1, 1 };
    return llgl_renderer->CreateTexture(whiteTexDesc, &whiteImageView);
}

LLGL::Sampler* create_model_sampler(LLGL::RenderSystemPtr& llgl_renderer) {
    LLGL::SamplerDescriptor modelSamplerDesc;
    modelSamplerDesc.maxAnisotropy = 8;
    modelSamplerDesc.addressModeU = LLGL::SamplerAddressMode::Repeat;
    modelSamplerDesc.addressModeV = LLGL::SamplerAddressMode::Repeat;
    return llgl_renderer->CreateSampler(modelSamplerDesc);
}

#ifdef _WIN32
int SDL_main(int argc, char** argv) {
#else
#if defined(__cplusplus) && defined(PLATFORM_IOS)
extern "C"
#endif
    int main(int argc, char* argv[]) {
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
    llgl_renderer = LLGL::RenderSystem::Load(desc, &report);

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

    // Load 3D model
    Model model;
    std::string modelPath = "../model.obj";

    if (argc > 1) {
        modelPath = argv[1];
    }

    if (!model.load(modelPath, llgl_renderer)) {
        LLGL::Log::Errorf("Failed to load model: %s\n", modelPath.c_str());
        LLGL::Log::Printf("Usage: %s [model_path]\n", argv[0]);
        LLGL::Log::Printf("Creating a default cube...\n");

        model = Primitives::createDefaultModel();
    }

    model.createBuffers(llgl_renderer);

    struct Matrices {
        Math::Mat4 model;
        Math::Mat4 view;
        Math::Mat4 projection;
    } matrices;

    LLGL::Buffer* uniformBuffer = create_uniform_buffer(llgl_renderer, sizeof(Matrices));

    // Pipeline layout for 3D model rendering (with texture)
    LLGL::PipelineLayout* modelPipelineLayout = create_texture_pipeline_layout(llgl_renderer);

    // Pipeline layout for 3D model rendering (without texture)
    LLGL::PipelineLayout* modelNoTexPipelineLayout = create_no_texture_pipeline_layout(llgl_renderer);

    auto modelVertexFormat = model.getVertexFormat();

    LLGL::PipelineState* modelPipeline = create_pipeline(llgl_renderer, llgl_swapChain, languages, modelVertexFormat,
                                                         "model", modelPipelineLayout, true, LLGL::CullMode::Back);

    LLGL::PipelineState* modelNoTexPipeline =
        create_pipeline(llgl_renderer, llgl_swapChain, languages, modelVertexFormat, "model_notex",
                        modelNoTexPipelineLayout, true, LLGL::CullMode::Back);

    // White texture for meshes without a texture
    LLGL::Texture* whiteTexture = create_white_texture(llgl_renderer);

    // Sampler for model textures
    LLGL::Sampler* modelSampler = create_model_sampler(llgl_renderer);

    // Create orbit camera
    OrbitCamera camera;
    Math::Vec3 modelCenter = model.getCenter();
    float modelRadius = model.getRadius();
    camera.setTarget(modelCenter, modelRadius * 2.5f);

    // Model rotation angles (for auto-rotation or manual rotation)
    float modelRotationY = 0.0f;
    float modelRotationX = 0.0f;
    bool autoRotate = false;

    auto llgl_cmdBuffer = llgl_renderer->CreateCommandBuffer(LLGL::CommandBufferFlags::ImmediateSubmit);

    InitImGui(*surface, llgl_renderer, llgl_swapChain, llgl_cmdBuffer);

    // Set up event callback for camera control
    surface->SetEventCallback([&camera](const SDL_Event& event) {
        // Don't process mouse if ImGui wants it
        if (!ImGui::GetIO().WantCaptureMouse) {
            switch (event.type) {
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        camera.onMouseDown(event.button.x, event.button.y);
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        camera.onMouseUp();
                    }
                    break;
                case SDL_MOUSEMOTION:
                    camera.onMouseMove(event.motion.x, event.motion.y);
                    break;
                case SDL_MOUSEWHEEL:
                    camera.onMouseWheel(static_cast<float>(event.wheel.y));
                    break;
            }
        }
    });

    // Main render loop
    while (surface->ProcessEvents(llgl_swapChain)) {
        // Update matrices
        float aspect = static_cast<float>(llgl_swapChain->GetResolution().width) /
                       static_cast<float>(llgl_swapChain->GetResolution().height);

        // Auto-rotation
        if (autoRotate) {
            modelRotationY += 0.01f;
        }

        // Model matrix (rotation around center)
        matrices.model = Math::Mat4::translate(-modelCenter);
        matrices.model = Math::Mat4::rotateY(modelRotationY) * matrices.model;
        matrices.model = Math::Mat4::rotateX(modelRotationX) * matrices.model;
        matrices.model = Math::Mat4::translate(modelCenter) * matrices.model;

        // View matrix from camera
        matrices.view = camera.getViewMatrix();

        // Projection matrix
        matrices.projection = Math::Mat4::perspective(3.14159f / 4.0f, aspect, 0.1f, 1000.0f);

        // Update uniform buffer
        llgl_renderer->WriteBuffer(*uniformBuffer, 0, &matrices, sizeof(Matrices));

        // Rendering
        llgl_cmdBuffer->Begin();
        {
            // Set viewport and scissor rectangle
            llgl_cmdBuffer->SetViewport(llgl_swapChain->GetResolution());

            llgl_cmdBuffer->BeginRenderPass(*llgl_swapChain);
            {
                // Clear with depth
                llgl_cmdBuffer->Clear(LLGL::ClearFlags::ColorDepth, LLGL::ClearValue{ 0.1f, 0.1f, 0.15f, 1.0f });

                // Render model meshes
                const auto& meshes = model.getMeshes();
                const auto& materials = model.getMaterials();
                for (size_t i = 0; i < meshes.size(); i++) {
                    const auto& mesh = meshes[i];

                    // Check if mesh has texture
                    bool hasTexture = false;
                    LLGL::Texture* meshTexture = whiteTexture;

                    if (mesh.materialIndex < materials.size()) {
                        const auto& material = materials[mesh.materialIndex];
                        if (material.hasTexture && material.diffuseTexture) {
                            hasTexture = true;
                            meshTexture = material.diffuseTexture;
                        }
                    }

                    // Set appropriate pipeline
                    if (hasTexture) {
                        llgl_cmdBuffer->SetPipelineState(*modelPipeline);
                        llgl_cmdBuffer->SetResource(0, *uniformBuffer);
                        llgl_cmdBuffer->SetResource(1, *meshTexture);
                        llgl_cmdBuffer->SetResource(2, *modelSampler);
                    } else {
                        llgl_cmdBuffer->SetPipelineState(*modelNoTexPipeline);
                        llgl_cmdBuffer->SetResource(0, *uniformBuffer);
                    }

                    // Draw mesh
                    llgl_cmdBuffer->SetVertexBuffer(*mesh.vertexBuffer);
                    llgl_cmdBuffer->SetIndexBuffer(*mesh.indexBuffer);
                    llgl_cmdBuffer->DrawIndexed(mesh.indexCount(), 0);
                }

                // GUI Rendering with ImGui library
                NewFrameImGui(llgl_renderer, llgl_cmdBuffer);
                ImGui::NewFrame();

                // Model viewer controls
                ImGui::Begin("Model Viewer");
                ImGui::Text("Model: %s", modelPath.c_str());
                ImGui::Text("Meshes: %zu", meshes.size());
                ImGui::Text("Materials: %zu", materials.size());
                ImGui::Separator();

                ImGui::Text("Camera Controls:");
                ImGui::Text("  - Left click + drag: Rotate view");
                ImGui::Text("  - Mouse wheel: Zoom in/out");
                ImGui::Separator();

                ImGui::Checkbox("Auto Rotate", &autoRotate);
                ImGui::SliderFloat("Rotation Y", &modelRotationY, -3.14159f, 3.14159f);
                ImGui::SliderFloat("Rotation X", &modelRotationX, -1.5f, 1.5f);
                ImGui::Separator();

                ImGui::Text("Camera:");
                float camDistance = camera.getDistance();
                float camYaw = camera.getYaw();
                float camPitch = camera.getPitch();
                if (ImGui::SliderFloat("Distance", &camDistance, 0.1f, modelRadius * 10.0f)) {
                    camera.setDistance(camDistance);
                }
                if (ImGui::SliderFloat("Yaw", &camYaw, -3.14159f, 3.14159f)) {
                    camera.setYaw(camYaw);
                }
                if (ImGui::SliderFloat("Pitch", &camPitch, -1.5f, 1.5f)) {
                    camera.setPitch(camPitch);
                }

                if (ImGui::Button("Reset Camera")) {
                    camera.setTarget(modelCenter, modelRadius * 2.5f);
                    camera.setYaw(0.0f);
                    camera.setPitch(0.0f);
                    modelRotationX = 0.0f;
                    modelRotationY = 0.0f;
                }

                ImGui::End();

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

    // Cleanup
    model.release(llgl_renderer);
    ShutdownImGui(llgl_renderer);
    LLGL::RenderSystem::Unload(std::move(llgl_renderer));
    SDL_Quit();

    return 0;
}