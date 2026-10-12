#include "rpmsg_tty.hpp"

#include <cerrno>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace dontfall {

bool RpmsgTty::open()
{
    close();
    // Non-blocking: a stuck M4 must not freeze the event loop on write().
    fd_ = ::open(path_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd_ < 0)
        return false;

    // Raw mode: no echo (the M4 would get its own replies back) and no
    // newline translation.
    termios tio {};
    if (tcgetattr(fd_, &tio) == 0) {
        cfmakeraw(&tio);
        tcsetattr(fd_, TCSANOW, &tio);
    }
    return true;
}

void RpmsgTty::close()
{
    if (fd_ >= 0)
        ::close(fd_);
    fd_ = -1;
}

bool RpmsgTty::write(const std::string &msg)
{
    ssize_t n;
    do {
        n = ::write(fd_, msg.data(), msg.size());
    } while (n < 0 && errno == EINTR);
    return n == static_cast<ssize_t>(msg.size());
}

ssize_t RpmsgTty::read(char *buf, std::size_t len)
{
    ssize_t n;
    do {
        n = ::read(fd_, buf, len);
    } while (n < 0 && errno == EINTR);
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return 0;
    return n > 0 ? n : -1;
}

} // namespace dontfall
