#include <fstream>

#include <spdlog/spdlog.h>

#include "defer.h"
#include "shader.h"

static std::string loadFile(const std::filesystem::path& path);

static std::string loadFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return "";
    }

    std::string data(std::filesystem::file_size(path), '\0');
    file.read(data.data(), data.size());
    return data;
}

Shader::Shader(const std::filesystem::path& vertexShaderPath, const std::filesystem::path& fragShaderPath)
{
    int success = GL_FALSE;
    static char infoLog[512] = { 0 };

    uint32_t vertexShader = 0;
    {
        std::string vertexShaderSrc = loadFile(vertexShaderPath);
        const char* vertexShaderSrcPtr = vertexShaderSrc.c_str();

        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSrcPtr, NULL);
        glCompileShader(vertexShader);

        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE) {
            glGetShaderInfoLog(vertexShader, sizeof(infoLog), NULL, infoLog);
            SPDLOG_ERROR("Vertex Shader compilation: {}", infoLog);
        }
    }

    uint32_t fragShader = 0;
    {
        std::string fragShaderSrc = loadFile(fragShaderPath);
        const char* fragShaderSrcPtr = fragShaderSrc.c_str();

        fragShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragShader, 1, &fragShaderSrcPtr, NULL);
        glCompileShader(fragShader);

        glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE) {
            glGetShaderInfoLog(fragShader, sizeof(infoLog), NULL, infoLog);
            SPDLOG_ERROR("Vertex Shader compilation: {}", infoLog);
        }
    }

    this->ID = glCreateProgram();
    {
        glAttachShader(this->ID, vertexShader);
        glAttachShader(this->ID, fragShader);
        glLinkProgram(this->ID);

        DEFER({
            glDeleteShader(vertexShader);
            glDeleteShader(fragShader);
        });

        glGetProgramiv(this->ID, GL_LINK_STATUS, &success);
        if (success == GL_FALSE) {
            glGetProgramInfoLog(this->ID, sizeof(infoLog), NULL, infoLog);
            SPDLOG_ERROR("Shader linking: {}", infoLog);
        }
    }
}

void Shader::useProgram() const
{
    glUseProgram(this->ID);
}

void Shader::setUniformBool(const char* name, bool value) const
{
    int loc = glGetUniformLocation(this->ID, name);
    if (loc == -1) {
        SPDLOG_WARN("Uniform {} not found", name);
    }
    glUniform1i(loc, (int)value);
}

void Shader::setUniformInt(const char* name, int value) const
{
    int loc = glGetUniformLocation(this->ID, name);
    if (loc == -1) {
        SPDLOG_WARN("Uniform {} not found", name);
    }
    glUniform1i(loc, value);
}

void Shader::setUniformFloat(const char* name, float value) const
{
    int loc = glGetUniformLocation(this->ID, name);
    if (loc == -1) {
        SPDLOG_WARN("Uniform {} not found", name);
    }
    glUniform1f(loc, value);
}
