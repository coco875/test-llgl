#ifndef IMGUI_LLGL_H
#define IMGUI_LLGL_H

#ifdef __APPLE__
void Imgui_Metal_llgl_Shutdown();
void Imgui_Metal_llgl_NewFrame(LLGL::CommandBuffer* cmdBuffer);
void Imgui_Metal_llgl_RenderDrawData(ImDrawData* data);
void Imgui_Metal_llgl_Init(LLGL::RenderSystemPtr& renderer);
#endif

void InitImGui(SDLSurface& wnd, LLGL::RenderSystemPtr& renderer, LLGL::SwapChain* swapChain,
               LLGL::CommandBuffer* cmdBuffer);
void NewFrameImGui(LLGL::RenderSystemPtr& renderer, LLGL::CommandBuffer* cmdBuffer);
void RenderImGui(ImDrawData* data, LLGL::RenderSystemPtr& renderer, LLGL::CommandBuffer* llgl_cmdBuffer);
void ShutdownImGui(LLGL::RenderSystemPtr& renderer);

#endif