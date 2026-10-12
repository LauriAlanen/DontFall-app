// The rpmsg-tty channel to the M4 (/dev/ttyRPMSG0): raw, non-blocking.
//
// The device node exists only while the M4 firmware runs. When remoteproc
// stops or restarts the M4, the kernel hangs the tty up; read() then
// reports EOF/EIO and the owner closes and reopens the channel.
#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <sys/types.h>

namespace dontfall {

class RpmsgTty {
public:
    explicit RpmsgTty(std::string path) : path_(std::move(path)) {}
    ~RpmsgTty() { close(); }
    RpmsgTty(const RpmsgTty &) = delete;
    RpmsgTty &operator=(const RpmsgTty &) = delete;

    // false with errno set if the device can't be opened.
    bool open();
    void close();
    bool is_open() const { return fd_ >= 0; }
    int fd() const { return fd_; }
    const std::string &path() const { return path_; }

    // Whole message or nothing; false on error (EAGAIN = M4 not draining).
    bool write(const std::string &msg);

    // >0 bytes read, 0 nothing available, -1 hung up or error.
    ssize_t read(char *buf, std::size_t len);

private:
    std::string path_;
    int fd_ = -1;
};

} // namespace dontfall
