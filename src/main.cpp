#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#include <fcntl.h>
#include <poll.h>
#include <sys/utsname.h>
#include <termios.h>
#include <unistd.h>

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

namespace {

constexpr const char *default_device = "/dev/ttyRPMSG0";
constexpr int reply_timeout_ms = 2000;
constexpr int blink_interval_ms = 1000;

volatile sig_atomic_t stop_requested;

void on_stop(int)
{
    stop_requested = 1;
}

// Read one line (up to and including '\n') or until the deadline passes.
std::string read_line(int fd, int timeout_ms)
{
    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + std::chrono::milliseconds(timeout_ms);
    std::string line;

    while (line.empty() || line.back() != '\n') {
        const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - clock::now()).count();
        if (left <= 0)
            break;

        pollfd pfd {fd, POLLIN, 0};
        int ready;
        do {
            ready = poll(&pfd, 1, static_cast<int>(left));
        } while (ready < 0 && errno == EINTR);
        if (ready <= 0)
            break;

        char buf[256];
        ssize_t n;
        do {
            n = read(fd, buf, sizeof(buf));
        } while (n < 0 && errno == EINTR);
        if (n <= 0)
            break;
        line.append(buf, static_cast<size_t>(n));
    }
    return line;
}

bool read_file(const std::string &path, std::string &out)
{
    const int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return false;
    char buf[1024];
    ssize_t n;
    do {
        n = read(fd, buf, sizeof(buf));
    } while (n < 0 && errno == EINTR);
    const int saved = errno;
    close(fd);
    if (n < 0) {
        errno = saved;
        return false;
    }
    out.assign(buf, static_cast<size_t>(n));
    return true;
}

bool write_file(const std::string &path, const char *value)
{
    const int fd = open(path.c_str(), O_WRONLY | O_TRUNC);
    if (fd < 0)
        return false;
    const size_t len = std::strlen(value);
    ssize_t n;
    do {
        n = write(fd, value, len);
    } while (n < 0 && errno == EINTR);
    const int saved = errno;
    close(fd);
    if (n != static_cast<ssize_t>(len)) {
        errno = saved;
        return false;
    }
    return true;
}

// A sysfs trigger file lists every trigger with the active one in
// brackets, e.g. "none [heartbeat] timer".
std::string current_trigger(const std::string &content)
{
    const auto open = content.find('[');
    const auto close = open == std::string::npos
        ? std::string::npos : content.find(']', open);
    if (close != std::string::npos)
        return content.substr(open + 1, close - open - 1);
    const auto first = content.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    return content.substr(first, content.find_last_not_of(" \t\r\n") - first + 1);
}

int say_hello(const char *dev)
{
    const int fd = open(dev, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        std::fprintf(stderr, "cannot open %s: %s\n", dev, std::strerror(errno));
        return 1;
    }

    // Raw mode: no echo (the M4 would get its own reply back) and no
    // newline translation.
    termios tio {};
    if (tcgetattr(fd, &tio) == 0) {
        cfmakeraw(&tio);
        tcsetattr(fd, TCSANOW, &tio);
    }

    const char hello[] = "hello from A7\n";
    if (write(fd, hello, sizeof(hello) - 1) != static_cast<ssize_t>(sizeof(hello) - 1)) {
        std::fprintf(stderr, "write to %s failed: %s\n", dev, std::strerror(errno));
        close(fd);
        return 1;
    }

    std::string reply = read_line(fd, reply_timeout_ms);
    close(fd);
    if (reply.empty()) {
        std::fprintf(stderr, "no reply from M4 within %ds\n", reply_timeout_ms / 1000);
        return 2;
    }

    if (reply.back() != '\n')
        reply.push_back('\n');
    std::fputs(reply.c_str(), stdout);
    return 0;
}

// Take over a Linux LED-class LED (led_dir/trigger + led_dir/brightness),
// say hello to the M4 once, then toggle brightness every blink_interval_ms
// until SIGTERM/SIGINT. On exit the LED is off and its original trigger
// is restored.
int blink(const std::string &led_dir, const char *dev)
{
    const std::string trigger_path = led_dir + "/trigger";
    const std::string brightness_path = led_dir + "/brightness";

    std::string trigger_content;
    if (!read_file(trigger_path, trigger_content)) {
        std::fprintf(stderr, "cannot read %s: %s\n",
                     trigger_path.c_str(), std::strerror(errno));
        return 1;
    }
    const std::string saved_trigger = current_trigger(trigger_content);

    // Handlers installed before the LED is taken over, so a stop during
    // the hello or the first tick still restores it.
    struct sigaction sa {};
    sa.sa_handler = on_stop;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    if (!write_file(trigger_path, "none")) {
        std::fprintf(stderr, "cannot write %s: %s\n",
                     trigger_path.c_str(), std::strerror(errno));
        return 1;
    }

    // A failed hello doesn't stop the blink.
    say_hello(dev);
    std::fflush(stdout);  // stdout is fully buffered under systemd

    bool led_on = false;
    timespec next {};
    clock_gettime(CLOCK_MONOTONIC, &next);
    while (!stop_requested) {
        led_on = !led_on;
        write_file(brightness_path, led_on ? "1" : "0");
        next.tv_nsec += blink_interval_ms * 1000000L;
        next.tv_sec += next.tv_nsec / 1000000000L;
        next.tv_nsec %= 1000000000L;
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr) == EINTR
               && !stop_requested) {
        }
    }

    write_file(brightness_path, "0");
    write_file(trigger_path, saved_trigger.c_str());
    return 0;
}

} // namespace

int main(int argc, char **argv)
{
    utsname sys {};
    uname(&sys);
    std::printf("dontfall-app %s on %s %s (%s)\n",
                APP_VERSION, sys.sysname, sys.release, sys.machine);
    std::fflush(stdout);

    const char *dev = default_device;
    std::string led_dir;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--led") {
            if (++i >= argc) {
                std::fprintf(stderr, "--led needs a directory\n");
                return 1;
            }
            led_dir = argv[i];
        } else if (arg[0] == '-') {
            std::fprintf(stderr, "usage: dontfall-app [--led <led-dir>] [device]\n");
            return 1;
        } else if (dev == default_device) {
            dev = argv[i];
        } else {
            std::fprintf(stderr, "usage: dontfall-app [--led <led-dir>] [device]\n");
            return 1;
        }
    }

    if (!led_dir.empty())
        return blink(led_dir, dev);
    return say_hello(dev);
}
