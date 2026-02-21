#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>

class Shader {
public:
    uint32_t ID;

    static std::optional<Shader> init(const std::filesystem::path& vertexShaderPath, const std::filesystem::path& fragShaderPath);

    void useProgram() const;
    void setUniformBool(const char* name, bool v) const;
    void setUniformInt(const char* name, int v) const;
    void setUniformFloat(const char* name, float v) const;
    void setUniformMatrix4f(const char* name, glm::mat4 v) const;
    void setUniformVec3f(const char* name, glm::vec3 v) const;
};
