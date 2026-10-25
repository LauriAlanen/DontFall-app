#include "imu_listener.hpp"

namespace dontfall::face {

ImuListener::ImuListener() : Node("dontfall_face")
{
    // Best effort, like the M4 publishes it (50 Hz): a lost sample doesn't
    // matter, the next one is 20 ms away.
    sub_ = create_subscription<sensor_msgs::msg::Imu>(
        "/dontfall_m4/imu", rclcpp::SensorDataQoS(),
        [this](const sensor_msgs::msg::Imu &msg) { on_imu(msg); });
}

void ImuListener::on_imu(const sensor_msgs::msg::Imu &msg)
{
    const ImuSample sample {static_cast<float>(msg.linear_acceleration.x),
                            static_cast<float>(msg.linear_acceleration.y),
                            static_cast<float>(msg.linear_acceleration.z), Clock::now()};
    std::lock_guard lock(mutex_);
    latest_ = sample;
}

std::optional<ImuSample> ImuListener::latest(Clock::time_point now,
                                             Clock::duration max_age) const
{
    std::lock_guard lock(mutex_);
    if (!latest_ || now - latest_->received > max_age)
        return std::nullopt;
    return latest_;
}

} // namespace dontfall::face
