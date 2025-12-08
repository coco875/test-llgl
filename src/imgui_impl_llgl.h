// dear imgui: Renderer Backend for LLGL (Low Level Graphics Library)
// This backend allows ImGui to render using LLGL, which abstracts OpenGL, Vulkan, D3D11, D3D12, Metal, etc.
// This needs to be used along with a Platform Backend (e.g. SDL2, GLFW, Win32, custom..)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'LLGL::Texture*' as void*/ImTextureID.
//  [X] Renderer: Large meshes support (64k+ vertices) with 32-bit indices.

#pragma once
#include "imgui.h"

#ifndef IMGUI_DISABLE

#include <LLGL/LLGL.h>

struct ImGui_ImplLLGL_InitInfo {
    LLGL::RenderSystem* RenderSystem = nullptr;
    LLGL::SwapChain* SwapChain = nullptr;
    LLGL::CommandBuffer* CommandBuffer = nullptr;
};

// Backend API
IMGUI_IMPL_API bool ImGui_ImplLLGL_Init(ImGui_ImplLLGL_InitInfo* info);
IMGUI_IMPL_API void ImGui_ImplLLGL_Shutdown();
IMGUI_IMPL_API void ImGui_ImplLLGL_NewFrame();
IMGUI_IMPL_API void ImGui_ImplLLGL_RenderDrawData(ImDrawData* draw_data);

// (Optional) Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool ImGui_ImplLLGL_CreateFontsTexture();
IMGUI_IMPL_API void ImGui_ImplLLGL_DestroyFontsTexture();
IMGUI_IMPL_API bool ImGui_ImplLLGL_CreateDeviceObjects();
IMGUI_IMPL_API void ImGui_ImplLLGL_DestroyDeviceObjects();

#endif // #ifndef IMGUI_DISABLE
