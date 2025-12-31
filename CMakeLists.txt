cmake_minimum_required(VERSION 4.1.0)
project(opengl-tut VERSION 0.1.0 LANGUAGES C CXX)

find_package(OpenGL REQUIRED)

add_subdirectory(vendor/spdlog)

option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
add_subdirectory(vendor/glfw)

add_subdirectory(vendor)

add_executable(${PROJECT_NAME} "")

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

target_sources(${PROJECT_NAME}
    PUBLIC
        FILE_SET common
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src/include
        FILES
            src/include/defer.h
            src/include/shader.h

    PUBLIC
        FILE_SET gladHeaders
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/include
        FILES
            vendor/glad/include/glad/glad.h


    PUBLIC
        FILE_SET stb_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb
        FILES
            vendor/stb/stb_image.h

    PRIVATE
        src/main.cpp
        src/shader.cpp
        vendor/glad/src/glad.c
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        spdlog::spdlog
        glfw
        OpenGL::GL
        imgui::imgui
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4 >
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>>:-Wall -Wextra -Wpedantic>

    $<$<CXX_COMPILER_ID:MSVC>:/W4 /EHs-c->
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wall -Wextra -Wpedantic -fno-exceptions>
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        SPDLOG_NO_EXCEPTIONS
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
        _HAS_EXCEPTIONS=0
)

# Copy Shader files
file(GLOB_RECURSE SHADERS
    "${CMAKE_SOURCE_DIR}/src/shaders/*"
)

add_custom_target(copy_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/src/shaders
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    DEPENDS ${SHADERS}
)

add_dependencies(${PROJECT_NAME} copy_shaders)

# Copy texture files
file(GLOB_RECURSE TEXTURES
    "${CMAKE_SOURCE_DIR}/src/textures/*"
)

add_custom_target(copy_textures ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/src/textures
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/textures
    DEPENDS ${TEXTURES}
)

add_dependencies(${PROJECT_NAME} copy_textures)
