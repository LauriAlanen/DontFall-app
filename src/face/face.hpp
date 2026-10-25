// The face model: what the eyes look like, as plain numbers. No drawing
// code here; the renderer turns these into pixels (docs/face-plan.md in
// DontFall, "The code's shape").
//
// Units are logical pixels: the screen as the face sees it, origin top
// left, whatever way the panel is mounted.
#pragma once

namespace dontfall::face {

// One eye: a rounded rectangle.
struct Eye {
    float x = 0;  // centre
    float y = 0;
    float width = 0;
    float height = 0;
    float radius = 0;  // corner radius
};

struct Face {
    Eye left;
    Eye right;
};

// The resting look, centred on a screen of the given logical size. Hard
// coded until moods arrive (face plan step 3).
inline Face neutral_face(float screen_width, float screen_height)
{
    const float cx = screen_width / 2;
    const float cy = screen_height / 2;
    const float spacing = screen_width * 0.22f;  // eye centre to screen centre
    Eye eye;
    eye.y = cy;
    eye.width = screen_width * 0.22f;
    eye.height = screen_height * 0.45f;
    eye.radius = eye.width * 0.25f;

    Face face;
    face.left = eye;
    face.left.x = cx - spacing;
    face.right = eye;
    face.right.x = cx + spacing;
    return face;
}

} // namespace dontfall::face
