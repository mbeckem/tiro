#include "hammer/core/defs.hpp"

#include <cstring>
#include <iostream>

namespace hammer::detail {

constexpr_assert_fail::constexpr_assert_fail(const char* file, int line, const char* cond,
                                             const char* message) {
    assert_fail(file, line, cond, message);
}

void assert_fail(const char* file, int line, const char* condition, const char* message) {
    std::cerr << "Assertion `" << condition << "` failed";
    if (message && std::strlen(message) > 0) {
        std::cerr << ": " << message;
    }
    std::cerr << "\n";
    std::cerr << "    (in " << file << ":" << line << ")" << std::endl;
    std::abort();
}

void unreachable(const char* file, int line, const char* message) {
    std::cerr << "Unreachable code executed";
    if (message && std::strlen(message) > 0) {
        std::cerr << ": " << message;
    }
    std::cerr << "\n";
    std::cerr << "    (in " << file << ":" << line << ")" << std::endl;
    std::abort();
}

void abort(const char* file, int line, const char* message) {
    if (message) {
        std::cerr << "Abort: " << message;
    } else {
        std::cerr << "Abort.";
    }
    std::cerr << "\n"
              << "    (in " << file << ":" << line << ")" << std::endl;
    std::abort();
}

} // namespace hammer::detail
