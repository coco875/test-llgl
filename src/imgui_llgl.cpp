// ImGui integration with LLGL using the unified LLGL backend
// This replaces the multi-backend approach with a single LLGL-based renderer

#include <LLGL/LLGL.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_llgl.h"

#include <SDL2/SDL_video.h>

#include "sdl_llgl.h"
#include "imgui_llgl.h"

void InitImGui(SDLSurface& wnd, LLGL::RenderSystemPtr& renderer, LLGL::SwapChain* swapChain,
               LLGL::CommandBuffer* cmdBuffer) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Initialize SDL2 platform backend based on renderer type
    switch (renderer->GetRendererID()) {
        case LLGL::RendererID::OpenGL:
        case LLGL::RendererID::OpenGLES:
            ImGui_ImplSDL2_InitForOpenGL(wnd.wnd, nullptr);
            break;
#ifdef LLGL_BUILD_RENDERER_VULKAN
        case LLGL::RendererID::Vulkan:
            ImGui_ImplSDL2_InitForVulkan(wnd.wnd);
            break;
#endif
#ifdef __APPLE__
        case LLGL::RendererID::Metal:
            ImGui_ImplSDL2_InitForMetal(wnd.wnd);
            break;
#endif
#ifdef WIN32
        case LLGL::RendererID::Direct3D11:
        case LLGL::RendererID::Direct3D12:
            ImGui_ImplSDL2_InitForD3D(wnd.wnd);
            break;
#endif
        default:
            ImGui_ImplSDL2_InitForOther(wnd.wnd);
            break;
    }

    // Initialize LLGL renderer backend
    ImGui_ImplLLGL_InitInfo initInfo = {};
    initInfo.RenderSystem = renderer.get();
    initInfo.SwapChain = swapChain;
    initInfo.CommandBuffer = cmdBuffer;
    ImGui_ImplLLGL_Init(&initInfo);
}

void NewFrameImGui() {
    ImGui_ImplLLGL_NewFrame();
    ImGui_ImplSDL2_NewFrame();
}

void RenderImGui(ImDrawData* data) {
    ImGui_ImplLLGL_RenderDrawData(data);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void ShutdownImGui() {
    ImGui_ImplLLGL_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}