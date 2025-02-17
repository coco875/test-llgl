// LLGL/SDL Test
// 2/16/25

#include <LLGL/LLGL.h>
#include <LLGL/Platform/NativeHandle.h>

#include <SDL2/SDL.h>

#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengles2.h>

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_syswm.h>
#include <GL/glx.h>
#include <LLGL/Backend/OpenGL/NativeHandle.h>

class CustomSurface final : public LLGL::Surface {
    public:
        // Constructor and destructor
        CustomSurface(const LLGL::Extent2D& size, const char* title);
        ~CustomSurface();
        
        // Interface implementation
        bool GetNativeHandle(void* nativeHandle, std::size_t nativeHandleSize) override;
        LLGL::Extent2D GetContentSize() const override;
        bool AdaptForVideoMode(LLGL::Extent2D* resolution, bool* fullscreen) override;
        LLGL::Display* FindResidentDisplay() const override;
        
        // Additional class functions
        static bool PollEvents();
    
        SDL_Window*     wnd = nullptr;
    private:
        std::string     title_;
        LLGL::Extent2D  size_;
};

CustomSurface::CustomSurface(const LLGL::Extent2D& size, const char* title) :
	title_ { title              },
	size_  { size               }
{
    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL;
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
    nativeHandlePtr->display = wmInfo.info.x11.display;
    nativeHandlePtr->window = wmInfo.info.x11.window;
    return true;
}

LLGL::Extent2D CustomSurface::GetContentSize() const {
    return size_;
}

bool CustomSurface::AdaptForVideoMode(LLGL::Extent2D* resolution, bool* fullscreen) {
    return false;
}

LLGL::Display* CustomSurface::FindResidentDisplay() const {
    return nullptr;
}

bool CustomSurface::PollEvents(){
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }
    }
    return true;
}

int main() {
    setenv("SDL_VIDEODRIVER", "x11", 1);
    LLGL::Log::RegisterCallbackStd();
    LLGL::Log::Printf("Load: OpenGL\n");
    
    // Init SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    
    const uint32_t window_width = 800;
    const uint32_t window_height = 600;

    LLGL::SwapChainDescriptor swapChainDesc;
    swapChainDesc.resolution = { window_width, window_height };
    swapChainDesc.samples = 4;
    auto surface = std::make_shared<CustomSurface>(swapChainDesc.resolution, "LLGL SwapChain");

    SDL_GL_GetDrawableSize(surface->wnd, (int*) &window_width, (int*) &window_height);

    SDL_GLContext ctx = SDL_GL_CreateContext(surface->wnd);

    SDL_GL_MakeCurrent(surface->wnd, ctx);
    
    // Init LLGL
    LLGL::RenderSystemPtr llgl_renderer;
    LLGL::Report report;
    LLGL::RenderSystemDescriptor desc = {"OpenGL"};
    auto handle = LLGL::OpenGL::RenderSystemNativeHandle{(GLXContext) ctx};
    desc.nativeHandle = (void*)&handle;
    desc.nativeHandleSize = sizeof(LLGL::OpenGL::RenderSystemNativeHandle);
    llgl_renderer = LLGL::RenderSystem::Load("OpenGL", &report);
    
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

    // swapChainDesc.samples = 4;
    LLGL::SwapChain* llgl_swapChain = llgl_renderer->CreateSwapChain(swapChainDesc, surface);

    LLGL::CommandBuffer* llgl_cmdBuffer = llgl_renderer->CreateCommandBuffer(LLGL::CommandBufferFlags::ImmediateSubmit);
    
    while (CustomSurface::PollEvents()) {
        // Clear background
        llgl_cmdBuffer->Begin();
        llgl_cmdBuffer->BeginRenderPass(*llgl_swapChain);
        llgl_cmdBuffer->Clear(LLGL::ClearFlags::Color, LLGL::ClearValue{ 0.2f, 0.2f, 0.4f, 1.0f });
        llgl_cmdBuffer->EndRenderPass();
        llgl_cmdBuffer->End();
        llgl_swapChain->Present();
    }
    
    LLGL::RenderSystem::Unload(std::move(llgl_renderer));
    
    SDL_Quit();
    
    return 0;
}