find_package(Vulkan REQUIRED)

add_subdirectory(spdlog)
add_subdirectory(vma)
add_subdirectory(openxr)
add_subdirectory(glm)

set_property(TARGET spdlog PROPERTY SYSTEM TRUE)
set_property(TARGET VulkanMemoryAllocator PROPERTY SYSTEM TRUE)
set_property(TARGET openxr_loader PROPERTY SYSTEM TRUE)
set_property(TARGET glm PROPERTY SYSTEM TRUE)
