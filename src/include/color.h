#pragma once

#include <cstdint>

#include <glm/glm.hpp>

constexpr glm::vec4 colorFromHex(uint32_t hex)
{
    const float inv255 = 1.0f / 255.0f;

    return glm::vec4(
        ((hex >> 16) & 0xFF) * inv255,
        ((hex >> 8) & 0xFF) * inv255,
        (hex & 0xFF) * inv255,
        1.0f);
}

namespace Color {

static const glm::vec4 BLACK = colorFromHex(0x000000);
static const glm::vec4 WHITE = colorFromHex(0xffffff);

static const glm::vec4 ORANGE_400 = colorFromHex(0xfb923c);
static const glm::vec4 SLATE_950 = colorFromHex(0x020617);

}
