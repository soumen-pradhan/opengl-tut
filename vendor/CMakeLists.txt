# This cmake build file is to adapt libraries that do not support cmake

cmake_minimum_required(VERSION 3.24)
project(cmake-adapter C CXX)

# Adapted from
# vcpkg/ports/imgui/CMakeLists.txt
# imgui/examples/example_glfw_opengl3/Makefile

add_library(imgui "")
add_library(imgui::imgui ALIAS imgui)

set_target_properties(imgui
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        DEBUG_POSTFIX d
)

target_sources(imgui
    PUBLIC
        FILE_SET common
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/imgui
        FILES
            imgui/imconfig.h
            imgui/imgui_internal.h
            imgui/imgui.h
            imgui/imstb_rectpack.h
            imgui/imstb_textedit.h
            imgui/imstb_truetype.h

        FILE_SET impl
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
        FILES
            imgui/backends/imgui_impl_glfw.h
            imgui/backends/imgui_impl_opengl3.h

    PRIVATE
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/misc/cpp/imgui_stdlib.cpp
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl3.cpp
)

target_link_libraries(imgui
    PRIVATE
        glfw
        OpenGL::GL
)
