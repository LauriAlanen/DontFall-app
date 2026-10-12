// dontfall-app: A7 side of the A7 <-> M4 heartbeat.
//
// Sends "PING <seq>" to the M4 every 500 ms over /dev/ttyRPMSG0. The M4
// blinks LD7 while PINGs arrive and answers "PONG <seq>". The link is up
// while PONGs come back within 1.5 s; with --led, that LED (blue LD8 on
// the DK2) blinks while the link is up and is off while it is down.
//
// Single-threaded poll() loop; every timing decision lives in Heartbeat.
// Runs until SIGTERM/SIGINT, and survives the M4 being restarted.
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <poll.h>
#include <sys/utsname.h>

#include "heartbeat.hpp"
#include "protocol.hpp"
#include "rpmsg_tty.hpp"
#include "sysfs_led.hpp"

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

namespace {

using namespace std::chrono_literals;
using dontfall::Heartbeat;
using Clock = Heartbeat::Clock;

constexpr const char *default_device = "/dev/ttyRPMSG0";
constexpr auto ping_period = 500ms;
// Must match HEARTBEAT_TIMEOUT_MS on the M4, so both sides agree on "lost".
constexpr auto link_timeout = 1500ms;
constexpr auto blink_interval = 1000ms;
constexpr auto reopen_interval = 1s;

volatile sig_atomic_t stop_requested;

void on_stop(int)
{
    stop_requested = 1;
}

int usage()
{
    std::fprintf(stderr, "usage: dontfall-app [--led <sysfs-led-dir>] [device]\n");
    return 1;
}

class App {
public:
    App(const char *dev, dontfall::SysfsLed &led)
        : tty_(dev), led_(led), heartbeat_(ping_period, link_timeout) {}

    void run()
    {
        while (!stop_requested) {
            const auto now = Clock::now();
            open_if_due(now);
            send_ping_if_due(now);
            handle(heartbeat_.poll(now));
            blink_if_due(now);
            wait(now);
        }
    }

private:
    void open_if_due(Clock::time_point now)
    {
        if (tty_.is_open() || now < next_open_)
            return;
        if (tty_.open()) {
            std::printf("opened %s\n", tty_.path().c_str());
            splitter_.reset();
            open_error_reported_ = false;
            return;
        }
        // Expected while the M4 is down: say it once, then retry quietly.
        if (!open_error_reported_)
            std::fprintf(stderr, "cannot open %s: %s; retrying every %llds\n",
                         tty_.path().c_str(), std::strerror(errno),
                         static_cast<long long>(reopen_interval.count()));
        open_error_reported_ = true;
        next_open_ = now + reopen_interval;
    }

    void send_ping_if_due(Clock::time_point now)
    {
        if (!tty_.is_open())
            return;
        const auto seq = heartbeat_.ping_due(now);
        if (!seq || tty_.write(dontfall::protocol::ping(*seq)))
            return;
        // EAGAIN: the M4 isn't draining its buffers; the missing PONG
        // takes the link down. Anything else: the channel is gone.
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            close_channel(now, std::strerror(errno));
    }

    void blink_if_due(Clock::time_point now)
    {
        if (!heartbeat_.link_up() || now < next_toggle_)
            return;
        led_on_ = !led_on_;
        led_.set(led_on_);
        next_toggle_ += blink_interval;
        if (next_toggle_ <= now)
            next_toggle_ = now + blink_interval;
    }

    void wait(Clock::time_point now)
    {
        auto deadline = tty_.is_open() ? heartbeat_.next_deadline() : next_open_;
        if (heartbeat_.link_up())
            deadline = std::min(deadline, next_toggle_);
        const auto ms = std::chrono::ceil<std::chrono::milliseconds>(deadline - now).count();
        const int timeout = static_cast<int>(std::clamp<long long>(ms, 0, 1000));

        pollfd pfd {tty_.fd(), POLLIN, 0};
        if (poll(&pfd, tty_.is_open() ? 1 : 0, timeout) <= 0)
            return;  // timeout, or EINTR from a stop signal
        if (pfd.revents & POLLIN)
            receive(Clock::now());
        if (tty_.is_open() && (pfd.revents & (POLLHUP | POLLERR)))
            close_channel(Clock::now(), "hung up");
    }

    void receive(Clock::time_point now)
    {
        char buf[256];
        std::vector<std::string> lines;
        ssize_t n;
        while ((n = tty_.read(buf, sizeof(buf))) > 0)
            splitter_.feed(buf, static_cast<size_t>(n), lines);

        for (const auto &line : lines) {
            if (const auto seq = dontfall::protocol::parse_pong(line))
                handle(heartbeat_.on_pong(*seq, now));
            else
                std::fprintf(stderr, "M4: %s\n", line.c_str());
        }
        if (n < 0)
            close_channel(now, "hung up");
    }

    void close_channel(Clock::time_point now, const char *why)
    {
        std::fprintf(stderr, "%s: %s; reopening\n", tty_.path().c_str(), why);
        tty_.close();
        handle(heartbeat_.channel_closed());
        next_open_ = now + reopen_interval;
    }

    void handle(Heartbeat::Event event)
    {
        switch (event) {
        case Heartbeat::Event::link_up:
            std::printf("M4 link up\n");
            next_toggle_ = Clock::now();
            break;
        case Heartbeat::Event::link_lost:
            std::printf("M4 link lost\n");
            led_on_ = false;
            led_.set(false);
            break;
        case Heartbeat::Event::none:
            break;
        }
    }

    dontfall::RpmsgTty tty_;
    dontfall::SysfsLed &led_;
    Heartbeat heartbeat_;
    dontfall::protocol::LineSplitter splitter_;
    Clock::time_point next_open_ {};
    Clock::time_point next_toggle_ {};
    bool led_on_ = false;
    bool open_error_reported_ = false;
};

} // namespace

int main(int argc, char **argv)
{
    // stdout goes to journald under systemd: flush per line, not per 4 KiB.
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    utsname sys {};
    uname(&sys);
    std::printf("dontfall-app %s on %s %s (%s)\n",
                APP_VERSION, sys.sysname, sys.release, sys.machine);

    const char *dev = default_device;
    std::string led_dir;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--led") {
            if (++i >= argc)
                return usage();
            led_dir = argv[i];
        } else if (arg[0] == '-' || dev != default_device) {
            return usage();
        } else {
            dev = argv[i];
        }
    }

    // No SA_RESTART: a stop signal must interrupt poll() right away.
    struct sigaction sa {};
    sa.sa_handler = on_stop;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    // A missing LED is a misconfiguration: fail so systemd reports it.
    dontfall::SysfsLed led;
    std::string failed_path;
    if (!led_dir.empty() && !led.take_over(led_dir, failed_path)) {
        std::fprintf(stderr, "cannot use %s: %s\n", failed_path.c_str(), std::strerror(errno));
        return 1;
    }

    App(dev, led).run();
    return 0;  // ~SysfsLed restores the LED's trigger
}
