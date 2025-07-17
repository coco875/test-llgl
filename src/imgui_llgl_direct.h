#ifndef IMGUI_LLGL_DIRECT_H
#define IMGUI_LLGL_DIRECT_H

#include <LLGL/LLGL.h>
#include <imgui.h>
#include <memory>

namespace ImGuiLLGL {

// Forward declarations
class DirectRenderer;

// Configuration structure for ImGui LLGL direct implementation
struct Config {
    // Maximum number of vertices and indices for dynamic buffers
    size_t maxVertices = 65536;
    size_t maxIndices = 65536;
    
    // Buffer growth factor when resizing
    float growthFactor = 1.5f;
    
    // Enable/disable alpha blending
    bool enableAlphaBlending = true;
    
    // Enable/disable scissor testing
    bool enableScissorTest = true;
};

// Direct ImGui renderer class that uses LLGL directly
class DirectRenderer {
public:
    DirectRenderer();
    ~DirectRenderer();

    // Initialize the renderer with LLGL render system
    bool Init(LLGL::RenderSystemPtr& renderSystem, LLGL::SwapChain* swapChain);
    
    // Shutdown the renderer and release resources
    void Shutdown();
    
    // Create a new frame
    void NewFrame();
    
    // Render ImGui draw data
    void RenderDrawData(ImDrawData* drawData, LLGL::CommandBuffer* cmdBuffer);
    
    // Create or update texture from image data
    LLGL::Texture* CreateTexture(const void* data, int width, int height, int channels = 4);
    
    // Update texture data
    void UpdateTexture(LLGL::Texture* texture, const void* data, int width, int height, int channels = 4);
    
    // Get the configuration
    const Config& GetConfig() const { return config_; }
    
    // Update configuration (call before Init)
    void SetConfig(const Config& config) { config_ = config; }

private:
    // Initialize shaders
    bool InitShaders();
    
    // Initialize buffers
    bool InitBuffers();
    
    // Initialize render states
    bool InitRenderStates();
    
    // Initialize font texture
    bool InitFontTexture();
    
    // Update vertex/index buffers with ImGui data
    void UpdateBuffers(ImDrawData* drawData);
    
    // Setup render state for ImGui rendering
    void SetupRenderState(ImDrawData* drawData, LLGL::CommandBuffer* cmdBuffer);

private:
    Config config_;
    
    // LLGL resources
    LLGL::RenderSystem* renderSystem_;
    LLGL::SwapChain* swapChain_;
    
    // Shader resources
    LLGL::Shader* vertexShader_;
    LLGL::Shader* fragmentShader_;
    LLGL::PipelineState* pipelineState_;
    LLGL::PipelineLayout* pipelineLayout_;
    
    // Buffer resources
    LLGL::Buffer* vertexBuffer_;
    LLGL::Buffer* indexBuffer_;
    LLGL::Buffer* uniformBuffer_;
    LLGL::Sampler* sampler_;
    
    // Font texture
    LLGL::Texture* fontTexture_;
    
    // Current buffer sizes
    size_t vertexBufferSize_;
    size_t indexBufferSize_;
    
    // Initialization state
    bool initialized_;
};

// Global functions for easy integration
bool Init(LLGL::RenderSystemPtr& renderSystem, LLGL::SwapChain* swapChain, const Config& config = Config{});
void Shutdown();
void NewFrame();
void RenderDrawData(ImDrawData* drawData, LLGL::CommandBuffer* cmdBuffer);
LLGL::Texture* CreateTexture(const void* data, int width, int height, int channels = 4);
void UpdateTexture(LLGL::Texture* texture, const void* data, int width, int height, int channels = 4);

} // namespace ImGuiLLGL

#endif // IMGUI_LLGL_DIRECT_H