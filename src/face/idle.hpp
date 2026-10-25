// Idle life: what keeps the eyes from looking like a still picture. Random
// blinks (sometimes double) and glances: the gaze jumps quickly to a new
// point, holds, and often comes back to the middle. Small, quick movements
// are what make eyes look alive. Now and then a passing mood, and a
// surprised look when the eyes get knocked about.
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

    // True and the mood to change to, when the idle life changes its mood.
    bool mood_change(Clock::time_point now, Mood &target);
    // The eyes hit their edge hard (googly): look surprised for a moment.
    void bump(Clock::time_point now);

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
    size_t mood_ = 0;  // index in moods, 0 = neutral
    Clock::time_point mood_until_;
    bool mood_changed_ = false;
};

} // namespace dontfall::face
