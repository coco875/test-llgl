// LLGL/SDL Test
// 2/16/25

#include <memory>

#include <LLGL/LLGL.h>

#ifndef __APPLE__
#include <GL/glx.h>
#endif

#include "imgui.h"

#include "sdl_llgl.h"
#include "imgui_llgl.h"

int main() {
    LLGL::Log::RegisterCallbackStd();
    LLGL::Log::Printf("Load: OpenGL\n");

    int rendererID = LLGL::RendererID::Metal;

#ifndef __APPLE__
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

    auto llgl_cmdBuffer = llgl_renderer->CreateCommandBuffer(LLGL::CommandBufferFlags::ImmediateSubmit);

    InitImGui(*surface, llgl_renderer, llgl_swapChain);

    while (surface->ProcessEvents(llgl_swapChain)) {
        // Rendering
        llgl_cmdBuffer->Begin();
        {
            llgl_cmdBuffer->BeginRenderPass(*llgl_swapChain);
            {
                // llgl_cmdBuffer->Clear(LLGL::ClearFlags::Color, LLGL::ClearValue{ 0.0f, 0.2f, 0.2f, 1.0f });

                // GUI Rendering with ImGui library
                llgl_cmdBuffer->PushDebugGroup("RenderGUI");
                {
                    // Start the Dear ImGui frame
                    NewFrameImGui(llgl_renderer, llgl_cmdBuffer);
                    ImGui::NewFrame();

                    // Show ImGui's demo window
                    ImGui::ShowDemoWindow();

                    // GUI Rendering
                    ImGui::Render();
                    RenderImGui(ImGui::GetDrawData(), llgl_renderer, llgl_cmdBuffer);
                }
                llgl_cmdBuffer->PopDebugGroup();
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