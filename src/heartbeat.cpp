#include "heartbeat.hpp"

#include <algorithm>

namespace dontfall {

Heartbeat::Heartbeat(Clock::duration period, Clock::duration timeout)
    : period_(period),
      timeout_(timeout),
      window_(static_cast<std::uint32_t>(
          std::max<Clock::rep>(1, (timeout + period - Clock::duration(1)) / period)))
{
}

std::optional<std::uint32_t> Heartbeat::ping_due(Clock::time_point now)
{
    if (now < next_ping_)
        return std::nullopt;
    // Fixed rate, but no burst of catch-up PINGs after a stall.
    next_ping_ += period_;
    if (next_ping_ <= now)
        next_ping_ = now + period_;
    if (++seq_ == 0)  // keep 0 meaning "none sent"
        seq_ = 1;
    return seq_;
}

Heartbeat::Event Heartbeat::on_pong(std::uint32_t seq, Clock::time_point now)
{
    // Unsigned arithmetic, so this also holds across seq wraparound.
    if (seq_ == 0 || seq == 0 || seq_ - seq >= window_)
        return Event::none;
    last_pong_ = now;
    if (up_)
        return Event::none;
    up_ = true;
    return Event::link_up;
}

Heartbeat::Event Heartbeat::poll(Clock::time_point now)
{
    if (!up_ || now - last_pong_ < timeout_)
        return Event::none;
    up_ = false;
    return Event::link_lost;
}

Heartbeat::Event Heartbeat::channel_closed()
{
    next_ping_ = {};
    if (!up_)
        return Event::none;
    up_ = false;
    return Event::link_lost;
}

Heartbeat::Clock::time_point Heartbeat::next_deadline() const
{
    return up_ ? std::min(next_ping_, last_pong_ + timeout_) : next_ping_;
}

} // namespace dontfall
