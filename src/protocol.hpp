// A7 <-> M4 line protocol on the rpmsg-tty channel. Mirror of
// DontFall-M4/app/src/protocol.h, which is the reference:
//
//   A7 -> M4      M4 -> A7
//   PING <seq>    PONG <seq>     <seq> = 1-10 decimal digits
//                 ERR <reason>   bad or unknown line
//
// ASCII lines ending in '\n'. Interim format until micro-ROS.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dontfall::protocol {

// Longest line either side accepts, without the '\n'.
constexpr std::size_t max_line = 31;

std::string ping(std::uint32_t seq);

// The seq of a "PONG <seq>" line (no '\n'), or nothing for any other line.
std::optional<std::uint32_t> parse_pong(std::string_view line);

// Reassembles '\n'-terminated lines from a byte stream; a trailing '\r'
// is dropped, empty lines are skipped, and lines longer than max_line
// are discarded whole.
class LineSplitter {
public:
    void feed(const char *data, std::size_t len, std::vector<std::string> &lines);
    void reset();

private:
    std::string partial_;
    bool overflow_ = false;
};

} // namespace dontfall::protocol
