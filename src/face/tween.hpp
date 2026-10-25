// Moves the face from its current mood to a target mood over a set time,
// with an ease-in-out curve, so every change is smooth.
#pragma once

#include <algorithm>
#include <chrono>

#include "mood.hpp"

namespace dontfall::face {

class MoodTween {
public:
    using Clock = std::chrono::steady_clock;

    explicit MoodTween(const Mood &start) : from_(start), to_(start) {}

    // Start moving towards target from wherever the face is now, also in
    // the middle of another change.
    void go(const Mood &target, Clock::duration duration, Clock::time_point now)
    {
        from_ = at(now);
        to_ = target;
        start_ = now;
        duration_ = duration;
    }

    Mood at(Clock::time_point now) const
    {
        if (duration_ <= Clock::duration::zero() || now >= start_ + duration_)
            return to_;
        const float t = std::chrono::duration<float>(now - start_) /
                        std::chrono::duration<float>(duration_);
        const float eased = t * t * (3 - 2 * t);  // smoothstep: slow, fast, slow
        return mix(from_, to_, std::clamp(eased, 0.0f, 1.0f));
    }

private:
    Mood from_;
    Mood to_;
    Clock::time_point start_ {};
    Clock::duration duration_ {};
};

} // namespace dontfall::face
