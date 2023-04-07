cmake_policy(SET CMP0048 NEW)

project(OZZ_LAND VERSION 0.1)

CMAKE_MINIMUM_REQUIRED(VERSION 3.23)

find_package(Vulkan 1.3.243 REQUIRED)

# Libraries
add_subdirectory(external)

# Renderer
add_subdirectory(ozz_vulkan)

# Main Application
add_subdirectory(app)