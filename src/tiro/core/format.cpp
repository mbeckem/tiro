#include "tiro/core/format.hpp"

namespace tiro {

FormatStream::FormatStream() {}

FormatStream::~FormatStream() {}

StringFormatStream::StringFormatStream(size_t initial_capacity) {
    buffer_.reserve(initial_capacity);
}

StringFormatStream::~StringFormatStream() {}

std::string StringFormatStream::take_str() {
    auto result = std::move(buffer_);
    buffer_.clear();
    return result;
}

void StringFormatStream::do_vformat(std::string_view format, fmt::format_args args) {
    fmt::vformat_to(std::back_inserter(buffer_), format, args);
}

BufferFormatStream::BufferFormatStream(fmt::memory_buffer& buffer)
    : buffer_(buffer) {}

BufferFormatStream ::~BufferFormatStream() = default;

void BufferFormatStream::do_vformat(std::string_view format, fmt::format_args args) {
    fmt::vformat_to(buffer_, format, args);
}

IndentStream::IndentStream(FormatStream& base, int indent, bool indent_first)
    : base_(base)
    , indent_(std::max(0, indent))
    , indent_next_(indent_first) {}

IndentStream::~IndentStream() {}

FormatStream& IndentStream::base() const {
    return base_;
}

int IndentStream::indent() const {
    return indent_;
}

void IndentStream::do_vformat(std::string_view format, fmt::format_args args) {
    buffer_.clear();
    fmt::vformat_to(buffer_, format, args);

    std::string_view message(buffer_.data(), buffer_.size());
    if (message.empty())
        return;

    if (indent_next_) {
        indent_next_ = false;
        base_.format("{0:>{1}}", "", indent_);
    }

    size_t cursor = 0;
    const size_t size = message.size();
    while (1) {
        const size_t line_end = message.find('\n', cursor);
        const size_t line_length = line_end == std::string_view::npos ? size - cursor
                                                                      : line_end - cursor + 1;

        base_.format("{}", message.substr(cursor, line_length));
        if (line_end != std::string_view::npos && line_end != size - 1)
            base_.format("{0:>{1}}", "", indent_);

        cursor += line_length;
        if (cursor == size) {
            break;
        }
    }

    indent_next_ = message.back() == '\n';
}

PrintStream::PrintStream()
    : out_(stdout) {}

PrintStream::PrintStream(std::FILE* file)
    : out_(file) {}

PrintStream::~PrintStream() {}

void PrintStream::do_vformat(std::string_view format, fmt::format_args args) {
    fmt::vprint(format, args);
}

} // namespace tiro
