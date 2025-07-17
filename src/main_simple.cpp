// Simplified main.cpp that demonstrates the direct ImGui LLGL implementation
// without the complex shader translation system

#include <memory>
#include <LLGL/LLGL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "sdl_llgl.h"
#include "imgui_llgl_direct.h"

LLGL::RenderSystemPtr llgl_renderer;

void print_info(LLGL::RenderSystemPtr& llgl_render, LLGL::SwapChain* llgl_swapChain) {
    // Print renderer information
    const auto& info = llgl_render->GetRendererInfo();

    LLGL::Log::Printf("Renderer:             %s\n"
                      "Device:               %s\n"
                      "Vendor:               %s\n"
                      "Shading Language:     %s\n"
                      "Resolution:           %u x %u\n"
                      "Samples:              %u\n",
                      info.rendererName.c_str(), info.deviceName.c_str(), info.vendorName.c_str(),
                      info.shadingLanguageName.c_str(), llgl_swapChain->GetResolution().width,
                      llgl_swapChain->GetResolution().height, llgl_swapChain->GetSamples());
}

#ifdef _WIN32
int SDL_main(int argc, char** argv) {
#else
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

    LLGL::RenderSystemDescriptor desc;
    auto surface = std::make_shared<SDLSurface>(swapChainDesc.resolution, "LLGL Direct ImGui Demo", rendererID, desc);
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
    auto llgl_cmdBuffer = llgl_renderer->CreateCommandBuffer(LLGL::CommandBufferFlags::ImmediateSubmit);

    print_info(llgl_renderer, llgl_swapChain);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup platform bindings for input
    ImGui_ImplSDL2_InitForOpenGL(surface->wnd, nullptr);

    // Initialize our direct LLGL renderer
    ImGuiLLGL::Config config;
    config.maxVertices = 65536;
    config.maxIndices = 65536;
    config.enableAlphaBlending = true;
    config.enableScissorTest = true;
    
    if (!ImGuiLLGL::Init(llgl_renderer, llgl_swapChain, config)) {
        LLGL::Log::Errorf("Failed to initialize ImGui LLGL direct renderer\n");
        return 1;
    }

    LLGL::Log::Printf("Successfully initialized ImGui LLGL direct renderer\n");

    // Demo state
    bool show_demo_window = true;
    bool show_direct_window = true;
    bool show_performance_window = true;
    float clear_color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    float rotation_speed = 1.0f;
    int frame_count = 0;

    while (surface->ProcessEvents(llgl_swapChain)) {
        frame_count++;

        // Start the Dear ImGui frame
        ImGuiLLGL::NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()!)
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves
        if (show_direct_window) {
            ImGui::Begin("Direct LLGL Implementation", &show_direct_window);

            ImGui::Text("This window is rendered using the direct LLGL ImGui implementation!");
            ImGui::Text("No traditional ImGui backends (OpenGL, Vulkan, etc.) are used.");
            
            ImGui::Separator();
            ImGui::Text("Features demonstrated:");
            ImGui::BulletText("Direct LLGL buffer and texture management");
            ImGui::BulletText("Custom GLSL shaders optimized for ImGui");
            ImGui::BulletText("Dynamic buffer resizing");
            ImGui::BulletText("Alpha blending and scissor testing");
            ImGui::BulletText("Font texture management");

            ImGui::Separator();
            ImGui::ColorEdit3("Clear color", clear_color);
            ImGui::SliderFloat("Rotation Speed", &rotation_speed, 0.0f, 5.0f);

            if (ImGui::Button("Close Application"))
                break;

            ImGui::End();
        }

        // 3. Show performance metrics
        if (show_performance_window) {
            ImGui::Begin("Performance Metrics", &show_performance_window);
            
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                       1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Text("Total frames rendered: %d", frame_count);
            
            ImDrawData* draw_data = ImGui::GetDrawData();
            if (draw_data) {
                ImGui::Text("Draw lists: %d", draw_data->CmdListsCount);
                ImGui::Text("Total vertices: %d", draw_data->TotalVtxCount);
                ImGui::Text("Total indices: %d", draw_data->TotalIdxCount);
            }

            // Display configuration
            ImGui::Separator();
            ImGui::Text("Renderer Configuration:");
            ImGui::Text("Max vertices: %zu", config.maxVertices);
            ImGui::Text("Max indices: %zu", config.maxIndices);
            ImGui::Text("Growth factor: %.2f", config.growthFactor);
            ImGui::Text("Alpha blending: %s", config.enableAlphaBlending ? "Enabled" : "Disabled");
            ImGui::Text("Scissor test: %s", config.enableScissorTest ? "Enabled" : "Disabled");

            ImGui::End();
        }

        // Rendering
        llgl_cmdBuffer->Begin();
        {
            // Set viewport and scissor rectangle
            llgl_cmdBuffer->SetViewport(llgl_swapChain->GetResolution());

            llgl_cmdBuffer->BeginRenderPass(*llgl_swapChain);
            {
                // Clear screen with the selected color
                LLGL::ClearValue clearValue;
                clearValue.color[0] = clear_color[0];
                clearValue.color[1] = clear_color[1];
                clearValue.color[2] = clear_color[2];
                clearValue.color[3] = clear_color[3];
                llgl_cmdBuffer->Clear(LLGL::ClearFlags::Color, clearValue);

                // Render ImGui using our direct implementation
                ImGui::Render();
                ImGuiLLGL::RenderDrawData(ImGui::GetDrawData(), llgl_cmdBuffer);
            }
            llgl_cmdBuffer->EndRenderPass();
        }
        llgl_cmdBuffer->End();

        // Present result on screen
        llgl_swapChain->Present();
    }

    // Cleanup
    LLGL::Log::Printf("Shutting down ImGui LLGL direct renderer\n");
    ImGuiLLGL::Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    
    LLGL::RenderSystem::Unload(std::move(llgl_renderer));
    SDL_Quit();

    return 0;
}