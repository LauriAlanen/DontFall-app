#include "googly.hpp"

#include <algorithm>
#include <cmath>

namespace dontfall::face {

namespace {

constexpr float pi = 3.14159265f;

// The spring: wobbles at 3.5 Hz, damped to 0.4 of critical, so a jolt
// rings once or twice and settles. Gaze units are -1..1. Tried: 4 Hz and
// 0.25 felt stiff, 2.5 Hz chaotic and sloppy (face plan step 4).
constexpr float omega = 2 * pi * 3.5f;
constexpr float stiffness = omega * omega;
constexpr float damping = 2 * 0.4f * omega;
// How far the push moves the eyes: 0.17 of the way to the edge per m/s^2,
// so 30 degrees of tilt (4.9 m/s^2 along the screen) moves them most of
// the way. A dead zone for small pushes felt clunky (face plan step 4).
constexpr float push_gain = 0.17f * stiffness;
// The push is smoothed with this time constant (s) against noise; short
// enough that taps still come through.
constexpr float push_smoothing = 0.06f;
// Speed kept when bouncing off the edge, and how fast a squash fades (s).
constexpr float bounce = 0.4f;
constexpr float squash_fade = 0.12f;
// Speed into the edge (gaze units per second) that gives a full squash.
constexpr float full_squash_speed = 10;
// Longer frames (a stall) are integrated as this, so the spring can't blow up.
constexpr float max_dt = 0.05f;
// Dizzy: how long, how fast round (turns per second) and how wide.
constexpr float spin_time = 2.0f;
constexpr float spin_turns = 1.5f;
constexpr float spin_radius = 0.85f;

} // namespace

void Googly::spin()
{
    spin_left_ = spin_time;
    spin_phase_ = std::atan2(pos_.y, pos_.x);  // start where the eyes are
}

void Googly::update(float dt, Gaze target, float push_x, float push_y)
{
    dt = std::clamp(dt, 0.0f, max_dt);

    // Dizzy: the spring chases a point going round in a circle.
    if (spin_left_ > 0) {
        spin_left_ -= dt;
        spin_phase_ += 2 * pi * spin_turns * dt;
        target = {spin_radius * std::cos(spin_phase_), spin_radius * std::sin(spin_phase_)};
    }

    const float follow = 1 - std::exp(-dt / push_smoothing);
    push_x_ += (push_x - push_x_) * follow;
    push_y_ += (push_y - push_y_) * follow;

    // Semi-implicit Euler: velocity first, then position with the new
    // velocity. Stable for omega * dt well below 2 (here ~0.5 at 50 fps).
    vel_.x += (stiffness * (target.x - pos_.x) - damping * vel_.x + push_gain * push_x_) * dt;
    vel_.y += (stiffness * (target.y - pos_.y) - damping * vel_.y + push_gain * push_y_) * dt;
    pos_.x += vel_.x * dt;
    pos_.y += vel_.y * dt;

    const float fade = std::exp(-dt / squash_fade);
    squash_x_ *= fade;
    squash_y_ *= fade;
    hit_ = 0;

    // The round edge: back onto it, bounce the speed going outwards, and
    // squash along the direction of the hit.
    const float r = std::hypot(pos_.x, pos_.y);
    if (r > 1) {
        const float nx = pos_.x / r;
        const float ny = pos_.y / r;
        pos_ = {nx, ny};
        const float outwards = vel_.x * nx + vel_.y * ny;
        if (outwards > 0) {
            vel_.x -= (1 + bounce) * outwards * nx;
            vel_.y -= (1 + bounce) * outwards * ny;
            hit_ = std::min(1.0f, outwards / full_squash_speed);
            squash_x_ = std::max(squash_x_, hit_ * std::fabs(nx));
            squash_y_ = std::max(squash_y_, hit_ * std::fabs(ny));
        }
    }
}

} // namespace dontfall::face
