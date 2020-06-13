#include "compiler/diagnostics.hpp"

#include "common/defs.hpp"

#include <fmt/format.h>

namespace tiro {

void Diagnostics::report(Level level, const SourceReference& source, std::string text) {

#if 0 == 1
    fmt::print("{}: {}\n", to_string(level), text);
#endif

    messages_.emplace_back(level, source, std::move(text));
    if (level == Error) {
        errors_++;
    } else {
        warnings_++;
    }
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
