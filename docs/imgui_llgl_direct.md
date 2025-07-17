# ImGui Direct LLGL Implementation

This document describes the new direct ImGui implementation for LLGL (Lightweight Graphics Library) that bypasses traditional ImGui backends and uses LLGL directly for improved performance and integration.

## Overview

The direct implementation provides:

1. **Direct LLGL Integration**: No dependency on traditional ImGui backends (OpenGL, Vulkan, etc.)
2. **Performance Optimized**: Uses LLGL's native buffer and texture management
3. **Modular Design**: Clean separation between rendering logic and application code
4. **Configurable**: Customizable buffer sizes and rendering settings
5. **Cross-Platform**: Works with all LLGL-supported backends

## Architecture

### Core Components

- **`ImGuiLLGL::DirectRenderer`**: Main renderer class that manages all LLGL resources
- **`ImGuiLLGL::Config`**: Configuration structure for customizing renderer behavior
- **Vertex/Index Buffers**: Dynamic buffers that grow as needed
- **Uniform Buffer**: Stores projection matrix and other shader uniforms
- **Font Texture**: Manages ImGui's font atlas texture
- **Pipeline State**: Configured for alpha blending and scissor testing

### File Structure

- `imgui_llgl_direct.h` - Header with class definitions and API
- `imgui_llgl_direct.cpp` - Implementation of the direct renderer
- `main_direct_example.cpp` - Example usage demonstration

## Usage

### Basic Setup

```cpp
#include "imgui_llgl_direct.h"

// Initialize ImGui context
IMGUI_CHECKVERSION();
ImGui::CreateContext();
ImGuiIO& io = ImGui::GetIO();
io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

// Setup ImGui style
ImGui::StyleColorsDark();

// Configure the renderer
ImGuiLLGL::Config config;
config.maxVertices = 65536;
config.maxIndices = 65536;
config.enableAlphaBlending = true;
config.enableScissorTest = true;

// Initialize the direct renderer
if (!ImGuiLLGL::Init(renderSystem, swapChain, config)) {
    // Handle error
}
```

### Rendering Loop

```cpp
// Start ImGui frame
ImGuiLLGL::NewFrame();
ImGui_ImplSDL2_NewFrame(); // For input handling
ImGui::NewFrame();

// Create your ImGui UI
ImGui::Begin("My Window");
ImGui::Text("Hello, World!");
ImGui::End();

// Render
ImGui::Render();
ImGuiLLGL::RenderDrawData(ImGui::GetDrawData(), commandBuffer);
```

### Cleanup

```cpp
ImGuiLLGL::Shutdown();
ImGui::DestroyContext();
```

## Configuration Options

### `ImGuiLLGL::Config`

- **`maxVertices`**: Maximum number of vertices in dynamic buffer (default: 65536)
- **`maxIndices`**: Maximum number of indices in dynamic buffer (default: 65536)
- **`growthFactor`**: Buffer growth multiplier when resizing (default: 1.5)
- **`enableAlphaBlending`**: Enable alpha blending for transparency (default: true)
- **`enableScissorTest`**: Enable scissor testing for clipping (default: true)

## API Reference

### Global Functions

#### `bool Init(LLGL::RenderSystemPtr& renderSystem, LLGL::SwapChain* swapChain, const Config& config = Config{})`
Initializes the direct renderer with the given LLGL render system and swap chain.

#### `void Shutdown()`
Shuts down the renderer and releases all resources.

#### `void NewFrame()`
Prepares for a new ImGui frame, updating display size and timing.

#### `void RenderDrawData(ImDrawData* drawData, LLGL::CommandBuffer* cmdBuffer)`
Renders ImGui draw data using the specified command buffer.

#### `LLGL::Texture* CreateTexture(const void* data, int width, int height, int channels = 4)`
Creates a new texture from image data.

#### `void UpdateTexture(LLGL::Texture* texture, const void* data, int width, int height, int channels = 4)`
Updates an existing texture with new image data.

### DirectRenderer Class

The `DirectRenderer` class provides the same API as the global functions but allows for multiple renderer instances.

## Technical Details

### Shader Implementation

The implementation uses custom GLSL shaders optimized for ImGui rendering:

- **Vertex Shader**: Transforms vertices using orthographic projection
- **Fragment Shader**: Applies texture and color blending

### Buffer Management

- **Dynamic Buffers**: Vertex and index buffers grow automatically as needed
- **Memory Mapping**: Efficient buffer updates using LLGL's mapping API
- **Growth Strategy**: Configurable growth factor to balance memory usage and performance

### Render State Management

- **Alpha Blending**: Proper alpha blending setup for transparency
- **Scissor Testing**: Accurate clipping for UI elements
- **Depth Testing**: Disabled for UI rendering
- **Culling**: Disabled to support all vertex winding orders

### Texture Management

- **Font Atlas**: Automatic management of ImGui's font texture
- **Custom Textures**: Support for user-provided textures
- **Format Support**: RGBA and RGB texture formats

## Performance Considerations

### Advantages

1. **Direct API Usage**: No overhead from traditional backends
2. **Optimized Buffer Management**: Efficient memory usage and updates
3. **Reduced State Changes**: Minimized render state transitions
4. **Custom Shaders**: Optimized specifically for ImGui rendering

### Best Practices

1. **Buffer Sizing**: Set appropriate initial buffer sizes to avoid frequent resizing
2. **Texture Reuse**: Reuse textures when possible to reduce GPU memory usage
3. **Batch Rendering**: ImGui naturally batches draw calls for optimal performance
4. **Scissor Testing**: Keep scissor testing enabled for proper clipping

## Limitations and Considerations

1. **LLGL Dependency**: Requires LLGL render system to be properly initialized
2. **Shader Compatibility**: Shaders must be compatible with the target graphics API
3. **Input Handling**: Still requires a separate input backend (e.g., SDL2)
4. **Platform Shaders**: May need different shader versions for different platforms

## Integration with Existing Code

The direct implementation can be used alongside or as a replacement for existing ImGui backends. To migrate:

1. Replace backend initialization calls with `ImGuiLLGL::Init()`
2. Replace rendering calls with `ImGuiLLGL::RenderDrawData()`
3. Update cleanup code to use `ImGuiLLGL::Shutdown()`
4. Keep input handling backends (SDL2, Win32, etc.) unchanged

## Example Application

See `main_direct_example.cpp` for a complete example demonstrating:

- Basic setup and initialization
- Rendering loop with ImGui demo window
- Custom UI elements
- Proper cleanup and shutdown

This example can be compiled and run to verify the implementation works correctly with your LLGL setup.

## Troubleshooting

### Common Issues

1. **Blank UI**: Check that shaders compiled successfully and buffers are properly initialized
2. **Input Not Working**: Ensure input backend (SDL2, etc.) is properly initialized
3. **Performance Issues**: Verify buffer sizes are appropriate for your use case
4. **Rendering Artifacts**: Check that alpha blending and scissor testing are properly configured

### Debug Features

The implementation includes error checking and logging to help identify issues during development.