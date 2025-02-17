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

add_executable(${PROJECT_NAME} ${ALL_FILES})

# FetchContent_Declare(
#     glfw
#     GIT_REPOSITORY https://github.com/glfw/glfw.git
#     GIT_TAG 3.4
# )

# FetchContent_MakeAvailable(glfw)

find_package(glfw3 3.3 REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# find_package(SDL2 REQUIRED)

# target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
# target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})

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
    GIT_TAG 3df9f9c9eb707d2c0cfedf9a9a26371a7cce91cb
    PATCH_COMMAND ${llgl_apply_patch_if_needed}
)
FetchContent_MakeAvailable(llgl)

target_link_libraries(${PROJECT_NAME} PRIVATE LLGL)