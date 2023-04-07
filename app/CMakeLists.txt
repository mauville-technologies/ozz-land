set(SOURCES
        src/main.cpp
)


add_executable(${PROJECT_NAME} ${SOURCES})


target_include_directories(${PROJECT_NAME}
    PRIVATE
        OZZ_VULKAN
    SYSTEM
        spdlog
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OZZ_VULKAN
        spdlog
)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED On)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_EXTENSIONS Off)