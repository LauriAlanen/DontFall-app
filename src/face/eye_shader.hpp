// GLSL ES 1.00 (OpenGL ES 2.0) shaders that draw one eye.
//
// The renderer draws one rectangle per eye, just big enough for the eye and
// its glow; the GC400 is too slow to run this shader for every pixel of the
// screen (face plan step 3: 8 fps full screen). The rectangle is made of
// horizontal bands: for the glitch (the fault look) the vertex shader moves
// some of them sideways, which costs nothing per pixel. The fragment shader runs
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
attribute vec2 a_corner;  // corner of a band in the unit square, 0..1
attribute float a_band;   // which band

uniform vec4 u_rect;    // the rectangle to draw, logical: x0, y0, x1, y1
uniform vec2 u_center;  // the eye's centre, logical
uniform vec3 u_style;   // 1 / rim width, 1 / half height, unused
uniform vec2 u_glitch;  // how far bands jump (logical pixels), time step

// Logical pixels (origin top left, rotated) from panel pixels (origin
// bottom left): logical = (row0 . p, row1 . p) + offset. Here it runs
// backwards; the matrix only swaps and flips axes, so its inverse is its
// transpose.
uniform vec2 u_row0;
uniform vec2 u_row1;
uniform vec2 u_offset;
uniform vec2 u_panel;  // panel size in pixels

// Position relative to the eye's centre, for the fragment shader: done
// here, per corner, instead of per pixel. The same for the shading from
// top to bottom: it is linear, so the GPU interpolates it for free.
varying vec2 v_pos;
varying float v_shade;

float hash(float n)
{
    return fract(sin(n) * 43758.5453);
}

void main()
{
    vec2 pos = mix(u_rect.xy, u_rect.zw, a_corner);
    v_pos = pos - u_center;
    v_shade = 0.8 - 0.15 * v_pos.y * u_style.y;
    if (u_glitch.x > 0.0) {
        // A new random pattern every time step: about a third of the bands
        // jump left or right, and those flicker brighter.
        float jumps = step(0.65, hash(a_band * 12.9898 + u_glitch.y * 78.233));
        float side = hash(a_band * 4.1414 + u_glitch.y * 9.7) * 2.0 - 1.0;
        v_pos.x += jumps * side * u_glitch.x;
        v_shade += jumps * 0.3;
    }
    vec2 q = pos - u_offset;
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

// Everything relative to the eye's centre, and worked out on the CPU
// where it can be: the GC400 is slow at per-pixel maths, divisions most.
uniform vec2 u_half;     // half width, half height
uniform float u_radius;  // corner radius, at most the smaller half size
uniform vec3 u_top;      // top lid line: normal x, y (pointing at the cut
                         //   side), distance from the eye's centre
uniform vec3 u_bottom;   // bottom lid circle: centre x, y, radius
uniform vec2 u_lids;     // 1 = the top / bottom lid is used
uniform vec3 u_color;
uniform vec2 u_glow;     // 1 / width (pixels to fade to zero), strength at the edge
uniform vec3 u_style;    // 1 / rim width, 1 / half height (vertex shader)

varying vec2 v_pos;
varying float v_shade;

// Distance from p to a rectangle with rounded corners centred at 0.
float rounded_box(vec2 p, vec2 half_size, float radius)
{
    vec2 q = abs(p) - half_size + radius;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
}

void main()
{
    float d = rounded_box(v_pos, u_half, u_radius);
    if (u_lids.x > 0.5)
        d = max(d, dot(u_top.xy, v_pos) - u_top.z);            // above the top lid
    if (u_lids.y > 0.5)
        d = max(d, u_bottom.z - length(v_pos - u_bottom.xy));  // inside the bottom lid

    float coverage = clamp(0.5 - d, 0.0, 1.0);

    // Inside: brighter along the rim (the same distance again), a little
    // darker towards the bottom.
    float rim = clamp(1.0 + d * u_style.x, 0.0, 1.0);
    float shade = v_shade + 0.35 * rim;

    float fade = clamp(1.0 - d * u_glow.x, 0.0, 1.0);
    float glow = u_glow.y * fade * fade;
    gl_FragColor = vec4(u_color * (coverage * shade + (1.0 - coverage) * glow), 1.0);
}
)glsl";

} // namespace dontfall::face
