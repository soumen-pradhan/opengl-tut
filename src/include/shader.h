#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <filesystem>

class Shader {
public:
    uint32_t ID;

    Shader(const std::filesystem::path& vertexShader, const std::filesystem::path& fragShader);

    void useProgram() const;
    void setUniformBool(const char* name, bool v) const;
    void setUniformInt(const char* name, int v) const;
    void setUniformFloat(const char* name, float v) const;
    void setUniformMatrix4f(const char* name, glm::mat4 v) const;
};
