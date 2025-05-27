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

# if ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
#     set(VCPKG_TRIPLET x86-windows-static)
#     set(VCPKG_TARGET_TRIPLET x86-windows-static)
# elseif ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
#     set(VCPKG_TRIPLET x64-windows-static)
#     set(VCPKG_TARGET_TRIPLET x64-windows-static)
# endif()

vcpkg_bootstrap()
vcpkg_install_packages(glslang spirv-cross sdl2 vulkan)
endif()

find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    include_directories(${Vulkan_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARY})
    target_compile_definitions(${PROJECT_NAME} PRIVATE LLGL_BUILD_RENDERER_VULKAN)
endif()

#=================== STB ===================
set(STB_DIR ${CMAKE_BINARY_DIR}/_deps/stb)
file(DOWNLOAD "https://github.com/nothings/stb/raw/0bc88af4de5fb022db643c2d8e549a0927749354/stb_image.h" "${STB_DIR}/stb_image.h")
file(WRITE "${STB_DIR}/stb_impl.c" "#define STB_IMAGE_IMPLEMENTATION\n#include \"stb_image.h\"")

add_library(stb STATIC)

target_sources(stb PRIVATE
    ${STB_DIR}/stb_image.h
    ${STB_DIR}/stb_impl.c
)

target_include_directories(stb PUBLIC ${STB_DIR})

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

find_package(OpenGL REQUIRED)
if(OPENGL_FOUND)
    include_directories(${OPENGL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES})
else()
    message(FATAL_ERROR "LLGL_BUILD_RENDERER_OPENGL failed: missing OpenGL libraries")
endif()

# Add the LLGL library
set(LLGL_BUILD_EXAMPLES OFF CACHE BOOL "Disable LLGL examples")
set(LLGL_BUILD_RENDERER_NULL OFF CACHE BOOL "Disable LLGL Null renderer")
set(LLGL_BUILD_RENDERER_OPENGL ON CACHE BOOL "Enable LLGL OpenGL renderer")
set(LLGL_GL_ENABLE_DSA_EXT ON CACHE BOOL "Enable OpenGL DSA extension")
set(LLGL_GL_ENABLE_VENDOR_EXT ON CACHE BOOL "Enable OpenGL vendor extensions")
set(LLGL_GL_INCLUDE_EXTERNAL ON CACHE BOOL "Include external OpenGL headers")
if (Vulkan_FOUND)
set(LLGL_BUILD_RENDERER_VULKAN ON CACHE BOOL "Enable LLGL Vulkan renderer")
else()
set(LLGL_BUILD_RENDERER_VULKAN OFF CACHE BOOL "Disable LLGL Vulkan renderer")
endif()
set(LLGL_BUILD_STATIC_LIB ON CACHE BOOL "Build LLGL as a static library")

set(LLGL_OUTPUT_DIR ${CMAKE_BINARY_DIR} CACHE INTERNAL "LLGL output directory")

set(llgl_patch_file ${CMAKE_CURRENT_SOURCE_DIR}/cmake/llgl.patch)

set(llgl_apply_patch_command ${CMAKE_COMMAND} -Dpatch_file=${llgl_patch_file} -Dwith_reset=TRUE -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/git-patch.cmake)

FetchContent_Declare(
    llgl
    GIT_REPOSITORY https://github.com/LukasBanana/LLGL.git
    GIT_TAG 95f961ac9c917cca5e5609f69c32bc5e373d0802
    PATCH_COMMAND ${llgl_apply_patch_command}
)
FetchContent_MakeAvailable(llgl)

if(LLGL_BUILD_RENDERER_NULL)
link_libraries(LLGL_Null)
endif()

if(LLGL_BUILD_RENDERER_VULKAN)
link_libraries(LLGL_Vulkan)
endif()

if(LLGL_BUILD_RENDERER_OPENGL)
link_libraries(LLGL_OpenGL)
endif()

if(LLGL_BUILD_RENDERER_DIRECT3D11)
link_libraries(LLGL_Direct3D11)
endif()

if(LLGL_BUILD_RENDERER_DIRECT3D12)
link_libraries(LLGL_Direct3D12)
endif()

if(LLGL_BUILD_RENDERER_METAL)
link_libraries(LLGL_Metal)
endif()

link_libraries(LLGL)

target_include_directories(${PROJECT_NAME} PUBLIC ${llgl_SOURCE_DIR})

find_package(glslang REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${glslang_SOURCE_DIR}/glslang/Public)
target_link_libraries(${PROJECT_NAME} PRIVATE glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper)

find_package(spirv_cross_core REQUIRED)
# if(LLGL_BUILD_RENDERER_OPENGL)
    find_package(spirv_cross_glsl REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${spirv_cross_glsl_SOURCE_DIR}/include)
    target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-glsl)
# endif()
# if(LLGL_BUILD_RENDERER_VULKAN OR LLGL_BUILD_RENDERER_DIRECT3D11 OR LLGL_BUILD_RENDERER_DIRECT3D12)
    find_package(spirv_cross_hlsl REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${spirv_cross_hlsl_SOURCE_DIR}/include)
    target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-hlsl)
# endif()
# if(LLGL_BUILD_RENDERER_METAL)
    find_package(spirv_cross_msl REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${spirv_cross_msl_SOURCE_DIR}/include)
    target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-msl)
# endif()
find_package(spirv_cross_cpp REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${spirv_cross_core_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${spirv_cross_cpp_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-core)
target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-cpp)

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
target_link_libraries(${PROJECT_NAME} PRIVATE stb)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    include(cmake/mac.cmake)
endif()
