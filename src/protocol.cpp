#include "protocol.hpp"

#include <charconv>

namespace dontfall::protocol {

namespace {

constexpr std::string_view pong_prefix = "PONG ";
constexpr std::size_t max_seq_digits = 10;

} // namespace

std::string ping(std::uint32_t seq)
{
    return "PING " + std::to_string(seq) + "\n";
}

std::optional<std::uint32_t> parse_pong(std::string_view line)
{
    if (line.substr(0, pong_prefix.size()) != pong_prefix)
        return std::nullopt;
    const std::string_view digits = line.substr(pong_prefix.size());
    if (digits.empty() || digits.size() > max_seq_digits)
        return std::nullopt;

    std::uint32_t seq = 0;
    const char *end = digits.data() + digits.size();
    const auto [ptr, ec] = std::from_chars(digits.data(), end, seq);
    if (ec != std::errc() || ptr != end)
        return std::nullopt;
    return seq;
}

void LineSplitter::feed(const char *data, std::size_t len, std::vector<std::string> &lines)
{
    for (std::size_t i = 0; i < len; ++i) {
        const char c = data[i];
        if (c != '\n') {
            if (partial_.size() < max_line + 1)  // +1: room for a '\r'
                partial_.push_back(c);
            else
                overflow_ = true;
            continue;
        }
        if (!partial_.empty() && partial_.back() == '\r')
            partial_.pop_back();
        if (!overflow_ && !partial_.empty() && partial_.size() <= max_line)
            lines.push_back(partial_);
        partial_.clear();
        overflow_ = false;
    }
}

void LineSplitter::reset()
{
    partial_.clear();
    overflow_ = false;
}

} // namespace dontfall::protocol
