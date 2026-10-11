#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
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

} // namespace

int main(int argc, char **argv)
{
    utsname sys {};
    uname(&sys);
    std::printf("dontfall-app %s on %s %s (%s)\n",
                APP_VERSION, sys.sysname, sys.release, sys.machine);
    std::fflush(stdout);

    const char *dev = argc > 1 ? argv[1] : default_device;
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
