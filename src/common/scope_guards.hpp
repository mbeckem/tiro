#ifndef TIRO_COMMON_SCOPE_HPP
#define TIRO_COMMON_SCOPE_HPP

#include "common/defs.hpp"
#include "common/type_traits.hpp"

#include <exception>
#include <utility>

namespace tiro {

namespace detail {

template<typename Func, typename InvokePolicy>
class ScopeGuardBase {
public:
    ScopeGuardBase(const Func& fn)
        : fn_(fn) {}

    ScopeGuardBase(Func&& fn)
        : fn_(std::move(fn)) {}

    ~ScopeGuardBase() noexcept(false) {
        const bool is_unwinding = std::uncaught_exceptions() > uncaught_exceptions_;
        if (!InvokePolicy::is_guard_enabled(is_unwinding))
            return;

        if (is_unwinding) {
            try {
                fn_();
            } catch (...) {
                // Eat all exceptions because another exception is already in flight.
            }
        } else {
            fn_();
        }
    }

    ScopeGuardBase(const ScopeGuardBase&) = delete;
    ScopeGuardBase& operator=(const ScopeGuardBase&) = delete;

private:
    int uncaught_exceptions_ = std::uncaught_exceptions();
    Func fn_;
};

struct InvokeAlways {
    static bool is_guard_enabled([[maybe_unused]] bool is_unwinding) { return true; }
};

struct InvokeSuccess {
    static bool is_guard_enabled(bool is_unwindig) { return !is_unwindig; }
};

struct InvokeUnwind {
    static bool is_guard_enabled(bool is_unwinding) { return is_unwinding; }
};

}; // namespace detail

/// Scope exit guards run unconditionally whenever they are destroyed.
template<typename Func>
class ScopeExit final : public detail::ScopeGuardBase<Func, detail::InvokeAlways> {
    using Base = typename ScopeExit::ScopeGuardBase;
    using Base::Base;
};

/// Scope success guards run only when the scope is exited normally (without throwing an exception).
template<typename Func>
class ScopeSuccess final : public detail::ScopeGuardBase<Func, detail::InvokeSuccess> {
    using Base = typename ScopeSuccess::ScopeGuardBase;
    using Base::Base;
};

/// Scope failure guards run only when the scope is exited by throwing an exception.
template<typename Func>
class ScopeFailure final : public detail::ScopeGuardBase<Func, detail::InvokeUnwind> {
    using Base = typename ScopeFailure::ScopeGuardBase;
    using Base::Base;
};

template<typename Func>
ScopeExit(Func &&) -> ScopeExit<remove_cvref_t<Func>>;

template<typename Func>
ScopeSuccess(Func &&) -> ScopeSuccess<remove_cvref_t<Func>>;

template<typename Func>
ScopeFailure(Func &&) -> ScopeFailure<remove_cvref_t<Func>>;

} // namespace tiro

#endif // TIRO_COMMON_SCOPE_HPP
