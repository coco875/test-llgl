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
#include <LLGL/Backend/Vulkan/NativeHandle.h>
#ifdef __APPLE__
#include <LLGL/Backend/Metal/NativeHandle.h>
#endif

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_vulkan.h"
#ifdef __APPLE__
#include "imgui_impl_metal.h"
#endif

#ifdef __APPLE__
void Imgui_Metal_llgl_Shutdown();
void Imgui_Metal_llgl_NewFrame();
void Imgui_Metal_llgl_EndFrame(ImDrawData* data);
void Imgui_Metal_llgl_Init();
extern "C" float Imgui_Metal_llgl_GetContentScale(NSWindow *wnd_);
#endif

LLGL::RenderSystemPtr llgl_renderer;
LLGL::SwapChain* llgl_swapChain;
LLGL::CommandBuffer* llgl_cmdBuffer;

static VkFormat GetVulkanColorFormat(LLGL::Format format)
{
    return (format == LLGL::Format::BGRA8UNorm ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_R8G8B8A8_UNORM);
}

static VkFormat GetVulkanDepthStencilFormat(LLGL::Format format)
{
    return (format == LLGL::Format::D32Float ? VK_FORMAT_D32_SFLOAT : VK_FORMAT_D24_UNORM_S8_UINT);
}

static VkRenderPass CreateVulkanRenderPass(VkDevice vulkanDevice)
{
    VkAttachmentDescription vulkanAttachmentDescs[2] = {};
    {
        vulkanAttachmentDescs[0].flags          = 0;
        vulkanAttachmentDescs[0].format         = GetVulkanColorFormat(llgl_swapChain->GetColorFormat());
        vulkanAttachmentDescs[0].samples        = VK_SAMPLE_COUNT_1_BIT;
        vulkanAttachmentDescs[0].loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
        vulkanAttachmentDescs[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        vulkanAttachmentDescs[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_LOAD;
        vulkanAttachmentDescs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        vulkanAttachmentDescs[0].initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        vulkanAttachmentDescs[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    {
        vulkanAttachmentDescs[1].flags          = 0;
        vulkanAttachmentDescs[1].format         = GetVulkanDepthStencilFormat(llgl_swapChain->GetDepthStencilFormat());
        vulkanAttachmentDescs[1].samples        = VK_SAMPLE_COUNT_1_BIT;
        vulkanAttachmentDescs[1].loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
        vulkanAttachmentDescs[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        vulkanAttachmentDescs[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_LOAD;
        vulkanAttachmentDescs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        vulkanAttachmentDescs[1].initialLayout  = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
        vulkanAttachmentDescs[1].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    VkAttachmentReference vulkanAttachmentRefs[2] = {};
    {
        vulkanAttachmentRefs[0].attachment  = 0;
        vulkanAttachmentRefs[0].layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    {
        vulkanAttachmentRefs[1].attachment  = 1;
        vulkanAttachmentRefs[1].layout      = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
    }

    VkSubpassDescription vulkanSubpassDescs[1] = {};
    {
        vulkanSubpassDescs[0].flags                     = 0;
        vulkanSubpassDescs[0].pipelineBindPoint         = VK_PIPELINE_BIND_POINT_GRAPHICS;
        vulkanSubpassDescs[0].inputAttachmentCount      = 0;
        vulkanSubpassDescs[0].pInputAttachments         = nullptr;
        vulkanSubpassDescs[0].colorAttachmentCount      = 1;
        vulkanSubpassDescs[0].pColorAttachments         = &vulkanAttachmentRefs[0];
        vulkanSubpassDescs[0].pResolveAttachments       = nullptr;
        vulkanSubpassDescs[0].pDepthStencilAttachment   = &vulkanAttachmentRefs[1];
        vulkanSubpassDescs[0].preserveAttachmentCount   = 0;
        vulkanSubpassDescs[0].pPreserveAttachments      = nullptr;
    }

    VkRenderPassCreateInfo vulkanRenderPassInfo = {};
    {
        vulkanRenderPassInfo.sType              = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        vulkanRenderPassInfo.pNext              = nullptr;
        vulkanRenderPassInfo.flags              = 0;
        vulkanRenderPassInfo.attachmentCount    = sizeof(vulkanAttachmentDescs)/sizeof(vulkanAttachmentDescs[0]);
        vulkanRenderPassInfo.pAttachments       = vulkanAttachmentDescs;
        vulkanRenderPassInfo.subpassCount       = sizeof(vulkanSubpassDescs)/sizeof(vulkanSubpassDescs[0]);
        vulkanRenderPassInfo.pSubpasses         = vulkanSubpassDescs;
        vulkanRenderPassInfo.dependencyCount    = 0;
        vulkanRenderPassInfo.pDependencies      = nullptr;
    }
    VkRenderPass vulkanRenderPass = VK_NULL_HANDLE;
    VkResult result = vkCreateRenderPass(vulkanDevice, &vulkanRenderPassInfo, nullptr, &vulkanRenderPass);
    assert(result == VK_SUCCESS);
    return vulkanRenderPass;
}

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
        case LLGL::RendererID::Vulkan:
            flags |= SDL_WINDOW_VULKAN;
            break;
        default:
            break;
    }
    wnd = SDL_CreateWindow(title, 400, 200, (int)size.width, (int)size.height, flags);
    if (wnd == nullptr) {
        LLGL::Log::Errorf("Failed to create SDL2 window\n");
        exit(1);
    }

#ifdef __APPLE__
    if (wnd) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(wnd, &wmInfo);
        float scale = Imgui_Metal_llgl_GetContentScale(wmInfo.info.cocoa.window);
        this->size = LLGL::Extent2D(size.width * scale, size.height * scale);
    }
#endif
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
#ifdef __APPLE__
        case LLGL::RendererID::Metal:
            ImGui_ImplSDL2_InitForMetal(wnd.wnd);
            Imgui_Metal_llgl_Init();
            break;
#endif
        case LLGL::RendererID::Vulkan: {
            ImGui_ImplSDL2_InitForVulkan(wnd.wnd);
            LLGL::Vulkan::RenderSystemNativeHandle instance;
            llgl_renderer->GetNativeHandle(&instance, sizeof(LLGL::Vulkan::RenderSystemNativeHandle));
            VkDevice vulkanDevice = instance.device;

            // Create Vulkan render pass
            VkRenderPass vulkanRenderPass = CreateVulkanRenderPass(vulkanDevice);

            ImGui_ImplVulkan_InitInfo initInfo = {};
            {
                initInfo.Instance           = instance.instance;
                initInfo.PhysicalDevice     = instance.physicalDevice;
                initInfo.Device             = instance.device;
                initInfo.QueueFamily        = instance.queueFamily;
                initInfo.Queue              = instance.queue;
                initInfo.DescriptorPool     = VK_NULL_HANDLE;
                initInfo.DescriptorPoolSize = 64;
                initInfo.RenderPass         = vulkanRenderPass;
                initInfo.MinImageCount      = 2;
                initInfo.ImageCount         = 2;
                initInfo.MSAASamples        = VK_SAMPLE_COUNT_1_BIT;
            }
            ImGui_ImplVulkan_Init(&initInfo);
            break;
        }
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
#ifdef __APPLE__
        case LLGL::RendererID::Metal:
            Imgui_Metal_llgl_Shutdown();
            break;
#endif
        case LLGL::RendererID::Vulkan:
            ImGui_ImplVulkan_Shutdown();
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
        case LLGL::RendererID::Metal: {
            Imgui_Metal_llgl_EndFrame(ImGui::GetDrawData());
            break;
        }
#endif
        case LLGL::RendererID::Vulkan: {
            LLGL::Vulkan::CommandBufferNativeHandle cmdBuffer;
            llgl_cmdBuffer->GetNativeHandle(&cmdBuffer, sizeof(LLGL::Vulkan::CommandBufferNativeHandle));
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer.commandBuffer);
            break;
        }
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
    ImGui_ImplSDL2_NewFrame();
    switch (llgl_renderer->GetRendererID()) {
        case LLGL::RendererID::OpenGL:
            ImGui_ImplOpenGL3_NewFrame();
            break;
        case LLGL::RendererID::OpenGLES:
            ImGui_ImplOpenGL3_NewFrame();
            break;
#ifdef __APPLE__
        case LLGL::RendererID::Metal:
            Imgui_Metal_llgl_NewFrame();
            break;
#endif
        case LLGL::RendererID::Vulkan:
            ImGui_ImplVulkan_NewFrame();
            break;
        default:
            break;
    }
    ImGui::NewFrame();
}

bool PollEvents(std::shared_ptr<CustomSurface> surface) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }
        if (event.type == SDL_WINDOWEVENT && (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
            uint32_t width = event.window.data1;
            uint32_t height = event.window.data2;
#ifdef __APPLE__
            if (surface->wnd) {
                SDL_SysWMinfo wmInfo;
                SDL_VERSION(&wmInfo.version);
                SDL_GetWindowWMInfo(surface->wnd, &wmInfo);
                float scale = Imgui_Metal_llgl_GetContentScale(wmInfo.info.cocoa.window);
                width *= scale;
                height *= scale;
            }
#endif
            surface->size = { width, height };
            llgl_swapChain->ResizeBuffers(surface->size);
        }
        ImGui_ImplSDL2_ProcessEvent(&event);
    }
    return true;
}

int main() {
    LLGL::Log::RegisterCallbackStd();
    LLGL::Log::Printf("Load: OpenGL\n");

    int rendererID = LLGL::RendererID::Vulkan;

    bool useOpenGL = rendererID == LLGL::RendererID::OpenGL || rendererID == LLGL::RendererID::OpenGLES;

#ifndef __APPLE__
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");
#endif
    
    // Init SDL
    SDL_Init(SDL_INIT_VIDEO);

    if (useOpenGL) {
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    }

    
    const uint32_t window_width = 800;
    const uint32_t window_height = 600;

    LLGL::SwapChainDescriptor swapChainDesc;
    swapChainDesc.resolution = { window_width, window_height };
    swapChainDesc.samples = 4;

    auto surface = std::make_shared<CustomSurface>(swapChainDesc.resolution, "LLGL SwapChain", rendererID);
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
    } else if (rendererID == LLGL::RendererID::Vulkan) {
        desc = {"Vulkan"};
    } else if (rendererID == LLGL::RendererID::Metal) {
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
        // Rendering
        llgl_cmdBuffer->Begin();
        {
            llgl_cmdBuffer->BeginRenderPass(*llgl_swapChain);
            {
                llgl_cmdBuffer->Clear(LLGL::ClearFlags::Color, LLGL::ClearValue{ 0.0f, 0.2f, 0.2f, 1.0f });

                // GUI Rendering with ImGui library
                llgl_cmdBuffer->PushDebugGroup("RenderGUI");
                {
                    // Start the Dear ImGui frame
                    NewFrameImGui();

                    // Show ImGui's demo window
                    ImGui::ShowDemoWindow();
                    
                    // GUI Rendering
                    RenderImGui();
                }
                llgl_cmdBuffer->PopDebugGroup();

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