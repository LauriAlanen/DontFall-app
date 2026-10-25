// The face's ROS 2 node (dontfall_face): listens to the M4's IMU and keeps
// the latest sample for the render loop. The node spins in its own thread;
// the render loop only copies the latest sample under a mutex, so drawing
// never waits on ROS.
#pragma once

#include <mutex>
#include <optional>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>

#include "tween.hpp"

namespace dontfall::face {

// What the face needs from the IMU: the accelerometer, in m/s^2 and the
// IMU's own axes (imu_link). At rest it reads +9.8 along the axis pointing
// up: the force a free mass feels is the opposite of it.
struct ImuSample {
    float ax = 0;
    float ay = 0;
    float az = 0;
    Clock::time_point received {};
};

class ImuListener : public rclcpp::Node {
public:
    ImuListener();

    // The latest sample if it is younger than max_age, else nothing (the
    // M4 or the micro-ROS Agent is gone).
    std::optional<ImuSample> latest(Clock::time_point now, Clock::duration max_age) const;

private:
    void on_imu(const sensor_msgs::msg::Imu &msg);

    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr sub_;
    mutable std::mutex mutex_;
    std::optional<ImuSample> latest_;
};

} // namespace dontfall::face
