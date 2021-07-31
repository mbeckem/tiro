#ifndef TIRO_VM_ERROR_UTILS_HPP
#define TIRO_VM_ERROR_UTILS_HPP

#define TIRO_TRY_IMPL(name, expr, tempname) \
    auto&& tempname = (expr);               \
    if (tempname.has_exception()) {         \
        return tempname.exception();        \
    }                                       \
    auto name = std::move(tempname).value();

#define TIRO_TRY_VOID_IMPL(name, expr, tempname) \
    auto&& tempname = (expr);                    \
    if (tempname.has_exception()) {              \
        return tempname.exception();             \
    }

#define TIRO_FRAME_TRY_IMPL(name, expr, tempname) \
    auto&& tempname = (expr);                     \
    if (tempname.has_exception()) {               \
        return frame.panic(tempname.exception()); \
    }                                             \
    auto name = std::move(tempname).value();

#define TIRO_FRAME_TRY_VOID_IMPL(name, expr, tempname) \
    auto&& tempname = (expr);                          \
    if (tempname.has_exception()) {                    \
        return frame.panic(tempname.exception());      \
    }

#define TIRO_TRY_LOCAL_IMPL(name, expr, tempname, scope) \
    auto&& tempname = (expr);                            \
    if (tempname.has_exception()) {                      \
        return tempname.exception();                     \
    }                                                    \
    auto name = (scope).local(std::move(tempname).value());

// This dance is needed to expand __LINE__
#define TIRO_TRY_EXPAND_2(impl, name, expr, line, ...) \
    impl(name, expr, fallible_##name##_##line, ##__VA_ARGS__)
#define TIRO_TRY_EXPAND_1(impl, name, expr, line, ...) \
    TIRO_TRY_EXPAND_2(impl, name, expr, line, ##__VA_ARGS__)

/**
 * Evaluates `expr` (which must produce a Fallible<T>) and, if the fallible does not
 * contain an exception, declares a variable `name` of type `T` that holds the value.
 *
 * If the fallible does contain an exception, the function returns with that exception.
 *
 * Requirements:
 *  - no variable called `name` exists in the current scope
 *  - `expr` returns a Fallible<T>
 *  - the current function's return type is compatible with Exception, e.g. Fallible
 */
#define TIRO_TRY(name, expr) TIRO_TRY_EXPAND_1(TIRO_TRY_IMPL, name, expr, __LINE__)

/**
 * Like `TIRO_TRY`, but for expressions that return Fallible<void> or for cases
 * where the value inside a fallible can be discarded.
 * No variable is declared in the success case.
 */
#define TIRO_TRY_VOID(expr) TIRO_TRY_EXPAND_1(TIRO_TRY_VOID_IMPL, _, expr, __LINE__)

/**
 * Evaluates `expr` (which must produce a Fallible<T>) and, if the fallible does not
 * contain an exception, declares a variable `name` of type `T` that holds the value.
 *
 * If the fallible does contain an exception, the function returns via `frame.panic(exception)`.
 *
 * Requirements:
 *  - no variable called `name` exists in the current scope
 *  - `expr` returns a Fallible<T>
 *  - a variable called `frame` exists (which should be a native function frame)
 */
#define TIRO_FRAME_TRY(name, expr) TIRO_TRY_EXPAND_1(TIRO_FRAME_TRY_IMPL, name, expr, __LINE__)

/**
 * Like `TIRO_FRAME_TRY`, but for expressions that return Fallible<void>.
 * No variable is declared in the success case.
 */
#define TIRO_FRAME_TRY_VOID(expr) TIRO_TRY_EXPAND_1(TIRO_FRAME_TRY_VOID_IMPL, _, expr, __LINE__)

/**
 * Like `TIRO_FRAME_TRY`, but declares a `Local` instead.
 *
 * Requirements:
 *  - no variable called `name` exists in the current scope
 *  - `scope` is a valid `Scope`
 *  - `expr' returns a Fallible<T> where T is a type derived from `Value`
 */
#define TIRO_TRY_LOCAL(scope, name, expr) \
    TIRO_TRY_EXPAND_1(TIRO_TRY_LOCAL_IMPL, name, expr, __LINE__, scope)

#endif // TIRO_VM_ERROR_UTILS_HPP
