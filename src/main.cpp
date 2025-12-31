#include <cstdint>

#include <glad/glad.h> // glad has to be above glfw3 header

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include "defer.h"
#include "shader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

int main()
{
    spdlog::set_level(spdlog::level::trace);
    spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e %^%-8l%$ | %s:%5# | %v");

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
    SPDLOG_DEBUG("glfwCreateWindow");
    DEFER(glfwDestroyWindow(window));

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        SPDLOG_ERROR("Failed to init GLAD");
        return 1;
    }

    SPDLOG_INFO("Loaded OpenGL {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    uint32_t vertexBufferObj = 0;
    uint32_t vertexArrayObj = 0;
    uint32_t elementBufferObj = 0;
    {
        // clang-format off
        float vertices[] = {
            // positions          // colors            // tex
             0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
             0.5f, -0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
        };

        uint32_t indices[] = {
            0, 1, 3,
            1, 2, 3
        };
        // clang-format on

        glGenBuffers(1, &vertexBufferObj);
        glGenVertexArrays(1, &vertexArrayObj);
        glGenBuffers(1, &elementBufferObj);

        glBindVertexArray(vertexArrayObj);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObj);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferObj);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
    SPDLOG_DEBUG("Created Vertice Buffer and Layout Objects");
    DEFER({
        glDeleteBuffers(1, &vertexBufferObj);
        glDeleteVertexArrays(1, &vertexArrayObj);
    });

    Shader shader("shaders/shader.vert", "shaders/shader.frag");
    SPDLOG_DEBUG("Created Shader Program");
    DEFER(glDeleteProgram(shader.ID));

    stbi_set_flip_vertically_on_load(true);

    uint32_t texture = 0;
    {
        int width, height, nrChannels;
        const uint8_t* data = stbi_load("textures/Texturelabs_Brick_159M.jpg",
            &width, &height, &nrChannels, 0);
        DEFER(stbi_image_free((void*)data));

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
            GL_RGB, GL_UNSIGNED_BYTE, (void*)data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    SPDLOG_DEBUG("Created Texture");
    DEFER(glDeleteTextures(1, &texture));

    uint32_t texture1 = 0;
    {
        int width, height, nrChannels;
        const uint8_t* data = stbi_load("textures/Texturelabs_InkPaint_241M.png",
            &width, &height, &nrChannels, 0);
        DEFER(stbi_image_free((void*)data));

        glGenTextures(1, &texture1);
        glBindTexture(GL_TEXTURE_2D, texture1);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, (void*)data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    SPDLOG_DEBUG("Created Texture1");
    DEFER(glDeleteTextures(1, &texture1));

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // render
        {
            shader.useProgram();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            shader.setUniformInt("texture0", 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture1);
            shader.setUniformInt("texture1", 1);

            glBindVertexArray(vertexArrayObj);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
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
