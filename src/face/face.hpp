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
};

// The eyes for a mood on a screen of the given logical size.
inline Face make_face(float screen_width, float screen_height, const Mood &mood)
{
    // The neutral eye and where the two sit.
    const float base_width = screen_width * 0.22f;
    const float base_height = screen_height * 0.45f;
    const float spacing = screen_width * 0.22f;  // eye centre to screen centre

    Eye eye;
    eye.y = screen_height / 2;
    eye.width = base_width * mood.width;
    eye.height = base_height * mood.height;
    eye.radius = eye.width * mood.radius;
    eye.lid_top = eye.height * mood.lid_top;
    eye.lid_bottom = eye.height * mood.lid_bottom;
    // smile 1: a bottom lid about as round as the eye is wide
    eye.lid_curve = mood.smile / (0.6f * eye.width);

    // The lid angle is towards the nose for both eyes: mirrored on screen.
    Face face;
    face.left = eye;
    face.left.x = screen_width / 2 - spacing;
    face.left.lid_angle = mood.lid_angle;
    face.right = eye;
    face.right.x = screen_width / 2 + spacing;
    face.right.lid_angle = -mood.lid_angle;
    return face;
}

} // namespace dontfall::face
