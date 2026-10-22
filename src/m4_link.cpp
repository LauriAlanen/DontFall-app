#include "m4_link.hpp"

namespace dontfall {

M4Link::Event M4Link::on_status(Clock::time_point now)
{
    last_status_ = now;
    if (up_)
        return Event::none;
    up_ = true;
    return Event::link_up;
}

M4Link::Event M4Link::poll(Clock::time_point now)
{
    if (!up_ || now - last_status_ < timeout_)
        return Event::none;
    up_ = false;
    return Event::link_lost;
}

} // namespace dontfall
