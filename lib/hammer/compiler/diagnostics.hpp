#ifndef HAMMER_COMPILER_DIAGNOSTICS_HPP
#define HAMMER_COMPILER_DIAGNOSTICS_HPP

#include "hammer/core/iter_range.hpp"
#include "hammer/compiler/source_reference.hpp"

#include <fmt/format.h>

#include <string>
#include <vector>

namespace hammer {

class Diagnostics {
public:
    enum Level { error, warning, not_implemented };

    static std::string_view to_string(Level level);

    struct Message {
        Level level = error;
        SourceReference source;
        std::string text;

        Message() = default;

        Message(Level level_, const SourceReference& source_, std::string text_)
            : level(level_)
            , source(source_)
            , text(std::move(text_)) {}
    };

public:
    bool has_errors() const { return errors_ > 0; }

    int error_count() const { return errors_; }
    int warning_count() const { return warnings_; }

    auto messages() const { return IterRange(messages_.cbegin(), messages_.cend()); }

    void report(Level level, const SourceReference& source, std::string text);

    template<typename... Args>
    void reportf(Level level, const SourceReference& source, std::string_view fmt_str,
                 Args&&... args) {
        report(level, source, fmt::format(fmt_str, std::forward<Args>(args)...));
    }

    size_t message_count() const { return messages_.size(); }

private:
    int errors_ = 0;
    int warnings_ = 0;
    std::vector<Message> messages_;
};

} // namespace hammer

#endif // HAMMER_COMPILER_DIAGNOSTICS_HPP
