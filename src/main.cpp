// dontfall-app: the A7's ROS 2 node (dontfall_app).
//
// For now it watches the M4: the link is up while the M4's statuses arrive
// on /diagnostics (DontFall-M4 diag.c, 1 Hz). With --led, that LED (blue
// LD8 on the DK2) blinks while the link is up and is off while it is down.
//
// Runs until SIGTERM/SIGINT (rclcpp handles both).
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <rclcpp/rclcpp.hpp>

#include "m4_link.hpp"
#include "sysfs_led.hpp"

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

namespace {

using namespace std::chrono_literals;
using diagnostic_msgs::msg::DiagnosticArray;
using dontfall::M4Link;
using Clock = M4Link::Clock;

// The M4 names its statuses "dontfall_m4: <part>".
constexpr std::string_view m4_prefix = "dontfall_m4:";
// Three 1 Hz statuses missed in a row = link lost.
constexpr auto link_timeout = 3s;
constexpr auto blink_interval = 1000ms;
constexpr auto tick_period = 100ms;

int usage()
{
    std::fprintf(stderr, "usage: dontfall-app [--led <sysfs-led-dir>]\n");
    return 1;
}

class AppNode : public rclcpp::Node {
public:
    explicit AppNode(dontfall::SysfsLed &led)
        : Node("dontfall_app"), led_(led), link_(link_timeout)
    {
        // Default QoS (reliable), what the M4 publishes /diagnostics with.
        sub_ = create_subscription<DiagnosticArray>(
            "/diagnostics", 10, [this](const DiagnosticArray &msg) { on_diagnostics(msg); });
        timer_ = create_wall_timer(tick_period, [this] { tick(); });
    }

private:
    void on_diagnostics(const DiagnosticArray &msg)
    {
        for (const auto &status : msg.status) {
            if (std::string_view(status.name).substr(0, m4_prefix.size()) == m4_prefix) {
                handle(link_.on_status(Clock::now()));
                return;
            }
        }
    }

    void tick()
    {
        const auto now = Clock::now();
        handle(link_.poll(now));
        if (!link_.up() || now < next_toggle_)
            return;
        led_on_ = !led_on_;
        led_.set(led_on_);
        next_toggle_ += blink_interval;
        if (next_toggle_ <= now)
            next_toggle_ = now + blink_interval;
    }

    void handle(M4Link::Event event)
    {
        switch (event) {
        case M4Link::Event::link_up:
            RCLCPP_INFO(get_logger(), "M4 link up");
            next_toggle_ = Clock::now();
            break;
        case M4Link::Event::link_lost:
            RCLCPP_INFO(get_logger(), "M4 link lost");
            led_on_ = false;
            led_.set(false);
            break;
        case M4Link::Event::none:
            break;
        }
    }

    dontfall::SysfsLed &led_;
    M4Link link_;
    rclcpp::Subscription<DiagnosticArray>::SharedPtr sub_;
    rclcpp::TimerBase::SharedPtr timer_;
    Clock::time_point next_toggle_ {};
    bool led_on_ = false;
};

} // namespace

int main(int argc, char **argv)
{
    // stdout goes to journald under systemd: flush per line, not per 4 KiB.
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    std::printf("dontfall-app %s\n", APP_VERSION);

    rclcpp::init(argc, argv);
    // our own arguments, without the --ros-args part
    const std::vector<std::string> args = rclcpp::remove_ros_arguments(argc, argv);
    std::string led_dir;
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--led" && i + 1 < args.size())
            led_dir = args[++i];
        else
            return usage();
    }

    // A missing LED is a misconfiguration: fail so systemd reports it.
    dontfall::SysfsLed led;
    std::string failed_path;
    if (!led_dir.empty() && !led.take_over(led_dir, failed_path)) {
        std::fprintf(stderr, "cannot use %s: %s\n", failed_path.c_str(), std::strerror(errno));
        return 1;
    }

    rclcpp::spin(std::make_shared<AppNode>(led));
    rclcpp::shutdown();
    return 0;  // ~SysfsLed restores the LED's trigger
}
