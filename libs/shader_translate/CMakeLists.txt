cmake_minimum_required(VERSION 3.16.0)

project(shader_translate VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Library sources
set(SHADER_TRANSLATE_SOURCES
    src/shader_translate.cpp
)

set(SHADER_TRANSLATE_HEADERS
    include/shader_translate/shader_translate.h
)

# Create library
add_library(shader_translate STATIC ${SHADER_TRANSLATE_SOURCES} ${SHADER_TRANSLATE_HEADERS})

# Include directories
target_include_directories(shader_translate PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Export include directory for parent project
set(SHADER_TRANSLATE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)

# Find required dependencies
find_package(glslang REQUIRED)
find_package(spirv_cross_core REQUIRED)
find_package(spirv_cross_glsl REQUIRED)
find_package(spirv_cross_hlsl REQUIRED)
find_package(spirv_cross_msl REQUIRED)

# Link dependencies
target_link_libraries(shader_translate PUBLIC
    glslang::glslang
    glslang::SPIRV
    spirv-cross-core
    spirv-cross-glsl
    spirv-cross-hlsl
    spirv-cross-msl
)

# Optional CLI tool
option(BUILD_SHADER_TRANSLATE_CLI "Build shader_translate CLI tool" ON)
if(BUILD_SHADER_TRANSLATE_CLI)
    add_executable(shader_translate_cli tools/shader_translate_cli.cpp)
    target_link_libraries(shader_translate_cli PRIVATE shader_translate)
endif()
