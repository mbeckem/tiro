#ifndef TIRO_COMPILER_DIAGNOSTICS_HPP
#define TIRO_COMPILER_DIAGNOSTICS_HPP

#include "common/format.hpp"
#include "common/ranges/iter_tools.hpp"
#include "compiler/source_range.hpp"

#include <string>
#include <vector>

namespace tiro {

/// Gathers compile time warnings and errors.
class Diagnostics final {
public:
    enum Level { Error, Warning };

    struct Message {
        Level level = Error;
        SourceRange range;
        std::string text;

        Message() = default;

        Message(Level level_, const SourceRange& range_, std::string text_)
            : level(level_)
            , range(range_)
            , text(std::move(text_)) {}
    };

public:
    /// True iff one or more errors have been reported through this instance.
    bool has_errors() const { return errors_ > 0; }

    /// Number of error messages.
    size_t error_count() const { return errors_; }

    /// Number of warning messages.
    size_t warning_count() const { return warnings_; }

    /// Total number of messages.
    size_t message_count() const { return messages_.size(); }

    /// Iterable ranges over all messages (in insertion order).
    auto messages() const { return IterRange(messages_.cbegin(), messages_.cend()); }

    /// Report a message at the given source text location.
    void report(Level level, const SourceRange& range, std::string text);

    void vreport(Level level, const SourceRange& range, std::string_view format_string,
        fmt::format_args format_args);

    /// Report a message at the given source text location, with fmt::format syntax.
    template<typename... Args>
    void reportf(Level level, const SourceRange& range, std::string_view format_string,
        const Args&... format_args) {
        vreport(level, range, format_string, fmt::make_format_args(format_args...));
    }

private:
    size_t errors_ = 0;
    size_t warnings_ = 0;
    std::vector<Message> messages_;
};

std::string_view to_string(Diagnostics::Level level);

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::Diagnostics::Level)

#endif // TIRO_COMPILER_DIAGNOSTICS_HPP
