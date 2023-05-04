project(OZZ_VULKANXR)

CMAKE_MINIMUM_REQUIRED(VERSION 3.23)

find_package(Vulkan 1.3.243 REQUIRED)

set(SOURCES
        include/ozz_vulkan/vk_utils.h
        src/renderer.cpp
        src/vma_implementation.cpp)


add_library(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Vulkan::Vulkan
        spdlog
        VulkanMemoryAllocator
    PRIVATE
        openxr_loader
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        include/
        ${Vulkan_INCLUDE_DIRS}
        VulkanMemoryAllocator
    PRIVATE
        openxr_loader
    SYSTEM
        spdlog
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED On)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_EXTENSIONS Off)