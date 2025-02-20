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

#=================== SDL2 ===================
find_package(SDL2 QUIET)
if (NOT ${SDL2_FOUND})
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.28.1
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(SDL2)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})

# Add the LLGL library
set(LLGL_BUILD_EXAMPLES OFF)
set(LLGL_BUILD_RENDERER_NULL OFF)
set(LLGL_BUILD_RENDERER_OPENGL ON)
set(LLGL_GL_ENABLE_DSA_EXT ON)
set(LLGL_GL_ENABLE_VENDOR_EXT ON)
set(LLGL_GL_INCLUDE_EXTERNAL ON)
set(LLGL_BUILD_RENDERER_VULKAN ON)

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
target_include_directories(${PROJECT_NAME} PRIVATE ${llgl_SOURCE_DIR})

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
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(ImGui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends PRIVATE ${SDL2_INCLUDE_DIRS})

target_link_libraries(ImGui PUBLIC SDL2::SDL2)

target_link_libraries(${PROJECT_NAME} PUBLIC ImGui)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    include(cmake/mac.cmake)
endif()

find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    include_directories(${Vulkan_INCLUDE_DIR})
else()
    message(FATAL_ERROR "LLGL_BUILD_RENDERER_VULKAN failed: missing Vulkan libraries")
endif()