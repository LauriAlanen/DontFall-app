// GLSL ES 1.00 (OpenGL ES 2.0) shaders that draw the eyes.
//
// One rectangle covers the whole screen; the fragment shader runs once per
// pixel and decides its colour from signed distance functions (SDF): the
// distance from the pixel to the eye's edge, negative inside. Coverage =
// 0.5 - distance, clamped, gives a one pixel anti-aliased edge.
//
// The lids cut shapes away: the top lid everything above a line, the
// bottom lid everything inside a circle. Keeping the part of the eye
// outside both is max() of the distances.
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

uniform vec4 u_left;         // centre x, y, half width, half height
uniform vec4 u_right;
uniform vec2 u_radius;       // corner radius: left, right
uniform vec3 u_left_top;     // top lid line: normal x, y (pointing at the
uniform vec3 u_right_top;    //   cut side), distance from the origin
uniform vec3 u_left_bottom;  // bottom lid circle: centre x, y, radius
uniform vec3 u_right_bottom;
uniform vec3 u_color;

// Distance from p to a rectangle with rounded corners centred at 0.
float rounded_box(vec2 p, vec2 half_size, float radius)
{
    vec2 q = abs(p) - half_size + radius;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
}

float eye(vec2 p, vec4 shape, float radius, vec3 top, vec3 bottom)
{
    float r = min(radius, min(shape.z, shape.w));
    float d = rounded_box(p - shape.xy, shape.zw, r);
    d = max(d, dot(top.xy, p) - top.z);              // above the top lid
    d = max(d, bottom.z - length(p - bottom.xy));    // inside the bottom lid
    return d;
}

void main()
{
    vec2 fc = gl_FragCoord.xy;
    vec2 p = vec2(dot(u_row0, fc), dot(u_row1, fc)) + u_offset;

    float d = min(eye(p, u_left, u_radius.x, u_left_top, u_left_bottom),
                  eye(p, u_right, u_radius.y, u_right_top, u_right_bottom));
    float coverage = clamp(0.5 - d, 0.0, 1.0);
    gl_FragColor = vec4(u_color * coverage, 1.0);
}
)glsl";

} // namespace dontfall::face
