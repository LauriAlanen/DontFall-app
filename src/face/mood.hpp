// Moods: how the eyes are shaped, as a few numbers relative to the neutral
// eye. An expression is just a different set of numbers; tween.hpp moves
// smoothly from one set to another.
#pragma once

#include <array>
#include <string_view>

namespace dontfall::face {

struct Mood {
    float width = 1;       // times the neutral eye's size
    float height = 1;
    float radius = 0.25f;  // corner radius, times the eye's width
    float lid_top = 0;     // part of the eye's height hidden from the top, 0..1
    float lid_angle = 0;   // top lid tilt in radians, + = inner corners lower
    float lid_bottom = 0;  // part of the eye's height hidden from the bottom, 0..1
    float smile = 0;       // bottom lid shape: 0 straight .. 1 round (happy)
};

// a + (b - a) * t for every field
inline Mood mix(const Mood &a, const Mood &b, float t)
{
    auto m = [t](float x, float y) { return x + (y - x) * t; };
    return {m(a.width, b.width),     m(a.height, b.height),         m(a.radius, b.radius),
            m(a.lid_top, b.lid_top), m(a.lid_angle, b.lid_angle), m(a.lid_bottom, b.lid_bottom),
            m(a.smile, b.smile)};
}

struct NamedMood {
    std::string_view name;
    Mood mood;
};

// The mood table. Tune the numbers here.
inline constexpr std::array<NamedMood, 6> moods = {{
    {"neutral", {}},
    {"happy", {1.05f, 1.0f, 0.3f, 0, 0, 0.45f, 1}},
    {"sad", {1.0f, 0.9f, 0.25f, 0.3f, -0.35f, 0, 0}},
    {"angry", {1.0f, 0.9f, 0.2f, 0.35f, 0.4f, 0, 0}},
    {"surprised", {1.15f, 1.2f, 0.5f, 0, 0, 0, 0}},
    {"sleepy", {1.0f, 1.0f, 0.25f, 0.55f, -0.1f, 0.1f, 0}},
}};

inline constexpr Mood neutral = moods[0].mood;

} // namespace dontfall::face
