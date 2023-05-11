set(SOURCES
        src/main.cpp
        src/application.cpp
        )


add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        OZZ_VULKANX
    SYSTEM
        spdlog
        openxr_loader
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OZZ_VULKANXR
        spdlog
        openxr_loader
)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED On)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_EXTENSIONS Off)


# Get all shader files
file(GLOB SHADERS shaders/*.vert shaders/*.frag)
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ASSETS_DIR_NAME}/shaders
)

foreach (SHADER ${SHADERS})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND
                ${Vulkan_GLSLC_EXECUTABLE} -c ${SHADER}
                -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ASSETS_DIR_NAME}/shaders/${FILE_NAME}.spv
    )
endforeach (SHADER)