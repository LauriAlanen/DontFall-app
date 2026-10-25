// GLSL ES 1.00 (OpenGL ES 2.0) shaders that draw the eyes.
//
// One rectangle covers the whole screen; the fragment shader runs once per
// pixel and decides its colour from signed distance functions (SDF): the
// distance from the pixel to the eye's edge, negative inside. Coverage =
// 0.5 - distance, clamped, gives a one pixel anti-aliased edge.
#pragma once

namespace dontfall::face {

inline constexpr const char *eye_vertex_shader = R"glsl(
attribute vec2 a_pos;  // clip space, -1..1

void main()
{
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)glsl";

inline constexpr const char *eye_fragment_shader = R"glsl(
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

// gl_FragCoord (panel pixels, origin bottom left) to logical pixels
// (origin top left, rotated): logical = (row0 . p, row1 . p) + offset
uniform vec2 u_row0;
uniform vec2 u_row1;
uniform vec2 u_offset;

uniform vec4 u_left;     // centre x, y, half width, half height
uniform vec4 u_right;
uniform vec2 u_radius;   // corner radius: left, right
uniform vec3 u_color;

// Distance from p to a rectangle with rounded corners centred at 0.
float rounded_box(vec2 p, vec2 half_size, float radius)
{
    vec2 q = abs(p) - half_size + radius;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
}

float eye(vec2 p, vec4 shape, float radius)
{
    float r = min(radius, min(shape.z, shape.w));
    return rounded_box(p - shape.xy, shape.zw, r);
}

void main()
{
    vec2 fc = gl_FragCoord.xy;
    vec2 p = vec2(dot(u_row0, fc), dot(u_row1, fc)) + u_offset;

    float d = min(eye(p, u_left, u_radius.x), eye(p, u_right, u_radius.y));
    float coverage = clamp(0.5 - d, 0.0, 1.0);
    gl_FragColor = vec4(u_color * coverage, 1.0);
}
)glsl";

} // namespace dontfall::face
