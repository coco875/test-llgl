// dear imgui: Renderer Backend for LLGL (Low Level Graphics Library)
// This backend allows ImGui to render using LLGL, which abstracts OpenGL, Vulkan, D3D11, D3D12, Metal, etc.

#include "imgui_impl_llgl.h"
#include "imgui.h"
#include "imgui_shaders.h"

#ifndef IMGUI_DISABLE

#include <LLGL/LLGL.h>
#include <LLGL/Utils/VertexFormat.h>

#include <cstring>
#include <vector>
#include <variant>

// LLGL backend data
struct ImGui_ImplLLGL_Data {
    LLGL::RenderSystem* RenderSystem = nullptr;
    LLGL::SwapChain* SwapChain = nullptr;
    LLGL::CommandBuffer* CommandBuffer = nullptr;

    LLGL::PipelineState* Pipeline = nullptr;
    LLGL::PipelineLayout* PipelineLayout = nullptr;
    LLGL::Shader* VertexShader = nullptr;
    LLGL::Shader* FragmentShader = nullptr;

    LLGL::Buffer* VertexBuffer = nullptr;
    LLGL::Buffer* IndexBuffer = nullptr;
    LLGL::Buffer* ConstantBuffer = nullptr;
    int VertexBufferSize = 0;
    int IndexBufferSize = 0;

    LLGL::Texture* FontTexture = nullptr;
    LLGL::Sampler* FontSampler = nullptr;

    // Keep shader data alive (required for some backends)
    std::variant<std::string, std::vector<uint32_t>> VertexShaderData;
    std::variant<std::string, std::vector<uint32_t>> FragmentShaderData;
};

// Uniform buffer layout
struct ImGui_ImplLLGL_VertexConstantBuffer {
    float MVP[4][4];
};

// Buffer growth margins to avoid frequent reallocations
static constexpr int VERTEX_BUFFER_GROW_MARGIN = 5000;
static constexpr int INDEX_BUFFER_GROW_MARGIN = 10000;

// Backend data stored in io.BackendRendererUserData
static ImGui_ImplLLGL_Data* ImGui_ImplLLGL_GetBackendData() {
    return ImGui::GetCurrentContext() ? (ImGui_ImplLLGL_Data*) ImGui::GetIO().BackendRendererUserData : nullptr;
}

// Helper functions to detect shading language
// Note: LLGL uses bitmask encoding where high bits indicate language type and low bits indicate version
// We need to mask out the version bits before comparing the language type
static constexpr int LANG_TYPE_MASK = ~static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);

static bool is_spirv(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    version = 0;
    const int spirvBase = static_cast<int>(LLGL::ShadingLanguage::SPIRV);
    for (const auto& language : languages) {
        int langValue = static_cast<int>(language);
        if ((langValue & LANG_TYPE_MASK) == spirvBase) {
            version = langValue & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            return true;
        }
    }
    return false;
}

static bool is_metal(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    version = 0;
    const int metalBase = static_cast<int>(LLGL::ShadingLanguage::Metal);
    for (const auto& language : languages) {
        int langValue = static_cast<int>(language);
        if ((langValue & LANG_TYPE_MASK) == metalBase) {
            version = langValue & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            return true;
        }
    }
    return false;
}

static bool is_hlsl(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    version = 0;
    const int hlslBase = static_cast<int>(LLGL::ShadingLanguage::HLSL);
    for (const auto& language : languages) {
        int langValue = static_cast<int>(language);
        if ((langValue & LANG_TYPE_MASK) == hlslBase) {
            version = langValue & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            return true;
        }
    }
    return false;
}

static bool is_glsl(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    version = 0;
    const int glslBase = static_cast<int>(LLGL::ShadingLanguage::GLSL);
    for (const auto& language : languages) {
        int langValue = static_cast<int>(language);
        if ((langValue & LANG_TYPE_MASK) == glslBase) {
            version = langValue & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            return true;
        }
    }
    return false;
}

static bool is_glsles(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    version = 0;
    const int esslBase = static_cast<int>(LLGL::ShadingLanguage::ESSL);
    for (const auto& language : languages) {
        int langValue = static_cast<int>(language);
        if ((langValue & LANG_TYPE_MASK) == esslBase) {
            version = langValue & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            return true;
        }
    }
    return false;
}

static LLGL::VertexFormat ImGui_ImplLLGL_GetVertexFormat() {
    LLGL::VertexFormat vertexFormat;
    vertexFormat.AppendAttribute({ "aPos", LLGL::Format::RG32Float });
    vertexFormat.AppendAttribute({ "aUV", LLGL::Format::RG32Float });
    vertexFormat.AppendAttribute({ "aColor", LLGL::Format::RGBA8UNorm });
    vertexFormat.SetStride(sizeof(ImDrawVert));
    return vertexFormat;
}

static bool ImGui_ImplLLGL_CreateShaders() {
    ImGui_ImplLLGL_Data* bd = ImGui_ImplLLGL_GetBackendData();
    if (!bd || !bd->RenderSystem) {
        return false;
    }

    LLGL::RenderSystem* rs = bd->RenderSystem;
    auto caps = rs->GetRenderingCaps();
    const auto& languages = caps.shadingLanguages;

    // Setup vertex format
    LLGL::VertexFormat vertexFormat = ImGui_ImplLLGL_GetVertexFormat();

    // Create shader descriptors
    LLGL::ShaderDescriptor vertShaderDesc, fragShaderDesc;

    // Select appropriate shader based on supported shading language
    int version = 0;
    if (is_spirv(languages, version)) {
        // Vulkan/SPIRV - use pre-compiled SPIR-V binary
        std::vector<uint32_t> vertSpirv(
            reinterpret_cast<const uint32_t*>(g_ImGuiVertexShader_SPIRV),
            reinterpret_cast<const uint32_t*>(g_ImGuiVertexShader_SPIRV + g_ImGuiVertexShader_SPIRV_Size));
        std::vector<uint32_t> fragSpirv(
            reinterpret_cast<const uint32_t*>(g_ImGuiFragmentShader_SPIRV),
            reinterpret_cast<const uint32_t*>(g_ImGuiFragmentShader_SPIRV + g_ImGuiFragmentShader_SPIRV_Size));

        bd->VertexShaderData = vertSpirv;
        vertShaderDesc = { LLGL::ShaderType::Vertex, reinterpret_cast<const char*>(g_ImGuiVertexShader_SPIRV) };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::BinaryBuffer;
        vertShaderDesc.sourceSize = g_ImGuiVertexShader_SPIRV_Size;

        bd->FragmentShaderData = fragSpirv;
        fragShaderDesc = { LLGL::ShaderType::Fragment, reinterpret_cast<const char*>(g_ImGuiFragmentShader_SPIRV) };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::BinaryBuffer;
        fragShaderDesc.sourceSize = g_ImGuiFragmentShader_SPIRV_Size;
    } else if (is_metal(languages, version)) {
        // Metal Shading Language
        bd->VertexShaderData = std::string(g_ImGuiVertexShader_Metal);
        vertShaderDesc = { LLGL::ShaderType::Vertex, std::get<std::string>(bd->VertexShaderData).c_str() };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        vertShaderDesc.entryPoint = "main0";
        vertShaderDesc.profile = "2.1";

        bd->FragmentShaderData = std::string(g_ImGuiFragmentShader_Metal);
        fragShaderDesc = { LLGL::ShaderType::Fragment, std::get<std::string>(bd->FragmentShaderData).c_str() };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        fragShaderDesc.entryPoint = "main0";
        fragShaderDesc.profile = "2.1";
    } else if (is_hlsl(languages, version)) {
        // HLSL shaders
        bd->VertexShaderData = std::string(g_ImGuiVertexShader_HLSL);
        vertShaderDesc = { LLGL::ShaderType::Vertex, std::get<std::string>(bd->VertexShaderData).c_str() };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        vertShaderDesc.entryPoint = "main";
        vertShaderDesc.profile = "vs_5_0";

        bd->FragmentShaderData = std::string(g_ImGuiFragmentShader_HLSL);
        fragShaderDesc = { LLGL::ShaderType::Fragment, std::get<std::string>(bd->FragmentShaderData).c_str() };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        fragShaderDesc.entryPoint = "main";
        fragShaderDesc.profile = "ps_5_0";
    } else if (is_glsl(languages, version)) {
        // OpenGL GLSL shaders
        bd->VertexShaderData = std::string(g_ImGuiVertexShader_GLSL);
        vertShaderDesc = { LLGL::ShaderType::Vertex, std::get<std::string>(bd->VertexShaderData).c_str() };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;

        bd->FragmentShaderData = std::string(g_ImGuiFragmentShader_GLSL);
        fragShaderDesc = { LLGL::ShaderType::Fragment, std::get<std::string>(bd->FragmentShaderData).c_str() };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
    } else if (is_glsles(languages, version)) {
        // OpenGL ES GLSL shaders
        bd->VertexShaderData = std::string(g_ImGuiVertexShader_GLSL_ES);
        vertShaderDesc = { LLGL::ShaderType::Vertex, std::get<std::string>(bd->VertexShaderData).c_str() };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;

        bd->FragmentShaderData = std::string(g_ImGuiFragmentShader_GLSL_ES);
        fragShaderDesc = { LLGL::ShaderType::Fragment, std::get<std::string>(bd->FragmentShaderData).c_str() };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
    } else {
        LLGL::Log::Errorf("ImGui LLGL: Unsupported shading language\n");
        return false;
    }

    // Set vertex attributes
    vertShaderDesc.vertex.inputAttribs = vertexFormat.attributes;

    // Create shaders
    bd->VertexShader = rs->CreateShader(vertShaderDesc);
    if (bd->VertexShader == nullptr) {
        LLGL::Log::Errorf("ImGui LLGL: Failed to create vertex shader\n");
        return false;
    }
    if (const LLGL::Report* report = bd->VertexShader->GetReport()) {
        if (report->HasErrors()) {
            LLGL::Log::Errorf("ImGui LLGL: Vertex shader error: %s\n", report->GetText());
            return false;
        }
    }

    bd->FragmentShader = rs->CreateShader(fragShaderDesc);
    if (bd->FragmentShader == nullptr) {
        LLGL::Log::Errorf("ImGui LLGL: Failed to create fragment shader\n");
        return false;
    }
    if (const LLGL::Report* report = bd->FragmentShader->GetReport()) {
        if (report->HasErrors()) {
            LLGL::Log::Errorf("ImGui LLGL: Fragment shader error: %s\n", report->GetText());
            return false;
        }
    }

    return true;
}

bool ImGui_ImplLLGL_CreateDeviceObjects() {
    ImGui_ImplLLGL_Data* bd = ImGui_ImplLLGL_GetBackendData();
    if (!bd || !bd->RenderSystem) {
        return false;
    }

    LLGL::RenderSystem* rs = bd->RenderSystem;

    // Create shaders using shader_translation
    if (!ImGui_ImplLLGL_CreateShaders()) {
        return false;
    }

    // Create pipeline layout (same naming convention as main.cpp)
    LLGL::PipelineLayoutDescriptor layoutDesc;
    {
        layoutDesc.bindings = {
            LLGL::BindingDescriptor{ "Matrices", LLGL::ResourceType::Buffer, LLGL::BindFlags::ConstantBuffer,
                                     LLGL::StageFlags::VertexStage, 0 },
            LLGL::BindingDescriptor{ "colorMap", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled,
                                     LLGL::StageFlags::FragmentStage, 1 },
            LLGL::BindingDescriptor{ "samplerState", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage,
                                     2 },
        };
        layoutDesc.combinedTextureSamplers = { LLGL::CombinedTextureSamplerDescriptor{ "colorMap", "colorMap",
                                                                                       "samplerState", 3 } };
    }
    bd->PipelineLayout = rs->CreatePipelineLayout(layoutDesc);

    // Setup vertex format
    LLGL::VertexFormat vertexFormat = ImGui_ImplLLGL_GetVertexFormat();

    // Create graphics pipeline
    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.vertexShader = bd->VertexShader;
    pipelineDesc.fragmentShader = bd->FragmentShader;
    pipelineDesc.pipelineLayout = bd->PipelineLayout;
    pipelineDesc.renderPass = bd->SwapChain->GetRenderPass();
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleList;

    // Blend state for transparency
    pipelineDesc.blend.targets[0].blendEnabled = true;
    pipelineDesc.blend.targets[0].srcColor = LLGL::BlendOp::SrcAlpha;
    pipelineDesc.blend.targets[0].dstColor = LLGL::BlendOp::InvSrcAlpha;
    pipelineDesc.blend.targets[0].srcAlpha = LLGL::BlendOp::One;
    pipelineDesc.blend.targets[0].dstAlpha = LLGL::BlendOp::InvSrcAlpha;

    // Rasterizer state
    pipelineDesc.rasterizer.cullMode = LLGL::CullMode::Disabled;
    pipelineDesc.rasterizer.scissorTestEnabled = true;

    // Depth-stencil state
    pipelineDesc.depth.testEnabled = false;
    pipelineDesc.depth.writeEnabled = false;

    bd->Pipeline = rs->CreatePipelineState(pipelineDesc);
    if (bd->Pipeline == nullptr) {
        LLGL::Log::Errorf("ImGui LLGL: Failed to create pipeline\n");
        return false;
    }

    if (const LLGL::Report* report = bd->Pipeline->GetReport()) {
        if (report->HasErrors()) {
            LLGL::Log::Errorf("ImGui LLGL: Pipeline error: %s\n", report->GetText());
            return false;
        }
    }

    // Create constant buffer
    LLGL::BufferDescriptor cbDesc;
    {
        cbDesc.size = sizeof(ImGui_ImplLLGL_VertexConstantBuffer);
        cbDesc.bindFlags = LLGL::BindFlags::ConstantBuffer;
        cbDesc.cpuAccessFlags = LLGL::CPUAccessFlags::Write;
        cbDesc.miscFlags = LLGL::MiscFlags::DynamicUsage;
    }
    bd->ConstantBuffer = rs->CreateBuffer(cbDesc);

    // Create font sampler
    LLGL::SamplerDescriptor samplerDesc;
    samplerDesc.minFilter = LLGL::SamplerFilter::Linear;
    samplerDesc.magFilter = LLGL::SamplerFilter::Linear;
    samplerDesc.mipMapFilter = LLGL::SamplerFilter::Linear;
    samplerDesc.addressModeU = LLGL::SamplerAddressMode::Clamp;
    samplerDesc.addressModeV = LLGL::SamplerAddressMode::Clamp;
    bd->FontSampler = rs->CreateSampler(samplerDesc);

    ImGui_ImplLLGL_CreateFontsTexture();

    return true;
}

void ImGui_ImplLLGL_DestroyDeviceObjects() {
    ImGui_ImplLLGL_Data* bd = ImGui_ImplLLGL_GetBackendData();
    if (!bd || !bd->RenderSystem) {
        return;
    }

    LLGL::RenderSystem* rs = bd->RenderSystem;

    ImGui_ImplLLGL_DestroyFontsTexture();

    if (bd->Pipeline) {
        rs->Release(*bd->Pipeline);
    }
    if (bd->PipelineLayout) {
        rs->Release(*bd->PipelineLayout);
    }
    if (bd->VertexShader) {
        rs->Release(*bd->VertexShader);
    }
    if (bd->FragmentShader) {
        rs->Release(*bd->FragmentShader);
    }
    if (bd->VertexBuffer) {
        rs->Release(*bd->VertexBuffer);
    }
    if (bd->IndexBuffer) {
        rs->Release(*bd->IndexBuffer);
    }
    if (bd->ConstantBuffer) {
        rs->Release(*bd->ConstantBuffer);
    }
    if (bd->FontSampler) {
        rs->Release(*bd->FontSampler);
    }

    bd->Pipeline = nullptr;
    bd->PipelineLayout = nullptr;
    bd->VertexShader = nullptr;
    bd->FragmentShader = nullptr;
    bd->VertexBuffer = nullptr;
    bd->IndexBuffer = nullptr;
    bd->ConstantBuffer = nullptr;
    bd->FontSampler = nullptr;
    bd->VertexBufferSize = 0;
    bd->IndexBufferSize = 0;
}

bool ImGui_ImplLLGL_CreateFontsTexture() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplLLGL_Data* bd = ImGui_ImplLLGL_GetBackendData();
    if (!bd || !bd->RenderSystem) {
        return false;
    }

    LLGL::RenderSystem* rs = bd->RenderSystem;

    // Build texture atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Create texture
    LLGL::TextureDescriptor texDesc;
    texDesc.type = LLGL::TextureType::Texture2D;
    texDesc.format = LLGL::Format::RGBA8UNorm;
    texDesc.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    texDesc.bindFlags = LLGL::BindFlags::Sampled;

    LLGL::ImageView imageView(LLGL::ImageFormat::RGBA, LLGL::DataType::UInt8, pixels, width * height * 4);

    bd->FontTexture = rs->CreateTexture(texDesc, &imageView);

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID) bd->FontTexture);

    return true;
}

void ImGui_ImplLLGL_DestroyFontsTexture() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplLLGL_Data* bd = ImGui_ImplLLGL_GetBackendData();
    if (!bd || !bd->RenderSystem) {
        return;
    }

    if (bd->FontTexture) {
        bd->RenderSystem->Release(*bd->FontTexture);
        bd->FontTexture = nullptr;
        io.Fonts->SetTexID(0);
    }
}

bool ImGui_ImplLLGL_Init(ImGui_ImplLLGL_InitInfo* info) {
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplLLGL_Data* bd = IM_NEW(ImGui_ImplLLGL_Data)();
    io.BackendRendererUserData = (void*) bd;
    io.BackendRendererName = "imgui_impl_llgl";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We support ImDrawCallback_SetVtxOffset

    bd->RenderSystem = info->RenderSystem;
    bd->SwapChain = info->SwapChain;
    bd->CommandBuffer = info->CommandBuffer;

    return true;
}

void ImGui_ImplLLGL_Shutdown() {
    ImGui_ImplLLGL_Data* bd = ImGui_ImplLLGL_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplLLGL_DestroyDeviceObjects();

    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
    IM_DELETE(bd);
}

void ImGui_ImplLLGL_NewFrame() {
    ImGui_ImplLLGL_Data* bd = ImGui_ImplLLGL_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized!");

    if (!bd->Pipeline) {
        ImGui_ImplLLGL_CreateDeviceObjects();
    }
}

static void ImGui_ImplLLGL_CreateOrResizeBuffer(LLGL::Buffer*& buffer, int& currentSize, int requiredSize,
                                                size_t elementSize, long bindFlags,
                                                LLGL::Format format = LLGL::Format::Undefined,
                                                const LLGL::VertexFormat* vertexFormat = nullptr) {
    ImGui_ImplLLGL_Data* bd = ImGui_ImplLLGL_GetBackendData();
    LLGL::RenderSystem* rs = bd->RenderSystem;

    if (buffer) {
        rs->Release(*buffer);
    }

    currentSize = requiredSize;

    LLGL::BufferDescriptor bufDesc;
    bufDesc.size = currentSize * elementSize;
    bufDesc.bindFlags = bindFlags;
    bufDesc.cpuAccessFlags = LLGL::CPUAccessFlags::Write;
    bufDesc.miscFlags = LLGL::MiscFlags::DynamicUsage;

    if (format != LLGL::Format::Undefined) {
        bufDesc.format = format;
    }

    if (vertexFormat) {
        bufDesc.vertexAttribs = vertexFormat->attributes;
    }

    buffer = rs->CreateBuffer(bufDesc);
}

static void ImGui_ImplLLGL_SetupRenderState(ImDrawData* draw_data, LLGL::CommandBuffer* cmd, int fb_width,
                                            int fb_height) {
    ImGui_ImplLLGL_Data* bd = ImGui_ImplLLGL_GetBackendData();

    // Setup orthographic projection matrix
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

    ImGui_ImplLLGL_VertexConstantBuffer cb;
    float mvp[4][4] = {
        { 2.0f / (R - L), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (T - B), 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.5f, 0.0f },
        { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
    };
    memcpy(cb.MVP, mvp, sizeof(mvp));

    // Update constant buffer
    bd->RenderSystem->WriteBuffer(*bd->ConstantBuffer, 0, &cb, sizeof(cb));

    // Setup viewport
    LLGL::Viewport vp;
    vp.x = 0;
    vp.y = 0;
    vp.width = (float) fb_width;
    vp.height = (float) fb_height;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    cmd->SetViewport(vp);

    // Bind pipeline
    cmd->SetPipelineState(*bd->Pipeline);

    // Bind buffers
    cmd->SetVertexBuffer(*bd->VertexBuffer);
    cmd->SetIndexBuffer(*bd->IndexBuffer);

    // Bind constant buffer
    cmd->SetResource(0, *bd->ConstantBuffer);
}

void ImGui_ImplLLGL_RenderDrawData(ImDrawData* draw_data) {
    // Avoid rendering when minimized
    int fb_width = (int) (draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int) (draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) {
        return;
    }

    ImGui_ImplLLGL_Data* bd = ImGui_ImplLLGL_GetBackendData();
    LLGL::RenderSystem* rs = bd->RenderSystem;
    LLGL::CommandBuffer* cmd = bd->CommandBuffer;

    // Create or resize vertex/index buffers if needed
    if (bd->VertexBuffer == nullptr || bd->VertexBufferSize < draw_data->TotalVtxCount) {
        LLGL::VertexFormat vertexFormat = ImGui_ImplLLGL_GetVertexFormat();
        ImGui_ImplLLGL_CreateOrResizeBuffer(bd->VertexBuffer, bd->VertexBufferSize,
                                            draw_data->TotalVtxCount + VERTEX_BUFFER_GROW_MARGIN, sizeof(ImDrawVert),
                                            LLGL::BindFlags::VertexBuffer, LLGL::Format::Undefined, &vertexFormat);
    }

    if (bd->IndexBuffer == nullptr || bd->IndexBufferSize < draw_data->TotalIdxCount) {
        LLGL::Format idxFormat = sizeof(ImDrawIdx) == 2 ? LLGL::Format::R16UInt : LLGL::Format::R32UInt;
        ImGui_ImplLLGL_CreateOrResizeBuffer(bd->IndexBuffer, bd->IndexBufferSize,
                                            draw_data->TotalIdxCount + INDEX_BUFFER_GROW_MARGIN, sizeof(ImDrawIdx),
                                            LLGL::BindFlags::IndexBuffer, idxFormat);
    }

    // Upload vertex/index data
    {
        std::vector<ImDrawVert> vtxData;
        std::vector<ImDrawIdx> idxData;
        vtxData.reserve(draw_data->TotalVtxCount);
        idxData.reserve(draw_data->TotalIdxCount);

        for (int n = 0; n < draw_data->CmdListsCount; n++) {
            const ImDrawList* drawList = draw_data->CmdLists[n];
            vtxData.insert(vtxData.end(), drawList->VtxBuffer.Data,
                           drawList->VtxBuffer.Data + drawList->VtxBuffer.Size);
            idxData.insert(idxData.end(), drawList->IdxBuffer.Data,
                           drawList->IdxBuffer.Data + drawList->IdxBuffer.Size);
        }

        rs->WriteBuffer(*bd->VertexBuffer, 0, vtxData.data(), vtxData.size() * sizeof(ImDrawVert));
        rs->WriteBuffer(*bd->IndexBuffer, 0, idxData.data(), idxData.size() * sizeof(ImDrawIdx));
    }

    // Setup render state
    ImGui_ImplLLGL_SetupRenderState(draw_data, cmd, fb_width, fb_height);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;
    ImVec2 clip_scale = draw_data->FramebufferScale;

    // Render command lists
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* drawList = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < drawList->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &drawList->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr) {
                // User callback, registered via ImDrawList::AddCallback()
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState) {
                    ImGui_ImplLLGL_SetupRenderState(draw_data, cmd, fb_width, fb_height);
                } else {
                    pcmd->UserCallback(drawList, pcmd);
                }
            } else {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y) {
                    continue;
                }

                // Apply scissor
                LLGL::Scissor scissor;
                scissor.x = (int) clip_min.x;
                scissor.y = (int) clip_min.y;
                scissor.width = (int) (clip_max.x - clip_min.x);
                scissor.height = (int) (clip_max.y - clip_min.y);
                cmd->SetScissor(scissor);

                // Bind texture
                LLGL::Texture* texture = (LLGL::Texture*) pcmd->GetTexID();
                if (texture) {
                    cmd->SetResource(1, *texture);
                    cmd->SetResource(2, *bd->FontSampler);
                }

                // Draw
                cmd->DrawIndexed(pcmd->ElemCount, global_idx_offset + pcmd->IdxOffset,
                                 global_vtx_offset + pcmd->VtxOffset);
            }
        }
        global_idx_offset += drawList->IdxBuffer.Size;
        global_vtx_offset += drawList->VtxBuffer.Size;
    }
}

#endif // #ifndef IMGUI_DISABLE
