
add_subdirectory(spdlog)
add_subdirectory(vma)

set_property(TARGET spdlog PROPERTY SYSTEM TRUE)
set_property(TARGET VulkanMemoryAllocator PROPERTY SYSTEM TRUE)
