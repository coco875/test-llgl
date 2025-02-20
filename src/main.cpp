// LLGL/SDL Test
// 2/16/25

#include <memory>

#include <LLGL/LLGL.h>
#include <LLGL/Utils/TypeNames.h>
#include <LLGL/Utils/VertexFormat.h>
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

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

// #include <glslang/Public/ShaderLang.h>

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
    
        SDL_Window*     wnd = nullptr;
        LLGL::Extent2D  size;
    private:
        std::string     title_;
};

CustomSurface::CustomSurface(const LLGL::Extent2D& size, const char* title) :
	title_ { title              },
	size  { size               }
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

bool PollEvents(LLGL::SwapChain* llgl_swapChain, std::shared_ptr<CustomSurface> surface) {
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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(wnd.wnd, nullptr);

    // Setup renderer backend
#ifdef __APPLE__
    ImGui_ImplOpenGL3_Init("#version 410 core");
#elif USE_OPENGLES
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 120");
#endif
}

static void ShutdownImGui()
{
    // Shutdown ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

static void RenderImGui()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGuiIO& io = ImGui::GetIO();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDL_Window* backupCurrentWindow = SDL_GL_GetCurrentWindow();
        SDL_GLContext backupCurrentContext = SDL_GL_GetCurrentContext();

        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();

        SDL_GL_MakeCurrent(backupCurrentWindow, backupCurrentContext);
    }
}

int main() {
    LLGL::Log::RegisterCallbackStd();
    LLGL::Log::Printf("Load: OpenGL\n");

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
    auto surface = std::make_shared<CustomSurface>(swapChainDesc.resolution, "LLGL SwapChain");

    SDL_GL_GetDrawableSize(surface->wnd, (int*) &window_width, (int*) &window_height);

    SDL_GLContext ctx = SDL_GL_CreateContext(surface->wnd);

    SDL_GL_MakeCurrent(surface->wnd, ctx);
    
    // Init LLGL
    LLGL::RenderSystemPtr llgl_renderer;
    LLGL::Report report;
    LLGL::RenderSystemDescriptor desc = {"OpenGL"};
#ifndef __APPLE__
    auto handle = LLGL::OpenGL::RenderSystemNativeHandle{(GLXContext) ctx};
    desc.nativeHandle = (void*)&handle;
    desc.nativeHandleSize = sizeof(LLGL::OpenGL::RenderSystemNativeHandle);
#else
    desc.nativeHandle = ctx;
    desc.nativeHandleSize = sizeof(void*);
#endif
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

    // swapChainDesc.samples = 4;
    LLGL::SwapChain* llgl_swapChain = llgl_renderer->CreateSwapChain(swapChainDesc, surface);

    // Print renderer information
    const auto& info = llgl_renderer->GetRendererInfo();

    LLGL::Log::Printf(
        "Renderer:             %s\n"
        "Device:               %s\n"
        "Vendor:               %s\n"
        "Shading Language:     %s\n"
        "Swap Chain Format:    %s\n"
        "Depth/Stencil Format: %s\n"
        "Resolution:           %u x %u\n"
        "Samples:              %u\n",
        info.rendererName.c_str(),
        info.deviceName.c_str(),
        info.vendorName.c_str(),
        info.shadingLanguageName.c_str(),
        LLGL::ToString(llgl_swapChain->GetColorFormat()),
        LLGL::ToString(llgl_swapChain->GetDepthStencilFormat()),
        llgl_swapChain->GetResolution().width,
        llgl_swapChain->GetResolution().height,
        llgl_swapChain->GetSamples()
    );

    // Vertex data structure
    struct Vertex
    {
        float   position[2];
        uint8_t color[4];
    };

    // Vertex data (3 vertices for our triangle)
    const float s = 0.5f;

    Vertex vertices[] =
    {
        { {  0,  s }, { 255, 0, 0, 255 } }, // 1st vertex: center-top, red
        { {  s, -s }, { 0, 255, 0, 255 } }, // 2nd vertex: right-bottom, green
        { { -s, -s }, { 0, 0, 255, 255 } }, // 3rd vertex: left-bottom, blue
    };

    // Vertex format
    LLGL::VertexFormat vertexFormat;

    // Append 2D float vector for position attribute
    vertexFormat.AppendAttribute({ "position", LLGL::Format::RG32Float });

    // Append 3D unsigned byte vector for color
    vertexFormat.AppendAttribute({ "color",    LLGL::Format::RGBA8UNorm });

    // Update stride in case out vertex structure is not 4-byte aligned
    vertexFormat.SetStride(sizeof(Vertex));

    // Create vertex buffer
    LLGL::BufferDescriptor vertexBufferDesc;
    {
        vertexBufferDesc.size           = sizeof(vertices);                 // Size (in bytes) of the vertex buffer
        vertexBufferDesc.bindFlags      = LLGL::BindFlags::VertexBuffer;    // Enables the buffer to be bound to a vertex buffer slot
        vertexBufferDesc.vertexAttribs  = vertexFormat.attributes;          // Vertex format layout
    }
    LLGL::Buffer* vertexBuffer = llgl_renderer->CreateBuffer(vertexBufferDesc, vertices);

    const auto& languages = llgl_renderer->GetRenderingCaps().shadingLanguages;

    LLGL::CommandBuffer* llgl_cmdBuffer = llgl_renderer->CreateCommandBuffer(LLGL::CommandBufferFlags::ImmediateSubmit);

    InitImGui(*surface);
    
    while (PollEvents(llgl_swapChain, surface)) {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

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
    
    LLGL::RenderSystem::Unload(std::move(llgl_renderer));
    ShutdownImGui();
    SDL_Quit();
    
    return 0;
}