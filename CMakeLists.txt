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

if(APPLE)
    enable_language(OBJCXX)
    enable_language(OBJC)
    file(GLOB_RECURSE APPLE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        "src/*.mm"
    )
    list(APPEND ALL_FILES ${APPLE_FILES})
endif()

# add_compile_options(-lglut -lGL -lGLEW -lGLU -lglx)
# add_link_options( -lGL -lGLEW -lGLU )

add_executable(${PROJECT_NAME} ${ALL_FILES})

## VCPKG
if(WIN32)
include(cmake/automate-vcpkg.cmake)

vcpkg_bootstrap()
vcpkg_install_packages(glslang spirv-cross sdl2)
endif()

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

find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    include_directories(${Vulkan_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARY})
    target_compile_definitions(${PROJECT_NAME} PRIVATE LLGL_BUILD_RENDERER_VULKAN)
endif()

find_package(OpenGL REQUIRED)
if(OPENGL_FOUND)
    include_directories(${OPENGL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES})
else()
    message(FATAL_ERROR "LLGL_BUILD_RENDERER_OPENGL failed: missing OpenGL libraries")
endif()

# Add the LLGL library
set(LLGL_BUILD_EXAMPLES OFF)
set(LLGL_BUILD_RENDERER_NULL OFF)
set(LLGL_BUILD_RENDERER_OPENGL ON)
set(LLGL_GL_ENABLE_DSA_EXT ON)
set(LLGL_GL_ENABLE_VENDOR_EXT ON)
set(LLGL_GL_INCLUDE_EXTERNAL ON)
if (Vulkan_FOUND)
set(LLGL_BUILD_RENDERER_VULKAN ON)
else()
set(LLGL_BUILD_RENDERER_VULKAN OFF)
endif()
# make more problems
# set(LLGL_BUILD_STATIC_LIB ON)

set(LLGL_OUTPUT_DIR ${CMAKE_BINARY_DIR})

set(llgl_patch_file ${CMAKE_CURRENT_SOURCE_DIR}/cmake/llgl.patch)

# Applies the patch or checks if it has already been applied successfully previously. Will error otherwise.
set(llgl_apply_patch_if_needed git apply ${llgl_patch_file} ${git_hide_output} || git apply --reverse --check ${llgl_patch_file})

FetchContent_Declare(
    llgl
    GIT_REPOSITORY https://github.com/LukasBanana/LLGL.git
    GIT_TAG ab3bdfdf8fd54adb998be3f655b75b93ec153df6
    PATCH_COMMAND ${llgl_apply_patch_if_needed}
)
FetchContent_MakeAvailable(llgl)

find_package(glslang REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${glslang_SOURCE_DIR}/glslang/Public)
target_link_libraries(${PROJECT_NAME} PRIVATE glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper)

find_package(spirv_cross_core REQUIRED)
if(LLGL_BUILD_RENDERER_OPENGL)
    find_package(spirv_cross_glsl REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${spirv_cross_glsl_SOURCE_DIR}/include)
    target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-glsl)
endif()
if(LLGL_BUILD_RENDERER_VULKAN OR LLGL_BUILD_RENDERER_DIRECT3D11 OR LLGL_BUILD_RENDERER_DIRECT3D12)
    find_package(spirv_cross_hlsl REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${spirv_cross_hlsl_SOURCE_DIR}/include)
    target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-hlsl)
endif()
if(LLGL_BUILD_RENDERER_METAL)
    find_package(spirv_cross_msl REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${spirv_cross_msl_SOURCE_DIR}/include)
    target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-msl)
endif()
find_package(spirv_cross_cpp REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${spirv_cross_core_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${spirv_cross_cpp_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-core)
target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-cpp)

# if(LLGL_BUILD_RENDERER_VULKAN)
# target_link_libraries(LLGL LLGL_Vulkan)
# endif()

# if(LLGL_BUILD_RENDERER_OPENGL)
# target_link_libraries(LLGL LLGL_OpenGL)
# endif()

# if(LLGL_BUILD_RENDERER_DIRECT3D11)
# target_link_libraries(LLGL LLGL_Direct3D11)
# endif()

# if(LLGL_BUILD_RENDERER_DIRECT3D12)
# target_link_libraries(LLGL LLGL_Direct3D12)
# endif()

# if(LLGL_BUILD_RENDERER_METAL)
# target_link_libraries(LLGL LLGL_Metal)
# endif()

target_link_libraries(${PROJECT_NAME} PUBLIC LLGL)
target_include_directories(${PROJECT_NAME} PUBLIC ${llgl_SOURCE_DIR})

if(WIN32)
    set(EXTERNAL_INCLUDE_DIR "${llgl_SOURCE_DIR}/external")
    include_directories("${EXTERNAL_INCLUDE_DIR}/OpenGL/include")
endif()

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

if(Vulkan_FOUND)
    target_sources(ImGui
        PRIVATE
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    )
endif()

if(WIN32)
    target_sources(ImGui
        PRIVATE
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
    )
endif()

target_include_directories(ImGui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends PRIVATE ${SDL2_INCLUDE_DIRS})

target_link_libraries(ImGui PUBLIC SDL2::SDL2)
if(Vulkan_FOUND)
    target_link_libraries(ImGui PUBLIC ${Vulkan_LIBRARY})
endif()
if(WIN32)
    target_link_libraries(ImGui PUBLIC SDL2::SDL2main)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /Od;
            /Oi-
        >
        $<$<CONFIG:Release>:
            /Oi;
            /Gy
        >
        /permissive-;
        /MP;
        /sdl;
        /W3;
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        ${DEFAULT_CXX_EXCEPTION_HANDLING};
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF
        >
        /SUBSYSTEM:CONSOLE
    )
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ImGui)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    include(cmake/mac.cmake)
endif()
