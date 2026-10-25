#include "idle.hpp"

#include <algorithm>

namespace dontfall::face {

namespace {

using namespace std::chrono_literals;

// One blink: lids close fast, stay shut a moment, open a bit slower.
constexpr auto blink_close = 60ms;
constexpr auto blink_shut = 40ms;
constexpr auto blink_open = 100ms;
// A double blink starts its second blink this long after the first.
constexpr auto double_blink_gap = 260ms;
constexpr float double_blink_chance = 0.2f;

// Glances: a saccade is a quick jump, real eyes do it in tens of ms.
constexpr auto saccade = 90ms;
constexpr float back_to_middle_chance = 0.4f;

float ms(Clock::duration d)
{
    return std::chrono::duration<float, std::milli>(d).count();
}

// 0 = open .. 1 = shut, t after the blink started
float blink_shape(Clock::duration t)
{
    if (t < 0ms)
        return 0;
    if (t < blink_close)
        return ms(t) / ms(blink_close);
    t -= blink_close;
    if (t < blink_shut)
        return 1;
    t -= blink_shut;
    if (t < blink_open)
        return 1 - ms(t) / ms(blink_open);
    return 0;
}

} // namespace

Idle::Idle(Clock::time_point now)
    : rng_(std::random_device {}()), gaze_(Gaze {}), next_glance_(now + 1s)
{
    next_blink_ = now + seconds(1, 3);
}

Clock::duration Idle::seconds(float lo, float hi)
{
    return std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<float>(uniform(lo, hi)));
}

float Idle::blink_amount(Clock::time_point now) const
{
    float amount = blink_shape(now - blink_start_);
    if (double_blink_)
        amount = std::max(amount, blink_shape(now - blink_start_ - double_blink_gap));
    return amount;
}

Look Idle::at(Clock::time_point now)
{
    if (now >= next_blink_) {
        blink_start_ = now;
        double_blink_ = uniform(0, 1) < double_blink_chance;
        next_blink_ = now + seconds(2, 6);
    }
    if (now >= next_glance_) {
        Gaze target;  // the middle
        if (uniform(0, 1) >= back_to_middle_chance)
            target = {uniform(-1, 1), uniform(-0.6f, 0.6f)};
        gaze_.go(target, saccade, now);
        next_glance_ = now + seconds(1, 4);
    }
    return {gaze_.at(now), blink_amount(now)};
}

} // namespace dontfall::face
