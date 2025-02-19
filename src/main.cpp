// LLGL/SDL Test
// 2/16/25

#include <memory>

#include <LLGL/LLGL.h>
#include <LLGL/Platform/NativeHandle.h>

#include <SDL2/SDL.h>

#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengl.h>

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_syswm.h>
#ifndef __APPLE__
#include <GL/glx.h>
#endif
#include <LLGL/Backend/OpenGL/NativeHandle.h>
#ifdef __APPLE__
#include <LLGL/Backend/Metal/NativeHandle.h>
#endif

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#ifdef __APPLE__
#include "imgui_impl_metal.h"
#endif

LLGL::RenderSystemPtr llgl_renderer;
LLGL::SwapChain* llgl_swapChain;
LLGL::CommandBuffer* llgl_cmdBuffer;

class CustomSurface final : public LLGL::Surface {
    public:
        // Constructor and destructor
        CustomSurface(const LLGL::Extent2D& size, const char* title, int rendererID);
        ~CustomSurface();
        
        // Interface implementation
        bool GetNativeHandle(void* nativeHandle, std::size_t nativeHandleSize) override;
        LLGL::Extent2D GetContentSize() const override;
        bool AdaptForVideoMode(LLGL::Extent2D* resolution, bool* fullscreen) override;
        LLGL::Display* FindResidentDisplay() const override;
    
        SDL_Window*     wnd = nullptr;
        LLGL::Extent2D  size;
    private:
        std::string     title_;
};

CustomSurface::CustomSurface(const LLGL::Extent2D& size, const char* title, int rendererID) :
	title_ { title              },
	size  { size               }
{   
    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    switch (rendererID) {
        case LLGL::RendererID::OpenGL:
            flags |= SDL_WINDOW_OPENGL;
            break;
        case LLGL::RendererID::OpenGLES:
            flags |= SDL_WINDOW_OPENGL;
            break;
        case LLGL::RendererID::Metal:
            flags |= SDL_WINDOW_METAL;
            break;
        default:
            break;
    }
    wnd = SDL_CreateWindow(title, 400, 200, (int)size.width, (int)size.height, flags);
    if (wnd == nullptr) {
        LLGL::Log::Errorf("Failed to create SDL2 window\n");
        exit(1);
    }
}

CustomSurface::~CustomSurface() {
}

bool CustomSurface::GetNativeHandle(void* nativeHandle, std::size_t nativeHandleSize) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(wnd, &wmInfo);
    auto* nativeHandlePtr = static_cast<LLGL::NativeHandle*>(nativeHandle);
#ifdef __APPLE__
    nativeHandlePtr->responder = wmInfo.info.cocoa.window;
#else
    nativeHandlePtr->display = wmInfo.info.x11.display;
    nativeHandlePtr->window = wmInfo.info.x11.window;
#endif
    return true;
}

LLGL::Extent2D CustomSurface::GetContentSize() const {
    return size;
}

bool CustomSurface::AdaptForVideoMode(LLGL::Extent2D* resolution, bool* fullscreen) {
    return false;
}

LLGL::Display* CustomSurface::FindResidentDisplay() const {
    return nullptr;
}

bool PollEvents(std::shared_ptr<CustomSurface> surface) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
            uint32_t width = event.window.data1;
            uint32_t height = event.window.data2;
            surface->size = { width, height };
            llgl_swapChain->ResizeBuffers(surface->size);
        }
        ImGui_ImplSDL2_ProcessEvent(&event);
    }
    return true;
}

static void InitImGui(CustomSurface& wnd)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // | ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    void* ctx = nullptr;

    switch (auto a = llgl_renderer->GetRendererID()) {
        case LLGL::RendererID::OpenGL:
                ImGui_ImplSDL2_InitForOpenGL(wnd.wnd, ctx);
                // Setup renderer backend
#ifdef __APPLE__
                ImGui_ImplOpenGL3_Init("#version 410 core");
#else
                ImGui_ImplOpenGL3_Init("#version 120");
#endif
            break;
        case LLGL::RendererID::OpenGLES:
            ImGui_ImplSDL2_InitForOpenGL(wnd.wnd, ctx);
            ImGui_ImplOpenGL3_Init("#version 300 es");
            break;
        case LLGL::RendererID::Metal:
            ImGui_ImplSDL2_InitForMetal(wnd.wnd);
            break;
        default:
            io.BackendRendererName = "imgui_impl_null";
            break;
    }
}

static void ShutdownImGui()
{
    // Shutdown ImGui
    switch (llgl_renderer->GetRendererID()) {
        case LLGL::RendererID::OpenGL:
            ImGui_ImplOpenGL3_Shutdown();
            break;
        case LLGL::RendererID::OpenGLES:
            ImGui_ImplOpenGL3_Shutdown();
            break;
        case LLGL::RendererID::Metal:
            // ImGui_ImplMetal_Shutdown();
            break;
        default:
            break;
    }
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

static void RenderImGui()
{
    ImGui::Render();
    switch (llgl_renderer->GetRendererID()) {
        case LLGL::RendererID::OpenGL:
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case LLGL::RendererID::OpenGLES:
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
#ifdef __APPLE__
        case LLGL::RendererID::Metal:
            LLGL::Metal::CommandBufferNativeHandle cmdBuffer;
            llgl_cmdBuffer->GetNativeHandle(&cmdBuffer, sizeof(LLGL::Metal::CommandBufferNativeHandle));

            ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), (MTL::CommandBuffer *) cmdBuffer.commandBuffer, nullptr);
            break;
#endif
        default:
            break;
    }
    ImGuiIO& io = ImGui::GetIO();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        if (llgl_renderer->GetRendererID() == LLGL::RendererID::OpenGL || llgl_renderer->GetRendererID() == LLGL::RendererID::OpenGLES) {
            SDL_Window* backupCurrentWindow = SDL_GL_GetCurrentWindow();
            SDL_GLContext backupCurrentContext = SDL_GL_GetCurrentContext();

            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            SDL_GL_MakeCurrent(backupCurrentWindow, backupCurrentContext);
        } else {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
}

static void NewFrameImGui()
{
    switch (llgl_renderer->GetRendererID()) {
        case LLGL::RendererID::OpenGL:
            ImGui_ImplOpenGL3_NewFrame();
            break;
        case LLGL::RendererID::OpenGLES:
            ImGui_ImplOpenGL3_NewFrame();
            break;
#ifdef __APPLE__
        case LLGL::RendererID::Metal:
            ImGui_ImplMetal_NewFrame(((MTSwapChain*)llgl_swapChain).GetNativeRenderPass());
            break;
#endif
        default:
            break;
    }
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

int main() {
    LLGL::Log::RegisterCallbackStd();
    LLGL::Log::Printf("Load: OpenGL\n");

    bool useOpenGL = true;

#ifndef __APPLE__
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");
#endif
    
    // Init SDL
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    
    const uint32_t window_width = 800;
    const uint32_t window_height = 600;

    LLGL::SwapChainDescriptor swapChainDesc;
    swapChainDesc.resolution = { window_width, window_height };
    swapChainDesc.samples = 4;
    auto surface = std::make_shared<CustomSurface>(swapChainDesc.resolution, "LLGL SwapChain", LLGL::RendererID::OpenGL);
    LLGL::RenderSystemDescriptor desc;
    if (useOpenGL) {
        SDL_GL_GetDrawableSize(surface->wnd, (int*) &window_width, (int*) &window_height);

        SDL_GLContext ctx = SDL_GL_CreateContext(surface->wnd);

        SDL_GL_MakeCurrent(surface->wnd, ctx);
        
        // Init LLGL
        desc = {"OpenGL"};
#ifndef __APPLE__
        auto handle = LLGL::OpenGL::RenderSystemNativeHandle{(GLXContext) ctx};
#else
        auto handle = LLGL::OpenGL::RenderSystemNativeHandle{(void*) ctx};
#endif
        desc.nativeHandle = (void*)&handle;
        desc.nativeHandleSize = sizeof(LLGL::OpenGL::RenderSystemNativeHandle);
    } else {
        desc = {"Metal"};
    }
    LLGL::Report report;
    llgl_renderer = LLGL::RenderSystem::Load(desc, &report);
    
    // Create SDL window and LLGL swap-chain
    if (!llgl_renderer)     {
        auto a = report.GetText();
        LLGL::Log::Errorf("Failed to load \"%s\" module. Falling back to \"Null\" device.\n", "OpenGL");
        LLGL::Log::Errorf("Reason for failure: %s", report.HasErrors() ? report.GetText() : "Unknown\n");
        llgl_renderer = LLGL::RenderSystem::Load("Null");
        if (!llgl_renderer) {
            LLGL::Log::Errorf("Failed to load \"Null\" module. Exiting.\n");
            return 1;
        }
    }

    swapChainDesc.samples = 4;
    llgl_swapChain = llgl_renderer->CreateSwapChain(swapChainDesc, surface);

    llgl_cmdBuffer = llgl_renderer->CreateCommandBuffer(LLGL::CommandBufferFlags::ImmediateSubmit);

    InitImGui(*surface);
    
    while (PollEvents(surface)) {
        // Start the Dear ImGui frame
        NewFrameImGui();

        // Show ImGui's demo window
        ImGui::ShowDemoWindow();

        // Rendering
        llgl_cmdBuffer->Begin();
        {
            llgl_cmdBuffer->BeginRenderPass(*llgl_swapChain);
            {
                llgl_cmdBuffer->Clear(LLGL::ClearFlags::Color, LLGL::ClearValue{ 0.0f, 0.0f, 0.0f, 1.0f });

                // GUI Rendering
                RenderImGui();
            }
            llgl_cmdBuffer->EndRenderPass();
        }
        llgl_cmdBuffer->End();

        // Present result on screen
        llgl_swapChain->Present();
    }
    
    ShutdownImGui();
    LLGL::RenderSystem::Unload(std::move(llgl_renderer));
    SDL_Quit();
    
    return 0;
}