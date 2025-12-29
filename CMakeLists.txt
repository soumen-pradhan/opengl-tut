cmake_minimum_required(VERSION 4.1.0)
project(opengl-tut VERSION 0.1.0 LANGUAGES CXX)

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

    PRIVATE
        src/main.cpp
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>>:-Wall -Wextra -Wpedantic>
)
