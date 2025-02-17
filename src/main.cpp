// LLGL/SDL Test
// 2/16/25

#include <LLGL/LLGL.h>
#include <LLGL/Platform/NativeHandle.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11 1
#include <GLFW/glfw3native.h>

// #include <SDL2/SDL.h>

// #define GL_GLEXT_PROTOTYPES 1
// #include <SDL2/SDL_opengles2.h>

// #include <SDL2/SDL_video.h>
// #include <SDL2/SDL_syswm.h>


// class CustomSurface final : public LLGL::Surface {
//     public:
//         // Constructor and destructor
//         CustomSurface(const LLGL::Extent2D& size, const char* title);
//         ~CustomSurface();
        
//         // Interface implementation
//         bool GetNativeHandle(void* nativeHandle, std::size_t nativeHandleSize) override;
//         LLGL::Extent2D GetContentSize() const override;
//         bool AdaptForVideoMode(LLGL::Extent2D* resolution, bool* fullscreen) override;
//         LLGL::Display* FindResidentDisplay() const override;
        
//         // Additional class functions
//         static bool PollEvents();
    
//     private:
//         std::string     title_;
//         LLGL::Extent2D  size_;
//         SDL_Window*     wnd_ = nullptr;
// };

// CustomSurface::CustomSurface(const LLGL::Extent2D& size, const char* title) :
// 	title_ { title              },
// 	size_  { size               }
// {
//     Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL;
//     wnd_ = SDL_CreateWindow(title, 400, 200, (int)size.width, (int)size.height, flags);
//     if (wnd_ == nullptr) {
//         LLGL::Log::Errorf("Failed to create SDL2 window\n");
//         exit(1);
//     }
// }

// CustomSurface::~CustomSurface() {
// }

// bool CustomSurface::GetNativeHandle(void* nativeHandle, std::size_t nativeHandleSize) {
//     SDL_SysWMinfo wmInfo;
//     SDL_VERSION(&wmInfo.version);
//     SDL_GetWindowWMInfo(wnd_, &wmInfo);
//     auto* nativeHandlePtr = static_cast<LLGL::NativeHandle*>(nativeHandle);
//     nativeHandlePtr->display = wmInfo.info.x11.display;
//     nativeHandlePtr->window = wmInfo.info.x11.window;
//     return true;
// }

// LLGL::Extent2D CustomSurface::GetContentSize() const {
//     return size_;
// }

// bool CustomSurface::AdaptForVideoMode(LLGL::Extent2D* resolution, bool* fullscreen) {
//     return false;
// }

// LLGL::Display* CustomSurface::FindResidentDisplay() const {
//     return nullptr;
// }

// bool CustomSurface::PollEvents(){
//     SDL_Event event;
//     while (SDL_PollEvent(&event)) {
//         if (event.type == SDL_QUIT) {
//             return false;
//         }
//     }
//     return true;
// }

class CustomSurface : public LLGL::Surface {
public:
    // Constructor and destructor
    CustomSurface(const LLGL::Extent2D& size, const char* title);
    ~CustomSurface();
    
    // Interface implementation
    bool GetNativeHandle(void* nativeHandle, std::size_t nativeHandleSize) override;
    LLGL::Extent2D GetContentSize() const override;
    bool AdaptForVideoMode(LLGL::Extent2D* resolution, bool* fullscreen) override;
    void ResetPixelFormat() override;
    LLGL::Display* FindResidentDisplay() const override;
    
    // Additional class functions
    void PollEvents();
    
private:
    GLFWwindow* CreateGLFWWindow();
    
    std::string    title_;
    LLGL::Extent2D size_;
    GLFWwindow*    wnd_ = nullptr; // GLFW window pointer
};

CustomSurface::CustomSurface(const LLGL::Extent2D& size, const char* title) :
title_ { title              },
size_  { size               },
wnd_   { CreateGLFWWindow() }
{
}

CustomSurface::~CustomSurface() {
	// Destroy GLFW window
	glfwDestroyWindow(wnd_);
}

void CustomSurface::ResetPixelFormat() {
	// Destroy and recreate GLFW window
	glfwDestroyWindow(wnd_);
	wnd_ = CreateGLFWWindow();
}

bool CustomSurface::GetNativeHandle(void* nativeHandle, std::size_t nativeHandleSize) {
	// This function must always return a valid native handle!
	auto handle = reinterpret_cast<LLGL::NativeHandle*>(nativeHandle);
    handle->display = glfwGetX11Display();
    handle->window = glfwGetX11Window(wnd_);
}

GLFWwindow* CustomSurface::CreateGLFWWindow() {
	// Create GLFW window with class members
	auto wnd = glfwCreateWindow(size_.width, size_.height, title_.c_str(), nullptr, nullptr);
	if (!wnd) {
		throw std::runtime_error("failed to create GLFW window");
	}
	return wnd;
}

LLGL::Extent2D CustomSurface::GetContentSize() const {
	// Actually the client-area size of the window must be returned,
	// but for this example the entire window size is sufficient.
	return size_;
}

bool CustomSurface::AdaptForVideoMode(LLGL::Extent2D* resolution, bool* fullscreen) {
	// Resize GLFW window for the new video mode resolution.
	size_ = *resolution;
	glfwSetWindowSize(wnd_, size_.width, size_.height);
	return true;
}

LLGL::Display* CustomSurface::FindResidentDisplay() const {
	// Just return primary display
	return nullptr;
}

int main() {
    setenv("SDL_VIDEODRIVER", "x11", 1);
    LLGL::Log::RegisterCallbackStd();
    LLGL::Log::Printf("Load: OpenGL\n");
    
    // Init SDL
    // SDL_Init(SDL_INIT_VIDEO);
    // SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");
    
    // Init LLGL
    LLGL::RenderSystemPtr llgl_renderer;
    LLGL::Report report;
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
    
    const uint32_t window_width = 800;
    const uint32_t window_height = 600;

    LLGL::SwapChainDescriptor swapChainDesc;
    swapChainDesc.resolution = { window_width, window_height };
    swapChainDesc.samples = 4;
    auto surface = std::make_shared<CustomSurface>(swapChainDesc.resolution, "LLGL SwapChain");
    LLGL::SwapChain* llgl_swapChain = llgl_renderer->CreateSwapChain(swapChainDesc, surface);

    LLGL::CommandBuffer* llgl_cmdBuffer = llgl_renderer->CreateCommandBuffer(LLGL::CommandBufferFlags::ImmediateSubmit);
    
    while (true) {
        // Clear background
        llgl_cmdBuffer->Begin();
        llgl_cmdBuffer->BeginRenderPass(*llgl_swapChain);
        llgl_cmdBuffer->Clear(LLGL::ClearFlags::Color, LLGL::ClearValue{ 0.2f, 0.2f, 0.4f, 1.0f });
        llgl_cmdBuffer->EndRenderPass();
        llgl_cmdBuffer->End();
        llgl_swapChain->Present();
    }
    
    LLGL::RenderSystem::Unload(std::move(llgl_renderer));
    
    // SDL_Quit();
    
    return 0;
}