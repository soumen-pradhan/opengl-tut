#include <fstream>

#include <glm/gtc/type_ptr.hpp>
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

std::optional<Shader> Shader::init(const std::filesystem::path& vertexShaderPath, const std::filesystem::path& fragShaderPath)
{
    Shader shader;

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
            SPDLOG_ERROR("Vertex Shader compilation ({}): {}", vertexShaderPath.string(), infoLog);
            return std::nullopt;
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
            SPDLOG_ERROR("Fragment Shader compilation ({}): {}", fragShaderPath.string(), infoLog);
            return std::nullopt;
        }
    }

    shader.ID = glCreateProgram();
    {
        glAttachShader(shader.ID, vertexShader);
        glAttachShader(shader.ID, fragShader);
        glLinkProgram(shader.ID);

        DEFER({
            glDeleteShader(vertexShader);
            glDeleteShader(fragShader);
        });

        glGetProgramiv(shader.ID, GL_LINK_STATUS, &success);
        if (success == GL_FALSE) {
            glGetProgramInfoLog(shader.ID, sizeof(infoLog), NULL, infoLog);
            SPDLOG_ERROR("Shader linking (vertex: {}, fragment: {}): {}", vertexShaderPath.string(), fragShaderPath.string(), infoLog);
            return std::nullopt;
        }
    }

    return shader;
}

void Shader::useProgram() const
{
    glUseProgram(this->ID);
}

void Shader::setUniformBool(const char* name, bool v) const
{
    int loc = glGetUniformLocation(this->ID, name);
    if (loc == -1) {
        SPDLOG_WARN("Uniform {} not found", name);
    }
    glUniform1i(loc, (int)v);
}

void Shader::setUniformInt(const char* name, int v) const
{
    int loc = glGetUniformLocation(this->ID, name);
    if (loc == -1) {
        SPDLOG_WARN("Uniform {} not found", name);
    }
    glUniform1i(loc, v);
}

void Shader::setUniformFloat(const char* name, float v) const
{
    int loc = glGetUniformLocation(this->ID, name);
    if (loc == -1) {
        SPDLOG_WARN("Uniform {} not found", name);
    }
    glUniform1f(loc, v);
}

void Shader::setUniformMatrix4f(const char* name, glm::mat4 v) const
{
    int loc = glGetUniformLocation(this->ID, name);
    if (loc == -1) {
        SPDLOG_WARN("Uniform {} not found", name);
    }
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(v));
}

void Shader::setUniformVec3f(const char* name, glm::vec3 v) const
{
    int loc = glGetUniformLocation(this->ID, name);
    if (loc == -1) {
        SPDLOG_WARN("Uniform {} not found", name);
    }
    glUniform3fv(loc, 1, glm::value_ptr(v));
}
