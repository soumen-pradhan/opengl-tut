#include <cstdint>

#include <glad/glad.h> // glad has to be above glfw3 header

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include "defer.h"
#include "shader.h"

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

int main()
{
    spdlog::set_level(spdlog::level::trace);
    spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e %^%5l%$ | %s:%# | %v");

    if (!glfwInit()) {
        SPDLOG_ERROR("Failed to init GLFW");
        return 1;
    }
    DEFER(glfwTerminate());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int32_t SCR_WIDTH = 800;
    const int32_t SCR_HEIGHT = 600;

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Hello World", nullptr, nullptr);
    if (!window) {
        SPDLOG_ERROR("Failed to create window");
        return 1;
    }
    DEFER(glfwDestroyWindow(window));

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        SPDLOG_ERROR("Failed to init GLAD");
        return 1;
    }

    SPDLOG_INFO("OpenGL {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    uint32_t vertexBufferObj = 0;
    uint32_t vertexArrayObj = 0;
    {
        // clang-format off
        float vertices[] = {
            // positions          // colors
             0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f,
            -0.5f, -0.5f,  0.0f,  0.0f,  1.0f, 0.0f,
             0.f,   0.5f,  0.0f,  0.0f,  0.0f, 1.0f,
        };
        // clang-format on

        glGenBuffers(1, &vertexBufferObj);
        glGenVertexArrays(1, &vertexArrayObj);

        glBindVertexArray(vertexArrayObj);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObj);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
    DEFER({
        glDeleteBuffers(1, &vertexBufferObj);
        glDeleteVertexArrays(1, &vertexArrayObj);
    });

    Shader shader("shaders/shader.vert", "shaders/shader.frag");
    DEFER(glDeleteProgram(shader.ID));

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // render
        {
            float timeValue = (float)glfwGetTime();

            shader.useProgram();
            shader.setUniformFloat("timeValue", timeValue);
            shader.setUniformFloat("worldOffset", 0.5f);

            glBindVertexArray(vertexArrayObj);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        SPDLOG_DEBUG("Pressed Esc Key");
        glfwSetWindowShouldClose(window, true);
    }
}
