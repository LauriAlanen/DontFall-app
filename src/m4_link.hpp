// Whether the M4 is reachable over ROS 2: the link is up while its
// /diagnostics statuses keep arriving within the timeout. That proves the
// whole chain: M4 firmware running, rpmsg, the micro-ROS Agent and DDS.
//
// No I/O and no clock reads: the caller passes `now`, so the logic is
// deterministic and independent of ROS callbacks.
#pragma once

#include <chrono>

namespace dontfall {

class M4Link {
public:
    using Clock = std::chrono::steady_clock;
    enum class Event { none, link_up, link_lost };

    explicit M4Link(Clock::duration timeout) : timeout_(timeout) {}

    // A status from the M4 arrived.
    Event on_status(Clock::time_point now);

    // Check the timeout; call regularly.
    Event poll(Clock::time_point now);

    bool up() const { return up_; }

private:
    Clock::duration timeout_;
    Clock::time_point last_status_ {};
    bool up_ = false;
};

} // namespace dontfall
