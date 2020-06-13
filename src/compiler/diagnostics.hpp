#ifndef TIRO_COMPILER_DIAGNOSTICS_HPP
#define TIRO_COMPILER_DIAGNOSTICS_HPP

#include "common/format.hpp"
#include "common/iter_tools.hpp"
#include "compiler/source_reference.hpp"

#include <string>
#include <vector>

namespace tiro {

/// Gathers compile time warnings and errors.
class Diagnostics final {
public:
    enum Level { Error, Warning };

    struct Message {
        Level level = Error;
        SourceReference source;
        std::string text;

        Message() = default;

        Message(Level level_, const SourceReference& source_, std::string text_)
            : level(level_)
            , source(source_)
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
    void report(Level level, const SourceReference& source, std::string text);

    /// Report a message at the given source text location, with fmt::format syntax.
    template<typename... Args>
    void
    reportf(Level level, const SourceReference& source, std::string_view fmt_str, Args&&... args) {
        report(level, source, fmt::format(fmt_str, std::forward<Args>(args)...));
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
