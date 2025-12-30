#include <filesystem>
#include <fstream>
#include <stdint.h>
#include <string>

#include <glad/glad.h> // glad has to be above glfw3 header

#include <GLFW/glfw3.h>
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>

#include "defer.h"

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

std::string loadFile(const std::filesystem::path& path);

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

    uint32_t vertexShader = 0;
    {
        const std::string vertexShaderSrc = loadFile("shaders/shader.vert");
        const char* vertexShaderSrcPtr = vertexShaderSrc.c_str();

        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSrcPtr, NULL);
        glCompileShader(vertexShader);

        int success = GL_FALSE;
        char infoLog[512] = { 0 };
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE) {
            glGetShaderInfoLog(vertexShader, sizeof(infoLog), NULL, infoLog);
            SPDLOG_ERROR("Vertex Shader compilation: {}", infoLog);
        }
    }

    uint32_t fragShader = 0;
    {
        const std::string fragShaderSrc = loadFile("shaders/shader.frag");
        const char* fragShaderSrcPtr = fragShaderSrc.c_str();

        fragShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragShader, 1, &fragShaderSrcPtr, NULL);
        glCompileShader(fragShader);

        int success = GL_FALSE;
        char infoLog[512] = { 0 };
        glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE) {
            glGetShaderInfoLog(fragShader, sizeof(infoLog), NULL, infoLog);
            SPDLOG_ERROR("Frag Shader compilation: {}", infoLog);
        }
    }

    uint32_t fragShader2 = 0;
    {
        const std::string fragShader2Src = loadFile("shaders/shader2.frag");
        const char* fragShader2SrcPtr = fragShader2Src.c_str();

        fragShader2 = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragShader2, 1, &fragShader2SrcPtr, NULL);
        glCompileShader(fragShader2);

        int success = GL_FALSE;
        char infoLog[512] = { 0 };
        glGetShaderiv(fragShader2, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE) {
            glGetShaderInfoLog(fragShader2, sizeof(infoLog), NULL, infoLog);
            SPDLOG_ERROR("Frag Shader compilation: {}", infoLog);
        }
    }

    uint32_t shaderProgram = 0;
    uint32_t shaderProgram2 = 0;
    {
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragShader);
        glLinkProgram(shaderProgram);

        shaderProgram2 = glCreateProgram();
        glAttachShader(shaderProgram2, vertexShader);
        glAttachShader(shaderProgram2, fragShader2);
        glLinkProgram(shaderProgram2);

        DEFER({
            glDeleteShader(vertexShader);
            glDeleteShader(fragShader);
            glDeleteShader(fragShader2);
        });

        int success = GL_FALSE;
        char infoLog[512] = { 0 };
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (success == GL_FALSE) {
            glGetProgramInfoLog(shaderProgram, sizeof(infoLog), NULL, infoLog);
            SPDLOG_ERROR("Shader linking: {}", infoLog);
        }

        glGetProgramiv(shaderProgram2, GL_LINK_STATUS, &success);
        if (success == GL_FALSE) {
            glGetProgramInfoLog(shaderProgram2, sizeof(infoLog), NULL, infoLog);
            SPDLOG_ERROR("Shader2 linking: {}", infoLog);
        }
    }
    DEFER({
        glDeleteProgram(shaderProgram);
        glDeleteProgram(shaderProgram2);
    });

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // render
        {
            float timeValue = (float)glfwGetTime();
            int timeValueLoc = glGetUniformLocation(shaderProgram, "timeValue");

            glUseProgram(shaderProgram);
            if (timeValueLoc == -1) {
                SPDLOG_ERROR("Could not find timeValue uniform location");
            }
            glUniform1f(timeValueLoc, timeValue);

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

std::string loadFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return "";
    }

    std::string data(std::filesystem::file_size(path), '\0');
    file.read(data.data(), data.size());
    return data;
}
