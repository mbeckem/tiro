#ifndef TIRO_CORE_FIX_HPP
#define TIRO_CORE_FIX_HPP

#include <utility>

namespace tiro {

/// Makes it possible to write recursive lambda functions.
///
/// In C++, lambda functions cannot refer to themselves by name, i.e.
/// they cannot be used to implement recursive functions.
/// The class `Fix` takes a lambda (or any other function object)
/// as an argument and creates a new function object that always
/// passes itself as the first argument.
/// This additional argument enables the function to refer to itself.
///
/// Example:
///
///     Fix fib = [](auto& self, int i) -> int {
///         if (i == 0)
///             return 0;
///         if (i == 1)
///             return 1;
///         return self(i - 2) + self(i - 1);
///     });
///     fib(42); // Works.
///
template<typename Function>
class Fix {
private:
    Function fn_;

public:
    Fix(const Function& fn)
        : fn_(fn) {}

    Fix(Function&& fn)
        : fn_(std::move(fn)) {}

    template<typename... Args>
    decltype(auto) operator()(Args&&... args) const {
        return fn_(*this, std::forward<Args>(args)...);
    }
};

} // namespace tiro

#endif // TIRO_CORE_FIX_HPP
