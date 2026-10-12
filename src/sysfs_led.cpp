#include "sysfs_led.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace dontfall {

namespace {

bool read_file(const std::string &path, std::string &out)
{
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
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
    const int fd = open(path.c_str(), O_WRONLY | O_TRUNC | O_CLOEXEC);
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

} // namespace

bool SysfsLed::take_over(const std::string &led_dir, std::string &failed_path)
{
    release();
    const std::string trigger = led_dir + "/trigger";
    std::string content;
    if (!read_file(trigger, content)) {
        failed_path = trigger;
        return false;
    }
    if (!write_file(trigger, "none")) {
        failed_path = trigger;
        return false;
    }
    saved_trigger_ = current_trigger(content);
    dir_ = led_dir;
    set(false);
    return true;
}

void SysfsLed::release()
{
    if (!owned())
        return;
    set(false);
    write_file(dir_ + "/trigger", saved_trigger_.c_str());
    dir_.clear();
}

void SysfsLed::set(bool on)
{
    if (owned())
        write_file(dir_ + "/brightness", on ? "1" : "0");
}

} // namespace dontfall
