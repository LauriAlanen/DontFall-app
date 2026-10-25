// The robot's state as the face shows it, from what the M4 reports. Plain
// logic, no ROS: face_node.cpp fills M4Status from /diagnostics.
#pragma once

#include <chrono>
#include <string>

#include "tween.hpp"

namespace dontfall::face {

// The M4's statuses on /diagnostics (DontFall-M4 diag.c, 1 Hz).
struct M4Status {
    bool armed = false;  // supervisor: "armed"
    bool fault = false;  // IMU error, or a safety disarm (supervisor WARN)
    std::string fault_text;  // e.g. "imu range", for the log
    Clock::time_point received {};
};

enum class RobotState {
    disarmed,   // resting, the normal case
    armed,
    fault,
    link_lost,  // no news from the M4: it, the micro-ROS Agent or the link is down
};

inline const char *to_string(RobotState state)
{
    switch (state) {
    case RobotState::disarmed:  return "disarmed";
    case RobotState::armed:     return "armed";
    case RobotState::fault:     return "fault";
    case RobotState::link_lost: return "link lost";
    }
    return "?";
}

// Three 1 Hz statuses missed in a row = link lost (like dontfall-app).
inline constexpr auto m4_timeout = std::chrono::seconds(3);

// imu_seen / status: the last IMU sample and the last M4 status, if any.
inline RobotState robot_state(Clock::time_point now, const Clock::time_point *imu_seen,
                              const M4Status *status)
{
    if (!imu_seen || now - *imu_seen > m4_timeout || !status ||
        now - status->received > m4_timeout)
        return RobotState::link_lost;
    if (status->fault)
        return RobotState::fault;
    return status->armed ? RobotState::armed : RobotState::disarmed;
}

} // namespace dontfall::face
