#ifndef HAMMER_COMMON_ERROR_HPP
#define HAMMER_COMMON_ERROR_HPP

#include "hammer/core/defs.hpp"

#include <string>

#include <fmt/format.h>

namespace hammer {

/**
 * Error class thrown by the library when a fatal internal error occurs.
 * 
 * Normal errors (like syntax errors or runtime script errors) are reported
 * through other channels.
 */
class Error : public virtual std::exception {
public:
    explicit Error(std::string message);
    virtual ~Error();

    virtual const char* what() const noexcept;

private:
    std::string message_;
};

/** 
 * Throws an internal error exception. The arguments to the macro are passed to fmt::format.
 */
#define HAMMER_ERROR(...) \
    (throw_internal_error(__FILE__, __LINE__, __func__, fmt::format(__VA_ARGS__)))

/**
 * Evaluates a condition and, if the condition evaluates to false, throws an internal error.
 * All other arguments are passed to HAMMER_ERROR().
 */
#define HAMMER_CHECK(cond, ...)         \
    do {                                \
        if (HAMMER_UNLIKELY(!(cond))) { \
            HAMMER_ERROR(__VA_ARGS__);  \
        }                               \
    } while (0)

[[noreturn]] HAMMER_DISABLE_INLINE HAMMER_COLD void
throw_internal_error(const char* file, int line, const char* function, std::string message);

} // namespace hammer

#endif // HAMMER_COMMON_ERROR_HPP
