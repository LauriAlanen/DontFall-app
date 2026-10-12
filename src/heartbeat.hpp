// A7 side of the A7 <-> M4 heartbeat: decides when the next PING is due
// and whether the link is up (PONGs keep coming back within the timeout).
//
// No I/O and no clock reads: the caller passes `now`, so the logic is
// deterministic and independent of the event loop.
#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

namespace dontfall {

class Heartbeat {
public:
    using Clock = std::chrono::steady_clock;
    enum class Event { none, link_up, link_lost };

    Heartbeat(Clock::duration period, Clock::duration timeout);

    // Seq for the PING to send now, or nothing if none is due yet.
    std::optional<std::uint32_t> ping_due(Clock::time_point now);

    // A PONG arrived. Only answers to the PINGs of the last timeout window
    // count, so a stale reply can't keep a dead link "up".
    Event on_pong(std::uint32_t seq, Clock::time_point now);

    // Check the timeout; call after every wakeup.
    Event poll(Clock::time_point now);

    // The channel went away: the link is down, and the next PING goes out
    // as soon as it is back.
    Event channel_closed();

    bool link_up() const { return up_; }

    // Earliest time something can change: next PING or link timeout.
    Clock::time_point next_deadline() const;

private:
    Clock::duration period_;
    Clock::duration timeout_;
    std::uint32_t window_;  // PINGs sent per timeout
    std::uint32_t seq_ = 0; // last PING sent; 0 = none yet
    Clock::time_point next_ping_ {};
    Clock::time_point last_pong_ {};
    bool up_ = false;
};

} // namespace dontfall
