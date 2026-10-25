// The face model: what the eyes look like, as plain numbers. No drawing
// code here; the renderer turns these into pixels (docs/face-plan.md in
// DontFall, "The code's shape").
//
// Units are logical pixels: the screen as the face sees it, origin top
// left, y down, whatever way the panel is mounted.
#pragma once

#include "mood.hpp"

namespace dontfall::face {

// One eye: a rounded rectangle, with a straight top lid and a straight or
// round bottom lid cutting parts of it away.
struct Eye {
    float x = 0;  // centre
    float y = 0;
    float width = 0;
    float height = 0;
    float radius = 0;      // corner radius
    float lid_top = 0;     // hidden from the top, at the eye's centre line
    float lid_angle = 0;   // top lid tilt in radians, + = clockwise on screen
    float lid_bottom = 0;  // hidden from the bottom, at the centre line
    float lid_curve = 0;   // bottom lid curvature, 1 / its radius; 0 = straight
};

struct Face {
    Eye left;
    Eye right;
    Color color;
};

// Where the eyes look: -1..1 from the middle, + = right and down.
struct Gaze {
    float x = 0;
    float y = 0;
};

inline Gaze mix(const Gaze &a, const Gaze &b, float t)
{
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

// What changes from moment to moment on top of the mood.
struct Look {
    Gaze gaze;
    float blink = 0;      // 0 open .. 1 shut
    float squash_x = 0;   // 0..1, squeezed sideways / up and down (googly
    float squash_y = 0;   //   eyes hitting their edge)
};

// The eyes for a mood and a look on a screen of the given logical size.
inline Face make_face(float screen_width, float screen_height, const Mood &mood,
                      const Look &look = {})
{
    // The neutral eye, where the two sit, and how far they move to look.
    const float base_width = screen_width * 0.22f;
    const float base_height = screen_height * 0.45f;
    const float spacing = screen_width * 0.22f;  // eye centre to screen centre
    const float reach_x = screen_width * 0.15f;
    const float reach_y = screen_height * 0.24f;
    // Looking to one side, the eye on that side grows and the other shrinks
    // a little: a hint of the head turning.
    const float turn = 0.08f * look.gaze.x;
    // Shut is a thin line, not nothing.
    const float open = 1 - 0.92f * look.blink;
    // A full squash takes this much off the size along the hit.
    constexpr float squash_depth = 0.25f;
    const float squeeze_x = 1 - squash_depth * look.squash_x;
    const float squeeze_y = 1 - squash_depth * look.squash_y;

    auto make_eye = [&](float x, float size, float lid_angle) {
        Eye eye;
        eye.x = x + look.gaze.x * reach_x;
        eye.y = screen_height / 2 + look.gaze.y * reach_y;
        eye.width = base_width * mood.width * size * squeeze_x;
        eye.height = base_height * mood.height * size * open * squeeze_y;
        eye.radius = eye.width * mood.radius;
        eye.lid_top = eye.height * mood.lid_top;
        eye.lid_angle = lid_angle;
        eye.lid_bottom = eye.height * mood.lid_bottom;
        // smile 1: a bottom lid about as round as the eye is wide
        eye.lid_curve = mood.smile / (0.6f * eye.width);
        return eye;
    };
    // The lid angle is towards the nose for both eyes: mirrored on screen.
    return {make_eye(screen_width / 2 - spacing, 1 - turn, mood.lid_angle),
            make_eye(screen_width / 2 + spacing, 1 + turn, -mood.lid_angle), mood.color};
}

} // namespace dontfall::face
