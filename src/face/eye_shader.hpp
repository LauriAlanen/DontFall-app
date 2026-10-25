// GLSL ES 1.00 (OpenGL ES 2.0) shaders that draw one eye.
//
// The renderer draws one rectangle per eye, just big enough for the eye and
// its glow; the GC400 is too slow to run this shader for every pixel of the
// screen (face plan step 3: 8 fps full screen). The fragment shader runs
// once per pixel of that rectangle and decides its colour from signed
// distance functions (SDF): the distance from the pixel to the eye's edge,
// negative inside. Coverage = 0.5 - distance, clamped, gives a one pixel
// anti-aliased edge.
//
// The lids cut shapes away: the top lid everything above a line, the
// bottom lid everything inside a circle. Keeping the part of the eye
// outside both is max() of the distances. A lid that isn't used is skipped:
// a branch on a uniform costs next to nothing, as every pixel takes the
// same way, and the lids are a third of the shader's time. Outside the eye
// the same distance gives a soft glow that fades to zero at the glow width.
#pragma once

namespace dontfall::face {

inline constexpr const char *eye_vertex_shader = R"glsl(
attribute vec2 a_corner;  // corner of the unit square, 0..1

uniform vec4 u_rect;  // the rectangle to draw, logical: x0, y0, x1, y1

// Logical pixels (origin top left, rotated) from panel pixels (origin
// bottom left): logical = (row0 . p, row1 . p) + offset. Here it runs
// backwards; the matrix only swaps and flips axes, so its inverse is its
// transpose.
uniform vec2 u_row0;
uniform vec2 u_row1;
uniform vec2 u_offset;
uniform vec2 u_panel;  // panel size in pixels

varying vec2 v_pos;  // logical position, for the fragment shader

void main()
{
    v_pos = mix(u_rect.xy, u_rect.zw, a_corner);
    vec2 q = v_pos - u_offset;
    vec2 panel = vec2(u_row0.x * q.x + u_row1.x * q.y, u_row0.y * q.x + u_row1.y * q.y);
    gl_Position = vec4(panel / u_panel * 2.0 - 1.0, 0.0, 1.0);
}
)glsl";

inline constexpr const char *eye_fragment_shader = R"glsl(
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

uniform vec4 u_shape;    // centre x, y, half width, half height
uniform float u_radius;  // corner radius
uniform vec3 u_top;      // top lid line: normal x, y (pointing at the cut
                         //   side), distance from the origin
uniform vec3 u_bottom;   // bottom lid circle: centre x, y, radius
uniform vec2 u_lids;     // 1 = the top / bottom lid is used
uniform vec3 u_color;
uniform vec2 u_glow;     // width (pixels to fade to zero), strength at the edge

varying vec2 v_pos;

// Distance from p to a rectangle with rounded corners centred at 0.
float rounded_box(vec2 p, vec2 half_size, float radius)
{
    vec2 q = abs(p) - half_size + radius;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
}

void main()
{
    float r = min(u_radius, min(u_shape.z, u_shape.w));
    float d = rounded_box(v_pos - u_shape.xy, u_shape.zw, r);
    if (u_lids.x > 0.5)
        d = max(d, dot(u_top.xy, v_pos) - u_top.z);            // above the top lid
    if (u_lids.y > 0.5)
        d = max(d, u_bottom.z - length(v_pos - u_bottom.xy));  // inside the bottom lid

    float coverage = clamp(0.5 - d, 0.0, 1.0);
    float fade = clamp(1.0 - d / u_glow.x, 0.0, 1.0);
    float glow = u_glow.y * fade * fade;
    gl_FragColor = vec4(u_color * (coverage + (1.0 - coverage) * glow), 1.0);
}
)glsl";

} // namespace dontfall::face
