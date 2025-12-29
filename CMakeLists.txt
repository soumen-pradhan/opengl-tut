cmake_minimum_required(VERSION 4.1.0)
project(opengl-tut VERSION 0.1.0 LANGUAGES C CXX)

add_subdirectory(vendor/spdlog)
add_subdirectory(vendor/glfw)

find_package(OpenGL REQUIRED)

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

    PUBLIC
        FILE_SET gladHeaders
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/include
        FILES
            vendor/glad/include/glad/glad.h

    PRIVATE
        src/main.cpp
        vendor/glad/src/glad.c
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        spdlog::spdlog
        glfw
        OpenGL::GL
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wall -Wextra -Wpedantic>
)
