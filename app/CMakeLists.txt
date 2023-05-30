set(SOURCES
        src/main.cpp
        src/application.cpp
        src/cube.cpp src/camera_object.cpp)


add_executable(${PROJECT_NAME} ${SOURCES})
add_dependencies(${PROJECT_NAME} COPY_ASSETS)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OZZ_VULKANXR
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED On)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_EXTENSIONS Off)

# Get all shader files
file(GLOB SHADERS shaders/*.vert shaders/*.frag)
add_custom_target(COPY_ASSETS ALL
        COMMAND ${CMAKE_COMMAND} -E echo "Copying assets to build directory"
        COMMENT "Copying assets to build directory"
        VERBATIM
)

add_custom_command(TARGET COPY_ASSETS PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ASSETS_DIR_NAME}/shaders
        COMMAND ${CMAKE_COMMAND} -E echo "Creating shader directory"
)

foreach (SHADER ${SHADERS})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    add_custom_command(TARGET COPY_ASSETS PRE_BUILD
            COMMAND
                ${Vulkan_GLSLC_EXECUTABLE} -c ${SHADER}
                -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ASSETS_DIR_NAME}/shaders/${FILE_NAME}.spv
            COMMAND ${CMAKE_COMMAND} -E echo "Compiling shader ${FILE_NAME}"
    )
endforeach (SHADER)