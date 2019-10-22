#ifndef HAMMER_COMPILER_DIAGNOSTICS_HPP
#define HAMMER_COMPILER_DIAGNOSTICS_HPP

#include "hammer/compiler/source_reference.hpp"
#include "hammer/core/iter_range.hpp"

#include <fmt/format.h>

#include <string>
#include <vector>

namespace hammer {

class Diagnostics {
public:
    enum Level { Error, Warning };

    static std::string_view to_string(Level level);

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
    /// True iff at least one error has been reported through this instance.
    bool has_errors() const { return errors_ > 0; }

    /// Number of error messages.
    size_t error_count() const { return errors_; }

    /// Number of warning messages.
    size_t warning_count() const { return warnings_; }

    /// Total number of messages.
    size_t message_count() const { return messages_.size(); }

    /// Iterable ranges over all messages (in insertion order).
    auto messages() const {
        return IterRange(messages_.cbegin(), messages_.cend());
    }

    /// Report a message at the given source text location.
    void report(Level level, const SourceReference& source, std::string text);

    /// Rerport a message at the given source text location, with fmt::format syntax.
    template<typename... Args>
    void reportf(Level level, const SourceReference& source,
        std::string_view fmt_str, Args&&... args) {
        report(
            level, source, fmt::format(fmt_str, std::forward<Args>(args)...));
    }

private:
    size_t errors_ = 0;
    size_t warnings_ = 0;
    std::vector<Message> messages_;
};

} // namespace hammer

#endif // HAMMER_COMPILER_DIAGNOSTICS_HPP
