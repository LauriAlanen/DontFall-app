// Moves a value (a Mood, a Gaze, ...) from where it is to a target over a
// set time, with an ease-in-out curve, so every change is smooth. T needs
// a mix(a, b, t) function.
#pragma once

#include <algorithm>
#include <chrono>

namespace dontfall::face {

using Clock = std::chrono::steady_clock;

template <typename T>
class Tween {
public:
    explicit Tween(const T &start) : from_(start), to_(start) {}

    // Start moving towards target from wherever the value is now, also in
    // the middle of another change.
    void go(const T &target, Clock::duration duration, Clock::time_point now)
    {
        from_ = at(now);
        to_ = target;
        start_ = now;
        duration_ = duration;
    }

    // Where it is heading (or is).
    const T &target() const { return to_; }

    T at(Clock::time_point now) const
    {
        if (duration_ <= Clock::duration::zero() || now >= start_ + duration_)
            return to_;
        const float t = std::chrono::duration<float>(now - start_) /
                        std::chrono::duration<float>(duration_);
        const float eased = t * t * (3 - 2 * t);  // smoothstep: slow, fast, slow
        return mix(from_, to_, std::clamp(eased, 0.0f, 1.0f));
    }

private:
    T from_;
    T to_;
    Clock::time_point start_ {};
    Clock::duration duration_ {};
};

} // namespace dontfall::face
