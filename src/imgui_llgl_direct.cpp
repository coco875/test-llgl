#include "imgui_llgl_direct.h"
#include <LLGL/Utils/VertexFormat.h>
#include <cstring>
#include <algorithm>
#include <chrono>

namespace ImGuiLLGL {

// Vertex structure for ImGui
struct ImGuiVertex {
    float pos[2];
    float uv[2];
    uint32_t col;
};

// Global instance for convenience functions
static std::unique_ptr<DirectRenderer> g_renderer;

// Shader sources
static const char* vertexShaderSource = R"(
#version 450 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;

layout(binding = 0) uniform UniformBuffer {
    mat4 ProjectionMatrix;
} ubo;

void main() {
    fragUV = aUV;
    fragColor = aColor;
    gl_Position = ubo.ProjectionMatrix * vec4(aPos, 0.0, 1.0);
}
)";

static const char* fragmentShaderSource = R"(
#version 450 core

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D uTexture;

void main() {
    outColor = fragColor * texture(uTexture, fragUV);
}
)";

// Uniform buffer structure
struct UniformBuffer {
    float ProjectionMatrix[4][4];
};

DirectRenderer::DirectRenderer() 
    : renderSystem_(nullptr)
    , swapChain_(nullptr)
    , vertexShader_(nullptr)
    , fragmentShader_(nullptr)
    , pipelineState_(nullptr)
    , pipelineLayout_(nullptr)
    , vertexBuffer_(nullptr)
    , indexBuffer_(nullptr)
    , uniformBuffer_(nullptr)
    , sampler_(nullptr)
    , fontTexture_(nullptr)
    , vertexBufferSize_(0)
    , indexBufferSize_(0)
    , initialized_(false) {
}

DirectRenderer::~DirectRenderer() {
    Shutdown();
}

bool DirectRenderer::Init(LLGL::RenderSystemPtr& renderSystem, LLGL::SwapChain* swapChain) {
    if (initialized_) {
        return false;
    }
    
    renderSystem_ = renderSystem.get();
    swapChain_ = swapChain;
    
    // Initialize components
    if (!InitShaders()) {
        return false;
    }
    
    if (!InitBuffers()) {
        return false;
    }
    
    if (!InitRenderStates()) {
        return false;
    }
    
    if (!InitFontTexture()) {
        return false;
    }
    
    initialized_ = true;
    return true;
}

void DirectRenderer::Shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Release resources
    if (renderSystem_) {
        renderSystem_->Release(*fontTexture_);
        renderSystem_->Release(*sampler_);
        renderSystem_->Release(*uniformBuffer_);
        renderSystem_->Release(*indexBuffer_);
        renderSystem_->Release(*vertexBuffer_);
        renderSystem_->Release(*pipelineState_);
        renderSystem_->Release(*pipelineLayout_);
        renderSystem_->Release(*fragmentShader_);
        renderSystem_->Release(*vertexShader_);
    }
    
    fontTexture_ = nullptr;
    sampler_ = nullptr;
    uniformBuffer_ = nullptr;
    indexBuffer_ = nullptr;
    vertexBuffer_ = nullptr;
    pipelineState_ = nullptr;
    pipelineLayout_ = nullptr;
    fragmentShader_ = nullptr;
    vertexShader_ = nullptr;
    renderSystem_ = nullptr;
    swapChain_ = nullptr;
    
    initialized_ = false;
}

void DirectRenderer::NewFrame() {
    if (!initialized_) {
        return;
    }
    
    // Setup ImGui for new frame
    ImGuiIO& io = ImGui::GetIO();
    
    // Update display size
    const auto& resolution = swapChain_->GetResolution();
    io.DisplaySize = ImVec2(static_cast<float>(resolution.width), static_cast<float>(resolution.height));
    
    // Update time (basic implementation)
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    
    io.DeltaTime = deltaTime > 0.0f ? deltaTime : 1.0f / 60.0f;
}

void DirectRenderer::RenderDrawData(ImDrawData* drawData, LLGL::CommandBuffer* cmdBuffer) {
    if (!initialized_ || !drawData || !cmdBuffer) {
        return;
    }
    
    // Avoid rendering when display size is invalid
    if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f) {
        return;
    }
    
    // Update vertex/index buffers
    UpdateBuffers(drawData);
    
    // Setup render state
    SetupRenderState(drawData, cmdBuffer);
    
    // Render draw lists
    int globalIdxOffset = 0;
    int globalVtxOffset = 0;
    
    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        
        for (int cmdIndex = 0; cmdIndex < cmdList->CmdBuffer.Size; cmdIndex++) {
            const ImDrawCmd* cmd = &cmdList->CmdBuffer[cmdIndex];
            
            if (cmd->UserCallback) {
                // User callback
                cmd->UserCallback(cmdList, cmd);
            } else {
                // Set scissor rectangle
                if (config_.enableScissorTest) {
                    LLGL::Scissor scissor;
                    scissor.x = static_cast<int>(cmd->ClipRect.x);
                    scissor.y = static_cast<int>(cmd->ClipRect.y);
                    scissor.width = static_cast<int>(cmd->ClipRect.z - cmd->ClipRect.x);
                    scissor.height = static_cast<int>(cmd->ClipRect.w - cmd->ClipRect.y);
                    cmdBuffer->SetScissor(scissor);
                }
                
                // Bind texture
                LLGL::Texture* texture = reinterpret_cast<LLGL::Texture*>(cmd->TextureId);
                if (texture) {
                    cmdBuffer->SetResource(1, *texture);
                } else {
                    cmdBuffer->SetResource(1, *fontTexture_);
                }
                
                // Draw
                cmdBuffer->DrawIndexed(cmd->ElemCount, globalIdxOffset + cmd->IdxOffset, globalVtxOffset + cmd->VtxOffset);
            }
        }
        
        globalIdxOffset += cmdList->IdxBuffer.Size;
        globalVtxOffset += cmdList->VtxBuffer.Size;
    }
}

LLGL::Texture* DirectRenderer::CreateTexture(const void* data, int width, int height, int channels) {
    if (!initialized_ || !data || width <= 0 || height <= 0) {
        return nullptr;
    }
    
    LLGL::TextureDescriptor textureDesc;
    textureDesc.type = LLGL::TextureType::Texture2D;
    textureDesc.format = (channels == 4) ? LLGL::Format::RGBA8UNorm : LLGL::Format::RGB8UNorm;
    textureDesc.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    textureDesc.mipLevels = 1;
    
    LLGL::ImageView imageView;
    imageView.format = (channels == 4) ? LLGL::ImageFormat::RGBA : LLGL::ImageFormat::RGB;
    imageView.dataType = LLGL::DataType::UInt8;
    imageView.data = data;
    imageView.dataSize = width * height * channels;
    
    return renderSystem_->CreateTexture(textureDesc, &imageView);
}

void DirectRenderer::UpdateTexture(LLGL::Texture* texture, const void* data, int width, int height, int channels) {
    if (!initialized_ || !texture || !data || width <= 0 || height <= 0) {
        return;
    }
    
    LLGL::TextureRegion region;
    region.subresource.baseMipLevel = 0;
    region.subresource.numMipLevels = 1;
    region.subresource.baseArrayLayer = 0;
    region.subresource.numArrayLayers = 1;
    region.offset = { 0, 0, 0 };
    region.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    
    LLGL::ImageView imageView;
    imageView.format = (channels == 4) ? LLGL::ImageFormat::RGBA : LLGL::ImageFormat::RGB;
    imageView.dataType = LLGL::DataType::UInt8;
    imageView.data = data;
    imageView.dataSize = width * height * channels;
    
    renderSystem_->WriteTexture(*texture, region, imageView);
}

bool DirectRenderer::InitShaders() {
    // Get the supported shading languages
    const auto& languages = renderSystem_->GetRenderingCaps().shadingLanguages;
    
    // Determine the appropriate shader source type and version
    LLGL::ShaderSourceType sourceType = LLGL::ShaderSourceType::CodeString;
    std::string vertexShaderCode = vertexShaderSource;
    std::string fragmentShaderCode = fragmentShaderSource;
    
    // Adjust shader version based on the rendering backend
    if (renderSystem_->GetRendererID() == LLGL::RendererID::OpenGL) {
        // Use OpenGL-compatible shader version
        vertexShaderCode = R"(
#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

out vec2 fragUV;
out vec4 fragColor;

uniform mat4 ProjectionMatrix;

void main() {
    fragUV = aUV;
    fragColor = aColor;
    gl_Position = ProjectionMatrix * vec4(aPos, 0.0, 1.0);
}
)";
        
        fragmentShaderCode = R"(
#version 330 core

in vec2 fragUV;
in vec4 fragColor;

out vec4 outColor;

uniform sampler2D uTexture;

void main() {
    outColor = fragColor * texture(uTexture, fragUV);
}
)";
    }
    
    // Create vertex shader
    LLGL::ShaderDescriptor vertexShaderDesc;
    vertexShaderDesc.type = LLGL::ShaderType::Vertex;
    vertexShaderDesc.source = vertexShaderCode.c_str();
    vertexShaderDesc.sourceType = sourceType;
    vertexShaderDesc.entryPoint = "main";
    
    // Setup vertex attributes
    LLGL::VertexFormat vertexFormat;
    vertexFormat.AppendAttribute({ "aPos", LLGL::Format::RG32Float });
    vertexFormat.AppendAttribute({ "aUV", LLGL::Format::RG32Float });
    vertexFormat.AppendAttribute({ "aColor", LLGL::Format::RGBA8UNorm });
    vertexFormat.SetStride(sizeof(ImGuiVertex));
    
    vertexShaderDesc.vertex.inputAttribs = vertexFormat.attributes;
    
    vertexShader_ = renderSystem_->CreateShader(vertexShaderDesc);
    if (!vertexShader_) {
        return false;
    }
    
    // Create fragment shader
    LLGL::ShaderDescriptor fragmentShaderDesc;
    fragmentShaderDesc.type = LLGL::ShaderType::Fragment;
    fragmentShaderDesc.source = fragmentShaderCode.c_str();
    fragmentShaderDesc.sourceType = sourceType;
    fragmentShaderDesc.entryPoint = "main";
    
    fragmentShader_ = renderSystem_->CreateShader(fragmentShaderDesc);
    if (!fragmentShader_) {
        return false;
    }
    
    // Create pipeline layout based on shader type
    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    
    if (renderSystem_->GetRendererID() == LLGL::RendererID::OpenGL) {
        // OpenGL uses different binding layout
        pipelineLayoutDesc.uniforms = {
            LLGL::UniformDescriptor{ "ProjectionMatrix", LLGL::UniformType::Float4x4 },
            LLGL::UniformDescriptor{ "uTexture", LLGL::UniformType::Sampler }
        };
    } else {
        // Vulkan-style binding
        pipelineLayoutDesc.bindings = {
            LLGL::BindingDescriptor{ "UniformBuffer", LLGL::ResourceType::Buffer, LLGL::BindFlags::ConstantBuffer, LLGL::StageFlags::VertexStage, 0 },
            LLGL::BindingDescriptor{ "uTexture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, 1 }
        };
    }
    
    pipelineLayout_ = renderSystem_->CreatePipelineLayout(pipelineLayoutDesc);
    if (!pipelineLayout_) {
        return false;
    }
    
    // Create graphics pipeline
    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.vertexShader = vertexShader_;
    pipelineDesc.fragmentShader = fragmentShader_;
    pipelineDesc.renderPass = swapChain_->GetRenderPass();
    pipelineDesc.pipelineLayout = pipelineLayout_;
    
    // Setup blend state for alpha blending
    if (config_.enableAlphaBlending) {
        pipelineDesc.blend.targets[0].colorMask = LLGL::ColorMaskFlags::All;
        pipelineDesc.blend.targets[0].blendEnabled = true;
        pipelineDesc.blend.targets[0].srcColor = LLGL::BlendOp::SrcAlpha;
        pipelineDesc.blend.targets[0].dstColor = LLGL::BlendOp::InvSrcAlpha;
        pipelineDesc.blend.targets[0].colorArithmetic = LLGL::BlendArithmetic::Add;
        pipelineDesc.blend.targets[0].srcAlpha = LLGL::BlendOp::InvSrcAlpha;
        pipelineDesc.blend.targets[0].dstAlpha = LLGL::BlendOp::Zero;
        pipelineDesc.blend.targets[0].alphaArithmetic = LLGL::BlendArithmetic::Add;
    }
    
    // Setup rasterizer
    pipelineDesc.rasterizer.cullMode = LLGL::CullMode::Disabled;
    pipelineDesc.rasterizer.scissorTestEnabled = config_.enableScissorTest;
    
    // Setup depth state
    pipelineDesc.depth.testEnabled = false;
    pipelineDesc.depth.writeEnabled = false;
    
    pipelineState_ = renderSystem_->CreatePipelineState(pipelineDesc);
    if (!pipelineState_) {
        return false;
    }
    
    return true;
}

bool DirectRenderer::InitBuffers() {
    // Create vertex buffer
    LLGL::BufferDescriptor vertexBufferDesc;
    vertexBufferDesc.size = config_.maxVertices * sizeof(ImGuiVertex);
    vertexBufferDesc.bindFlags = LLGL::BindFlags::VertexBuffer;
    vertexBufferDesc.cpuAccessFlags = LLGL::CPUAccessFlags::Write;
    vertexBufferDesc.miscFlags = LLGL::MiscFlags::DynamicUsage;
    
    vertexBuffer_ = renderSystem_->CreateBuffer(vertexBufferDesc);
    if (!vertexBuffer_) {
        return false;
    }
    
    vertexBufferSize_ = config_.maxVertices;
    
    // Create index buffer
    LLGL::BufferDescriptor indexBufferDesc;
    indexBufferDesc.size = config_.maxIndices * sizeof(ImDrawIdx);
    indexBufferDesc.bindFlags = LLGL::BindFlags::IndexBuffer;
    indexBufferDesc.cpuAccessFlags = LLGL::CPUAccessFlags::Write;
    indexBufferDesc.miscFlags = LLGL::MiscFlags::DynamicUsage;
    indexBufferDesc.format = (sizeof(ImDrawIdx) == 2) ? LLGL::Format::R16UInt : LLGL::Format::R32UInt;
    
    indexBuffer_ = renderSystem_->CreateBuffer(indexBufferDesc);
    if (!indexBuffer_) {
        return false;
    }
    
    indexBufferSize_ = config_.maxIndices;
    
    // Create uniform buffer
    LLGL::BufferDescriptor uniformBufferDesc;
    uniformBufferDesc.size = sizeof(UniformBuffer);
    uniformBufferDesc.bindFlags = LLGL::BindFlags::ConstantBuffer;
    uniformBufferDesc.cpuAccessFlags = LLGL::CPUAccessFlags::Write;
    uniformBufferDesc.miscFlags = LLGL::MiscFlags::DynamicUsage;
    
    uniformBuffer_ = renderSystem_->CreateBuffer(uniformBufferDesc);
    if (!uniformBuffer_) {
        return false;
    }
    
    return true;
}

bool DirectRenderer::InitRenderStates() {
    // Create sampler
    LLGL::SamplerDescriptor samplerDesc;
    samplerDesc.minFilter = LLGL::SamplerFilter::Linear;
    samplerDesc.magFilter = LLGL::SamplerFilter::Linear;
    samplerDesc.addressModeU = LLGL::SamplerAddressMode::Repeat;
    samplerDesc.addressModeV = LLGL::SamplerAddressMode::Repeat;
    samplerDesc.addressModeW = LLGL::SamplerAddressMode::Repeat;
    
    sampler_ = renderSystem_->CreateSampler(samplerDesc);
    if (!sampler_) {
        return false;
    }
    
    return true;
}

bool DirectRenderer::InitFontTexture() {
    // Get font texture data from ImGui
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    
    // Create font texture
    fontTexture_ = CreateTexture(pixels, width, height, 4);
    if (!fontTexture_) {
        return false;
    }
    
    // Set texture ID in ImGui
    io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(fontTexture_));
    
    return true;
}

void DirectRenderer::UpdateBuffers(ImDrawData* drawData) {
    // Calculate total vertices and indices
    size_t totalVertices = 0;
    size_t totalIndices = 0;
    
    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        totalVertices += cmdList->VtxBuffer.Size;
        totalIndices += cmdList->IdxBuffer.Size;
    }
    
    // Resize buffers if needed
    if (totalVertices > vertexBufferSize_) {
        size_t newSize = static_cast<size_t>(totalVertices * config_.growthFactor);
        
        LLGL::BufferDescriptor vertexBufferDesc;
        vertexBufferDesc.size = newSize * sizeof(ImGuiVertex);
        vertexBufferDesc.bindFlags = LLGL::BindFlags::VertexBuffer;
        vertexBufferDesc.cpuAccessFlags = LLGL::CPUAccessFlags::Write;
        vertexBufferDesc.miscFlags = LLGL::MiscFlags::DynamicUsage;
        
        renderSystem_->Release(*vertexBuffer_);
        vertexBuffer_ = renderSystem_->CreateBuffer(vertexBufferDesc);
        vertexBufferSize_ = newSize;
    }
    
    if (totalIndices > indexBufferSize_) {
        size_t newSize = static_cast<size_t>(totalIndices * config_.growthFactor);
        
        LLGL::BufferDescriptor indexBufferDesc;
        indexBufferDesc.size = newSize * sizeof(ImDrawIdx);
        indexBufferDesc.bindFlags = LLGL::BindFlags::IndexBuffer;
        indexBufferDesc.cpuAccessFlags = LLGL::CPUAccessFlags::Write;
        indexBufferDesc.miscFlags = LLGL::MiscFlags::DynamicUsage;
        indexBufferDesc.format = (sizeof(ImDrawIdx) == 2) ? LLGL::Format::R16UInt : LLGL::Format::R32UInt;
        
        renderSystem_->Release(*indexBuffer_);
        indexBuffer_ = renderSystem_->CreateBuffer(indexBufferDesc);
        indexBufferSize_ = newSize;
    }
    
    // Update vertex buffer
    if (totalVertices > 0) {
        ImGuiVertex* vertices = static_cast<ImGuiVertex*>(renderSystem_->MapBuffer(*vertexBuffer_, LLGL::CPUAccess::WriteOnly));
        if (vertices) {
            for (int n = 0; n < drawData->CmdListsCount; n++) {
                const ImDrawList* cmdList = drawData->CmdLists[n];
                const ImDrawVert* srcVertices = cmdList->VtxBuffer.Data;
                
                for (int i = 0; i < cmdList->VtxBuffer.Size; i++) {
                    vertices->pos[0] = srcVertices[i].pos.x;
                    vertices->pos[1] = srcVertices[i].pos.y;
                    vertices->uv[0] = srcVertices[i].uv.x;
                    vertices->uv[1] = srcVertices[i].uv.y;
                    vertices->col = srcVertices[i].col;
                    vertices++;
                }
            }
            renderSystem_->UnmapBuffer(*vertexBuffer_);
        }
    }
    
    // Update index buffer
    if (totalIndices > 0) {
        ImDrawIdx* indices = static_cast<ImDrawIdx*>(renderSystem_->MapBuffer(*indexBuffer_, LLGL::CPUAccess::WriteOnly));
        if (indices) {
            for (int n = 0; n < drawData->CmdListsCount; n++) {
                const ImDrawList* cmdList = drawData->CmdLists[n];
                std::memcpy(indices, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
                indices += cmdList->IdxBuffer.Size;
            }
            renderSystem_->UnmapBuffer(*indexBuffer_);
        }
    }
}

void DirectRenderer::SetupRenderState(ImDrawData* drawData, LLGL::CommandBuffer* cmdBuffer) {
    // Set viewport
    LLGL::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = drawData->DisplaySize.x;
    viewport.height = drawData->DisplaySize.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    cmdBuffer->SetViewport(viewport);
    
    // Set vertex and index buffers
    cmdBuffer->SetVertexBuffer(*vertexBuffer_);
    cmdBuffer->SetIndexBuffer(*indexBuffer_);
    
    // Set pipeline state
    cmdBuffer->SetPipelineState(*pipelineState_);
    
    // Update and bind uniform buffer
    UniformBuffer* uniformData = static_cast<UniformBuffer*>(renderSystem_->MapBuffer(*uniformBuffer_, LLGL::CPUAccess::WriteOnly));
    if (uniformData) {
        float left = drawData->DisplayPos.x;
        float right = drawData->DisplayPos.x + drawData->DisplaySize.x;
        float top = drawData->DisplayPos.y;
        float bottom = drawData->DisplayPos.y + drawData->DisplaySize.y;
        
        // Create orthographic projection matrix
        uniformData->ProjectionMatrix[0][0] = 2.0f / (right - left);
        uniformData->ProjectionMatrix[0][1] = 0.0f;
        uniformData->ProjectionMatrix[0][2] = 0.0f;
        uniformData->ProjectionMatrix[0][3] = 0.0f;
        
        uniformData->ProjectionMatrix[1][0] = 0.0f;
        uniformData->ProjectionMatrix[1][1] = 2.0f / (top - bottom);
        uniformData->ProjectionMatrix[1][2] = 0.0f;
        uniformData->ProjectionMatrix[1][3] = 0.0f;
        
        uniformData->ProjectionMatrix[2][0] = 0.0f;
        uniformData->ProjectionMatrix[2][1] = 0.0f;
        uniformData->ProjectionMatrix[2][2] = -1.0f;
        uniformData->ProjectionMatrix[2][3] = 0.0f;
        
        uniformData->ProjectionMatrix[3][0] = (right + left) / (left - right);
        uniformData->ProjectionMatrix[3][1] = (top + bottom) / (bottom - top);
        uniformData->ProjectionMatrix[3][2] = 0.0f;
        uniformData->ProjectionMatrix[3][3] = 1.0f;
        
        renderSystem_->UnmapBuffer(*uniformBuffer_);
    }
    
    // Bind uniform buffer
    cmdBuffer->SetResource(0, *uniformBuffer_);
    
    // Bind sampler
    cmdBuffer->SetResource(2, *sampler_);
}

// Global function implementations
bool Init(LLGL::RenderSystemPtr& renderSystem, LLGL::SwapChain* swapChain, const Config& config) {
    if (g_renderer) {
        return false;
    }
    
    g_renderer = std::make_unique<DirectRenderer>();
    g_renderer->SetConfig(config);
    return g_renderer->Init(renderSystem, swapChain);
}

void Shutdown() {
    if (g_renderer) {
        g_renderer->Shutdown();
        g_renderer.reset();
    }
}

void NewFrame() {
    if (g_renderer) {
        g_renderer->NewFrame();
    }
}

void RenderDrawData(ImDrawData* drawData, LLGL::CommandBuffer* cmdBuffer) {
    if (g_renderer) {
        g_renderer->RenderDrawData(drawData, cmdBuffer);
    }
}

LLGL::Texture* CreateTexture(const void* data, int width, int height, int channels) {
    if (g_renderer) {
        return g_renderer->CreateTexture(data, width, height, channels);
    }
    return nullptr;
}

void UpdateTexture(LLGL::Texture* texture, const void* data, int width, int height, int channels) {
    if (g_renderer) {
        g_renderer->UpdateTexture(texture, data, width, height, channels);
    }
}

} // namespace ImGuiLLGL