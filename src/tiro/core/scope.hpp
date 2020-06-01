#ifndef TIRO_CORE_SCOPE_HPP
#define TIRO_CORE_SCOPE_HPP

#include "tiro/core/defs.hpp"

#include <exception>
#include <utility>

namespace tiro {

/// A ScopeExit object can execute an arbitrary function object from its destructor. It is typically
/// used for custom cleanup actions.
///
/// ScopeExit object can be enabled or disabled. An enabled ScopeExit will execute the function
/// object it has been created from when it is being destroyed. A disabled ScopeExit will do nothing.
/// When a ScopeExit is being moved, the moved-from object becomes disabled.
/// ScopeExits are enabled by default.
template<typename Function>
class [[nodiscard]] ScopeExit final {
private:
    bool invoke_ = false;
    Function fn_;

public:
    /// Constructs a ScopeExit object that will execute `fn` in its destructor,
    /// unless it was disabled previously.
    ScopeExit(const Function& fn)
        : invoke_(true)
        , fn_(fn) {}

    /// Constructs a ScopeExit object that will execute `fn` in its destructor,
    /// unless it was disabled previously.
    ScopeExit(Function && fn)
        : invoke_(true)
        , fn_(std::move(fn)) {}

    /// Executes the function object, unless disabled.
    ~ScopeExit() noexcept(noexcept(fn_())) {
        try {
            if (invoke_)
                fn_();
        } catch (...) {
            if (!std::uncaught_exceptions())
                throw;
        }
    }

    ScopeExit(ScopeExit && other) noexcept(std::is_nothrow_move_constructible_v<Function>)
        : invoke_(std::exchange(other.invoke_, false))
        , fn_(std::move(other.fn_)) {}

    ScopeExit(const ScopeExit& other) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;

    /// Enable the execution of the function object in the destructor of `*this`.
    void enable() { invoke_ = true; }

    /// Disables the execution of the function object in the destructor of `*this`.
    void disable() { invoke_ = false; }

    /// Returns true if the function object will be executed.
    bool enabled() const { return invoke_; }
};

/// Invokes a function object when the scope is left successfully, i.e.
/// when no exception is in flight.
template<typename Function>
class [[nodiscard]] ScopeSuccess final {
private:
    bool invoke_ = false;
    Function fn_;

public:
    /// Constructs a ScopeSuccess object that will execute `fn` in its destructor,
    /// unless it was disabled previously.
    ScopeSuccess(const Function& fn)
        : invoke_(true)
        , fn_(fn) {}

    /// Constructs a ScopeSuccess object that will execute `fn` in its destructor,
    /// unless it was disabled previously.
    ScopeSuccess(Function && fn)
        : invoke_(true)
        , fn_(std::move(fn)) {}

    /// Executes the function object, unless disabled or an active exception is in flight.
    /// TODO nested exceptions?
    ~ScopeSuccess() noexcept(noexcept(fn_())) {
        if (invoke_ && !std::uncaught_exceptions())
            fn_();
    }

    ScopeSuccess(ScopeSuccess && other) noexcept(std::is_nothrow_move_constructible_v<Function>)
        : invoke_(std::exchange(other.invoke_, false))
        , fn_(std::move(other.fn_)) {}

    ScopeSuccess(const ScopeSuccess& other) = delete;
    ScopeSuccess& operator=(const ScopeSuccess&) = delete;

    /// Enable the execution of the function object in the destructor of `*this`.
    void enable() { invoke_ = true; }

    /// Disables the execution of the function object in the destructor of `*this`.
    void disable() { invoke_ = false; }

    /// Returns true if the function object will be executed.
    bool enabled() const { return invoke_; }
};

} // namespace tiro

#endif // TIRO_CORE_SCOPE_HPP
