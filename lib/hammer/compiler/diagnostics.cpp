#include "hammer/compiler/diagnostics.hpp"

#include "hammer/core/defs.hpp"

#include <fmt/format.h>

namespace hammer {

std::string_view Diagnostics::to_string(Level level) {
    switch (level) {
    case Warning:
        return "warning";
    case Error:
        return "error";
    }

    HAMMER_UNREACHABLE("Invalid message level.");
}

void Diagnostics::report(
    Level level, const SourceReference& source, std::string text) {

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

} // namespace hammer
