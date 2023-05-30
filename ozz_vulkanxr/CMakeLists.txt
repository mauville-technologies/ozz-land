project(OZZ_VULKANXR)

CMAKE_MINIMUM_REQUIRED(VERSION 3.23)

find_package(Vulkan REQUIRED)

set(SOURCES
        include/ozz_vulkan/internal/vk_utils.h
        src/renderer.cpp
        src/vma_implementation.cpp
        src/shader.cpp
        src/buffer.cpp
        )


add_library(${PROJECT_NAME} ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PUBLIC
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_FORCE_RADIANS
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Vulkan::Vulkan
        spdlog
        VulkanMemoryAllocator
        glm
        openxr_loader
    PRIVATE
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        include/
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED On)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_EXTENSIONS Off)