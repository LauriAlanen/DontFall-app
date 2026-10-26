// Picks the mood the face heads for, from the robot's state:
//
//   link lost  sleepy, after a while asleep (eyes slowly closing)
//   fault      angry (red)
//   armed      surprised for a moment, then focused
//   disarmed   neutral, now and then a passing mood (happy, sad, surprised)
//
// A hard knock of the googly eyes against their edge looks surprised for a
// moment while armed or disarmed, a poke on an eye (touch) squints.
//
// The camera (the PC's vision node), while armed or disarmed: someone
// appearing gets a happy look (surprised if nobody had come for a long
// time), and disarmed with nobody around for a long time the face gets
// sleepy. Only while the camera is watching: without the PC the moods
// are as above. The robot's own state always wins.
#pragma once

#include <random>

#include "mood.hpp"
#include "robot_state.hpp"
#include "tween.hpp"

namespace dontfall::face {

// What the camera says.
struct Company {
    bool watching = false;  // the vision node is running
    bool someone = false;   // and sees a face
};

struct MoodChoice {
    const Mood *mood = nullptr;
    Clock::duration change {};  // how long the tween to it takes
};

class MoodPicker {
public:
    explicit MoodPicker(Clock::time_point now);

    // True and the new choice when the mood to head for changes.
    bool pick(Clock::time_point now, RobotState state, bool bumped, const Company &company,
              MoodChoice &choice);

    // An eye was poked (touch): squint for a moment while armed or disarmed.
    void poke(Clock::time_point now);

private:
    const Mood *passing(Clock::time_point now);
    void notice(Clock::time_point now, const Company &company);
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
    // The camera: whether someone is seen, since when nobody has been (or
    // since the camera started watching), and the greeting for who came.
    bool someone_ = false;
    Clock::time_point alone_since_;
    const Mood *greeting_ = nullptr;
    Clock::time_point greeting_until_ {};
    bool lonely_ = false;
};

} // namespace dontfall::face
