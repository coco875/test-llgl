cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

# Set the project version and language
project(Test-LLGL VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")
set(CMAKE_C_STANDARD 11 CACHE STRING "The C standard to use")

include(FetchContent)

file(GLOB_RECURSE ALL_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "src/*.h"
    "src/*.c"
    "src/*.cpp"
)

# add_compile_options(-lglut -lGL -lGLEW -lGLU -lglx)
# add_link_options( -lGL -lGLEW -lGLU )

add_executable(${PROJECT_NAME} ${ALL_FILES})

find_package(SDL2 REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})

# Add the LLGL library
set(LLGL_BUILD_EXAMPLES OFF)
set(LLGL_BUILD_RENDERER_NULL OFF)
set(LLGL_BUILD_RENDERER_OPENGL ON)
set(LLGL_GL_ENABLE_DSA_EXT ON)
set(LLGL_GL_ENABLE_VENDOR_EXT ON)
set(LLGL_GL_INCLUDE_EXTERNAL ON)

set(LLGL_OUTPUT_DIR ${CMAKE_BINARY_DIR})

set(llgl_patch_file ${CMAKE_CURRENT_SOURCE_DIR}/cmake/llgl.patch)

# Applies the patch or checks if it has already been applied successfully previously. Will error otherwise.
set(llgl_apply_patch_if_needed git apply ${llgl_patch_file} ${git_hide_output} || git apply --reverse --check ${llgl_patch_file})

FetchContent_Declare(
    llgl
    GIT_REPOSITORY https://github.com/LukasBanana/LLGL.git
    GIT_TAG 250eb8336e347dda4410921d48d525dea395962d
    PATCH_COMMAND ${llgl_apply_patch_if_needed}
)
FetchContent_MakeAvailable(llgl)

target_link_libraries(${PROJECT_NAME} PRIVATE LLGL)

# #=================== spirv-headers ===================
# FetchContent_Declare(
#     spirv-headers
#     GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
#     GIT_TAG vulkan-sdk-1.4.304.1
# )
# FetchContent_MakeAvailable(spirv-headers)

# #=================== spirv-cross ===================
# FetchContent_Declare(
#     spirv-cross
#     GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
#     GIT_TAG vulkan-sdk-1.4.304.1
# )
# FetchContent_MakeAvailable(spirv-cross)

# #=================== spirv-tools ===================
# FetchContent_Declare(
#     spirv-tools
#     GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git
#     GIT_TAG vulkan-sdk-1.4.304.1
# )
# FetchContent_MakeAvailable(spirv-tools)

# #=================== glslang ===================
# FetchContent_Declare(
#     glslang
#     GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
#     GIT_TAG vulkan-sdk-1.4.304.1
# )
# FetchContent_MakeAvailable(glslang)

#=================== ImGui ===================
FetchContent_Declare(
    ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.6-docking
)
FetchContent_MakeAvailable(ImGui)
target_include_directories(${PROJECT_NAME} PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

add_library(ImGui STATIC)
set_property(TARGET ImGui PROPERTY CXX_STANDARD 20)

target_sources(ImGui
    PRIVATE
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
)

target_sources(ImGui
    PRIVATE
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
)

target_include_directories(ImGui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends PRIVATE ${SDL2_INCLUDE_DIRS})

target_link_libraries(ImGui PUBLIC SDL2::SDL2)

target_link_libraries(${PROJECT_NAME} PUBLIC ImGui)