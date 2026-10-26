#include "face_node.hpp"

#include <chrono>
#include <string>
#include <string_view>

namespace dontfall::face {

namespace {

using namespace std::chrono_literals;
using diagnostic_msgs::msg::DiagnosticStatus;

// The M4's status names (DontFall-M4 diag.c).
constexpr std::string_view imu_status = "dontfall_m4: imu";
constexpr std::string_view supervisor_status = "dontfall_m4: supervisor";

// Faces older than this when they arrive are ignored: the PC or the WiFi
// lagged, and looking there would be looking at the past. Detections carry
// the frame's capture time from the board's own clock (the camera is
// here), so the age is exact.
constexpr auto face_max_age = 500ms;

} // namespace

FaceNode::FaceNode()
    : Node("dontfall_face"),
      camera_width_(declare_parameter("camera_width", 640)),
      camera_height_(declare_parameter("camera_height", 480))
{
    // Best effort, like the M4 publishes it (50 Hz): a lost sample doesn't
    // matter, the next one is 20 ms away.
    imu_sub_ = create_subscription<sensor_msgs::msg::Imu>(
        "/dontfall_m4/imu", rclcpp::SensorDataQoS(),
        [this](const sensor_msgs::msg::Imu &msg) { on_imu(msg); });
    // Default QoS (reliable), what the M4 publishes /diagnostics with.
    diag_sub_ = create_subscription<diagnostic_msgs::msg::DiagnosticArray>(
        "/diagnostics", 10,
        [this](const diagnostic_msgs::msg::DiagnosticArray &msg) { on_diagnostics(msg); });
    // Reliable, like the vision node publishes it; only the newest matters.
    faces_sub_ = create_subscription<vision_msgs::msg::Detection2DArray>(
        "/vision/faces", 1,
        [this](const vision_msgs::msg::Detection2DArray &msg) { on_faces(msg); });
}

void FaceNode::on_imu(const sensor_msgs::msg::Imu &msg)
{
    const ImuSample sample {static_cast<float>(msg.linear_acceleration.x),
                            static_cast<float>(msg.linear_acceleration.y),
                            static_cast<float>(msg.linear_acceleration.z), Clock::now()};
    std::lock_guard lock(mutex_);
    imu_ = sample;
}

void FaceNode::on_diagnostics(const diagnostic_msgs::msg::DiagnosticArray &msg)
{
    // Other nodes may publish on /diagnostics too: only the M4's message,
    // the one with its supervisor status, counts.
    M4Status status;
    bool from_m4 = false;
    auto note_fault = [&status](std::string_view part, const DiagnosticStatus &s) {
        if (s.level == DiagnosticStatus::OK)
            return;
        status.fault = true;
        if (!status.fault_text.empty())
            status.fault_text += ", ";
        status.fault_text += std::string(part) + " " + s.message;
    };
    for (const auto &s : msg.status) {
        if (s.name == supervisor_status) {
            from_m4 = true;
            status.armed = s.message == "armed";
            note_fault("supervisor", s);
        } else if (s.name == imu_status) {
            note_fault("imu", s);
        }
    }
    if (!from_m4)
        return;
    status.received = Clock::now();
    std::lock_guard lock(mutex_);
    m4_status_ = status;
}

void FaceNode::on_faces(const vision_msgs::msg::Detection2DArray &msg)
{
    if (now() - rclcpp::Time(msg.header.stamp) > rclcpp::Duration(face_max_age))
        return;
    {
        std::lock_guard lock(mutex_);
        faces_received_ = Clock::now();
    }
    const vision_msgs::msg::Detection2D *biggest = nullptr;
    for (const auto &d : msg.detections)
        if (!biggest || d.bbox.size_x * d.bbox.size_y > biggest->bbox.size_x * biggest->bbox.size_y)
            biggest = &d;
    if (!biggest)
        return;
    const SeenFace face {
        static_cast<float>(biggest->bbox.center.position.x / camera_width_ * 2 - 1),
        static_cast<float>(biggest->bbox.center.position.y / camera_height_ * 2 - 1),
        Clock::now()};
    std::lock_guard lock(mutex_);
    face_ = face;
}

std::optional<ImuSample> FaceNode::imu() const
{
    std::lock_guard lock(mutex_);
    return imu_;
}

std::optional<M4Status> FaceNode::m4_status() const
{
    std::lock_guard lock(mutex_);
    return m4_status_;
}

std::optional<SeenFace> FaceNode::face() const
{
    std::lock_guard lock(mutex_);
    return face_;
}

std::optional<Clock::time_point> FaceNode::faces_received() const
{
    std::lock_guard lock(mutex_);
    return faces_received_;
}

} // namespace dontfall::face
