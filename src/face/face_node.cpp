#include "face_node.hpp"

#include <string>
#include <string_view>

namespace dontfall::face {

namespace {

using diagnostic_msgs::msg::DiagnosticStatus;

// The M4's status names (DontFall-M4 diag.c).
constexpr std::string_view imu_status = "dontfall_m4: imu";
constexpr std::string_view supervisor_status = "dontfall_m4: supervisor";

} // namespace

FaceNode::FaceNode() : Node("dontfall_face")
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

} // namespace dontfall::face
