#include "compiler/diagnostics.hpp"

#include "common/defs.hpp"

#include <fmt/format.h>

namespace tiro {

void Diagnostics::report(Level level, const SourceReference& source, std::string text) {
    messages_.emplace_back(level, source, std::move(text));
    if (level == Error) {
        errors_++;
    } else {
        warnings_++;
    }
}

void Diagnostics::vreport(Level level, const SourceReference& source,
    std::string_view format_string, fmt::format_args format_args) {
    report(level, source, fmt::vformat(format_string, format_args));
}

void Diagnostics::truncate(size_t message_count) {
    if (messages_.size() <= message_count)
        return;

    auto begin = messages_.begin() + message_count;
    auto end = messages_.end();
    for (auto p = begin; p != end; ++p) {
        switch (p->level) {
        case Error:
            --errors_;
            break;
        case Warning:
            --warnings_;
            break;
        }
    }
    messages_.erase(begin, end);
}

std::string_view to_string(Diagnostics::Level level) {
    switch (level) {
    case Diagnostics::Warning:
        return "warning";
    case Diagnostics::Error:
        return "error";
    }

    TIRO_UNREACHABLE("Invalid message level.");
}

} // namespace tiro
