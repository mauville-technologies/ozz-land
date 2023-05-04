cmake_policy(SET CMP0048 NEW)

project(OZZ_LAND VERSION 0.1)

CMAKE_MINIMUM_REQUIRED(VERSION 3.23)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist/${CMAKE_SYSTEM_NAME})

#enable C++20
set(CMAKE_CXX_STANDARD 20)

set(ASSETS_DIR_NAME "assets")


# Libraries
add_subdirectory(external)

# Renderer
add_subdirectory(ozz_vulkanxr)

# Main Application
add_subdirectory(app)