// The face's ROS 2 node (dontfall_face): listens to the M4's IMU and its
// statuses, and to the faces the PC's vision node sees, and keeps the
// latest of each for the render loop. The node spins
// in its own thread; the render loop only copies the latest values under a
// mutex, so drawing never waits on ROS. It only listens, never commands.
#pragma once

#include <mutex>
#include <optional>

#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <vision_msgs/msg/detection2_d_array.hpp>

#include "robot_state.hpp"
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

// The biggest face in the camera's image (/vision/faces, from the PC):
// -1..1 from the middle, + = right and down, as the camera sees it.
struct SeenFace {
    float x = 0;
    float y = 0;
    Clock::time_point received {};
};

class FaceNode : public rclcpp::Node {
public:
    FaceNode();

    std::optional<ImuSample> imu() const;
    std::optional<M4Status> m4_status() const;
    // Updated only while a face is seen: its age says whether it still is.
    std::optional<SeenFace> face() const;

private:
    void on_imu(const sensor_msgs::msg::Imu &msg);
    void on_diagnostics(const diagnostic_msgs::msg::DiagnosticArray &msg);
    void on_faces(const vision_msgs::msg::Detection2DArray &msg);

    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_;
    rclcpp::Subscription<diagnostic_msgs::msg::DiagnosticArray>::SharedPtr diag_sub_;
    rclcpp::Subscription<vision_msgs::msg::Detection2DArray>::SharedPtr faces_sub_;
    // The camera image's size in pixels; detections are in pixels of it.
    double camera_width_;
    double camera_height_;
    mutable std::mutex mutex_;
    std::optional<ImuSample> imu_;
    std::optional<M4Status> m4_status_;
    std::optional<SeenFace> face_;
};

} // namespace dontfall::face
