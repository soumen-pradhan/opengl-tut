#pragma once

#include <glad/glad.h>

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
};
