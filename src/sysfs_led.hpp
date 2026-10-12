// A Linux LED-class LED (/sys/class/leds/<name>/{trigger,brightness}),
// taken over from its kernel trigger for as long as this object lives.
// On destruction the LED is switched off and the original trigger
// (e.g. "heartbeat") is restored.
#pragma once

#include <string>

namespace dontfall {

class SysfsLed {
public:
    SysfsLed() = default;
    ~SysfsLed() { release(); }
    SysfsLed(const SysfsLed &) = delete;
    SysfsLed &operator=(const SysfsLed &) = delete;

    // Save the current trigger and switch to manual control ("none").
    // false with errno set on failure; `failed_path` names the file.
    bool take_over(const std::string &led_dir, std::string &failed_path);
    void release();
    bool owned() const { return !dir_.empty(); }

    void set(bool on);

private:
    std::string dir_;
    std::string saved_trigger_;
};

} // namespace dontfall
