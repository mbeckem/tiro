#include "hammer/compiler/diagnostics.hpp"

#include "hammer/core/defs.hpp"

namespace hammer {

std::string_view Diagnostics::to_string(Level level) {
    switch (level) {
    case warning:
        return "warning";
    case error:
        return "error";
    case not_implemented:
        return "not_implemented";
    }

    HAMMER_UNREACHABLE("Invalid message level.");
}

void Diagnostics::report(Level level, const SourceReference& source, std::string text) {
    messages_.emplace_back(level, source, std::move(text));
    if (level == error || level == not_implemented) {
        errors_++;
    } else {
        warnings_++;
    }
}

} // namespace hammer
