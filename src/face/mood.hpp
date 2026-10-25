// Moods: how the eyes are shaped, as a few numbers relative to the neutral
// eye. An expression is just a different set of numbers; tween.hpp moves
// smoothly from one set to another.
#pragma once

#include <array>
#include <string_view>

namespace dontfall::face {

struct Color {
    float r = 0;
    float g = 0;
    float b = 0;
};

struct Mood {
    float width = 1;       // times the neutral eye's size
    float height = 1;
    float radius = 0.25f;  // corner radius, times the eye's width
    float lid_top = 0;     // part of the eye's height hidden from the top, 0..1
    float lid_angle = 0;   // top lid tilt in radians, + = inner corners lower
    float lid_bottom = 0;  // part of the eye's height hidden from the bottom, 0..1
    float smile = 0;       // bottom lid shape: 0 straight .. 1 round (happy)
    Color color {0.15f, 0.85f, 1.0f};  // cyan
    float glitch = 0;  // bands of the eyes jump sideways (the fault look), 0..1
};

// a + (b - a) * t for every field
inline Mood mix(const Mood &a, const Mood &b, float t)
{
    auto m = [t](float x, float y) { return x + (y - x) * t; };
    return {m(a.width, b.width),
            m(a.height, b.height),
            m(a.radius, b.radius),
            m(a.lid_top, b.lid_top),
            m(a.lid_angle, b.lid_angle),
            m(a.lid_bottom, b.lid_bottom),
            m(a.smile, b.smile),
            {m(a.color.r, b.color.r), m(a.color.g, b.color.g), m(a.color.b, b.color.b)},
            m(a.glitch, b.glitch)};
}

struct NamedMood {
    std::string_view name;
    Mood mood;
};

// The mood table. Tune the numbers here. Columns: width, height, radius,
// lid_top, lid_angle, lid_bottom, smile, colour, glitch.
inline constexpr std::array<NamedMood, 9> moods = {{
    {"neutral", {}},
    {"happy", {1.05f, 1.0f, 0.3f, 0, 0, 0.45f, 1, {1.0f, 0.8f, 0.2f}}},          // warm yellow
    {"sad", {1.0f, 0.9f, 0.25f, 0.3f, -0.35f, 0, 0, {0.25f, 0.4f, 1.0f}}},      // deep blue
    {"angry", {1.0f, 0.9f, 0.2f, 0.35f, 0.4f, 0, 0, {1.0f, 0.2f, 0.1f}, 1}},    // red, glitching
    {"surprised", {1.15f, 1.2f, 0.5f, 0, 0, 0, 0, {0.75f, 0.95f, 1.0f}}},       // pale, bright
    {"sleepy", {1.0f, 1.0f, 0.25f, 0.55f, -0.1f, 0.1f, 0, {0.1f, 0.45f, 0.55f}}},  // dim
    {"focused", {1.0f, 0.85f, 0.2f, 0.15f, 0.12f, 0.05f, 0, {0.2f, 1.0f, 0.7f}}},  // green-cyan
    {"asleep", {1.05f, 1.0f, 0.25f, 0.9f, 0, 0.05f, 0, {0.05f, 0.25f, 0.3f}}},     // nearly shut, dark
    {"squint", {1.05f, 0.95f, 0.3f, 0.32f, 0.08f, 0.3f, 0, {0.45f, 0.8f, 1.0f}}},  // poked
}};

inline constexpr Mood neutral = moods[0].mood;

// The mood called name, neutral if there is none.
inline const Mood &mood_named(std::string_view name)
{
    for (const auto &m : moods)
        if (m.name == name)
            return m.mood;
    return moods[0].mood;
}

// The name of a mood from the table, "?" for any other.
inline std::string_view mood_name(const Mood *mood)
{
    for (const auto &m : moods)
        if (&m.mood == mood)
            return m.name;
    return "?";
}

} // namespace dontfall::face
