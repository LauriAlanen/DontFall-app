// Idle life: what keeps the eyes from looking like a still picture. Random
// blinks (sometimes double) and glances: the gaze jumps quickly to a new
// point, holds, and often comes back to the middle. Small, quick movements
// are what make eyes look alive. Moods are mood_picker.hpp's job.
#pragma once

#include <chrono>
#include <random>

#include "face.hpp"
#include "tween.hpp"

namespace dontfall::face {

class Idle {
public:
    explicit Idle(Clock::time_point now);

    // Where the eyes look and how far they are closed at this moment.
    Look at(Clock::time_point now);

private:
    float uniform(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng_); }
    Clock::duration seconds(float lo, float hi);
    float blink_amount(Clock::time_point now) const;

    std::minstd_rand rng_;
    Clock::time_point next_blink_;
    Clock::time_point blink_start_ {};
    bool double_blink_ = false;
    Tween<Gaze> gaze_;
    Clock::time_point next_glance_;
};

} // namespace dontfall::face
