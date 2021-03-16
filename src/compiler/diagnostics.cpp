#include "compiler/diagnostics.hpp"

#include "common/defs.hpp"

#include <fmt/format.h>

namespace tiro {

void Diagnostics::report(Level level, const SourceRange& range, std::string text) {
    messages_.emplace_back(level, range, std::move(text));
    if (level == Error) {
        errors_++;
    } else {
        warnings_++;
    }
}

void Diagnostics::vreport(Level level, const SourceRange& range, std::string_view format_string,
    fmt::format_args format_args) {
    report(level, range, fmt::vformat(format_string, format_args));
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
