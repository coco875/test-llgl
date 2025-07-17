// Example usage of the direct ImGui LLGL implementation
// This demonstrates how to use the new direct renderer instead of traditional backends

#include <memory>
#include <LLGL/LLGL.h>
#include <stb_image.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "sdl_llgl.h"
#include "imgui_llgl_direct.h"

LLGL::RenderSystemPtr llgl_renderer;

int main(int argc, char* argv[]) {
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
    auto surface = std::make_shared<SDLSurface>(swapChainDesc.resolution, "LLGL Direct ImGui Example", rendererID, desc);
    desc.flags |= LLGL::RenderSystemFlags::DebugDevice;
    LLGL::Report report;
    llgl_renderer = LLGL::RenderSystem::Load(desc, &report);

    if (!llgl_renderer) {
        LLGL::Log::Errorf("Failed to load render system: %s\\n", report.GetText());
        return 1;
    }

    auto llgl_swapChain = llgl_renderer->CreateSwapChain(swapChainDesc, surface);
    auto llgl_cmdBuffer = llgl_renderer->CreateCommandBuffer(LLGL::CommandBufferFlags::ImmediateSubmit);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Initialize SDL backend for input handling
    ImGui_ImplSDL2_InitForOpenGL(surface->wnd, nullptr);

    // Initialize our direct LLGL renderer
    ImGuiLLGL::Config config;
    config.maxVertices = 65536;
    config.maxIndices = 65536;
    config.enableAlphaBlending = true;
    config.enableScissorTest = true;
    
    if (!ImGuiLLGL::Init(llgl_renderer, llgl_swapChain, config)) {
        LLGL::Log::Errorf("Failed to initialize ImGui LLGL direct renderer\\n");
        return 1;
    }

    // Demo variables
    bool showDemoWindow = true;
    bool showCustomWindow = true;
    float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    
    // Main loop
    while (surface->ProcessEvents(llgl_swapChain)) {
        // Start the Dear ImGui frame
        ImGuiLLGL::NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Show demo window
        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        // Show custom window
        if (showCustomWindow) {
            ImGui::Begin("Direct LLGL ImGui Example", &showCustomWindow);
            
            ImGui::Text("This is a custom window rendered using the direct LLGL ImGui implementation!");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                       1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            
            ImGui::ColorEdit3("Clear Color", clearColor);
            
            if (ImGui::Button("Close Application")) {
                break;
            }
            
            ImGui::End();
        }

        // Rendering
        llgl_cmdBuffer->Begin();
        {
            // Set viewport
            llgl_cmdBuffer->SetViewport(llgl_swapChain->GetResolution());

            // Begin render pass
            llgl_cmdBuffer->BeginRenderPass(*llgl_swapChain);
            {
                // Clear screen
                LLGL::ClearValue clearValue;
                clearValue.color[0] = clearColor[0];
                clearValue.color[1] = clearColor[1];
                clearValue.color[2] = clearColor[2];
                clearValue.color[3] = clearColor[3];
                llgl_cmdBuffer->Clear(LLGL::ClearFlags::Color, clearValue);

                // Render ImGui
                ImGui::Render();
                ImGuiLLGL::RenderDrawData(ImGui::GetDrawData(), llgl_cmdBuffer);
            }
            llgl_cmdBuffer->EndRenderPass();
        }
        llgl_cmdBuffer->End();

        // Present
        llgl_swapChain->Present();
    }

    // Cleanup
    ImGuiLLGL::Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    
    LLGL::RenderSystem::Unload(std::move(llgl_renderer));
    SDL_Quit();

    return 0;
}