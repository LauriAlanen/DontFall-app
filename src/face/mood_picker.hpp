// Picks the mood the face heads for, from the robot's state:
//
//   link lost  sleepy, after a while asleep (eyes slowly closing)
//   fault      angry (red)
//   armed      surprised for a moment, then focused
//   disarmed   neutral, now and then a passing mood (happy, sad, surprised)
//
// A hard knock of the googly eyes against their edge looks surprised for a
// moment while armed or disarmed, a poke on an eye (touch) squints.
#pragma once

#include <random>

#include "mood.hpp"
#include "robot_state.hpp"
#include "tween.hpp"

namespace dontfall::face {

struct MoodChoice {
    const Mood *mood = nullptr;
    Clock::duration change {};  // how long the tween to it takes
};

class MoodPicker {
public:
    explicit MoodPicker(Clock::time_point now);

    // True and the new choice when the mood to head for changes.
    bool pick(Clock::time_point now, RobotState state, bool bumped, MoodChoice &choice);

    // An eye was poked (touch): squint for a moment while armed or disarmed.
    void poke(Clock::time_point now);

private:
    const Mood *passing(Clock::time_point now);
    Clock::duration seconds(float lo, float hi);

    std::minstd_rand rng_;
    // Starts as link lost: the face wakes up when the M4's data arrives.
    RobotState state_ = RobotState::link_lost;
    Clock::time_point state_since_;
    Clock::time_point surprised_until_ {};
    Clock::time_point squint_until_ {};
    const Mood *passing_ = nullptr;  // nullptr = neutral
    Clock::time_point passing_until_;
    const Mood *current_ = nullptr;
};

} // namespace dontfall::face
