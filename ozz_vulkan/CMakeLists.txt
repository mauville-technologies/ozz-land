project(OZZ_VULKAN)

CMAKE_MINIMUM_REQUIRED(VERSION 3.23)

set(SOURCES
    src/renderer.cpp
)


add_library(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Vulkan::Vulkan
        spdlog
        VulkanMemoryAllocator
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        include/
        ${Vulkan_INCLUDE_DIRS}
    PRIVATE
    SYSTEM
        spdlog
        VulkanMemoryAllocator
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED On)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_EXTENSIONS Off)