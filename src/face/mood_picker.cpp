#include "mood_picker.hpp"

#include <iterator>

namespace dontfall::face {

namespace {

using namespace std::chrono_literals;

// Mood changes take this long; falling asleep is slow.
constexpr auto change = 400ms;
constexpr auto fall_asleep = 3s;
// Link lost: sleepy first, asleep after this.
constexpr auto asleep_after = 10s;
// Surprise when armed, and after a knock.
constexpr auto arm_surprise = 1200ms;
constexpr auto bump_surprise = 1200ms;
constexpr auto poke_squint = 1000ms;
// Disarmed: neutral for 6-15 s, then a passing mood for 2-4 s.
constexpr float neutral_min_s = 6, neutral_max_s = 15;
constexpr float passing_min_s = 2, passing_max_s = 4;
// Not angry or sleepy: those mean a fault or a lost link.
constexpr std::string_view passing_moods[] = {"happy", "sad", "surprised"};

} // namespace

MoodPicker::MoodPicker(Clock::time_point now)
    : rng_(std::random_device {}()), state_since_(now),
      passing_until_(now + seconds(neutral_min_s, neutral_max_s))
{
}

Clock::duration MoodPicker::seconds(float lo, float hi)
{
    return std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<float>(std::uniform_real_distribution<float>(lo, hi)(rng_)));
}

const Mood *MoodPicker::passing(Clock::time_point now)
{
    if (now >= passing_until_) {
        if (passing_) {
            passing_ = nullptr;
            passing_until_ = now + seconds(neutral_min_s, neutral_max_s);
        } else {
            constexpr size_t count = std::size(passing_moods);
            passing_ = &mood_named(passing_moods[rng_() % count]);
            passing_until_ = now + seconds(passing_min_s, passing_max_s);
        }
    }
    return passing_ ? passing_ : &mood_named("neutral");
}

bool MoodPicker::pick(Clock::time_point now, RobotState state, bool bumped, MoodChoice &choice)
{
    if (state != state_) {
        if (state == RobotState::armed)
            surprised_until_ = now + arm_surprise;
        if (state == RobotState::disarmed) {  // passing moods start over
            passing_ = nullptr;
            passing_until_ = now + seconds(neutral_min_s, neutral_max_s);
        }
        state_ = state;
        state_since_ = now;
    }
    const bool calm = state == RobotState::armed || state == RobotState::disarmed;
    if (bumped && calm)
        surprised_until_ = now + bump_surprise;

    const Mood *want = nullptr;
    auto duration = change;
    switch (state) {
    case RobotState::link_lost:
        if (now - state_since_ < asleep_after) {
            want = &mood_named("sleepy");
        } else {
            want = &mood_named("asleep");
            duration = fall_asleep;
        }
        break;
    case RobotState::fault:
        want = &mood_named("angry");
        break;
    case RobotState::armed:
    case RobotState::disarmed:
        if (now < squint_until_)
            want = &mood_named("squint");
        else if (now < surprised_until_)
            want = &mood_named("surprised");
        else if (state == RobotState::armed)
            want = &mood_named("focused");
        else
            want = passing(now);
        break;
    }

    if (want == current_)
        return false;
    current_ = want;
    choice = {want, duration};
    return true;
}

void MoodPicker::poke(Clock::time_point now)
{
    squint_until_ = now + poke_squint;
}

} // namespace dontfall::face
