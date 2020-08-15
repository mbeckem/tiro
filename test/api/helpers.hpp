#ifndef TIRO_TEST_API_HELPERS_HPP
#define TIRO_TEST_API_HELPERS_HPP

#include "tiro/api.h"

#include "fmt/format.h"

#include <assert.h>
#include <memory>
#include <stdexcept>

namespace tiro::api {

struct AllowNull {};

inline constexpr AllowNull allow_null{};

template<typename T, void (*delete_fn)(T)>
class Wrapper {
    static_assert(std::is_pointer_v<T>, "T must be a pointer type.");

public:
    using ValueType = T;

    Wrapper() = default;

    explicit Wrapper(T value)
        : value_(value) {
        if (!value_)
            throw std::invalid_argument("Wrapped value is null.");
    }

    explicit Wrapper(AllowNull, T value)
        : value_(value) {}

    ~Wrapper() { delete_fn(value_); }

    Wrapper(const Wrapper&) = delete;
    Wrapper& operator=(const Wrapper&) = delete;

    T get() { return value_; }
    operator T() { return value_; }

    void reset() {
        delete_fn(value_);
        value_ = nullptr;
    }

    T* out() {
        assert(value_ == nullptr && "Value must not have been initialized");
        return &value_;
    }

private:
    T value_ = nullptr;
};

class VM final : public Wrapper<tiro_vm_t, tiro_vm_free> {
    using Wrapper::Wrapper;
};

class Frame final : public Wrapper<tiro_frame_t, tiro_frame_free> {
    using Wrapper::Wrapper;
};

class Compiler final : public Wrapper<tiro_compiler_t, tiro_compiler_free> {
    using Wrapper::Wrapper;
};

class Module final : public Wrapper<tiro_module_t, tiro_module_free> {
    using Wrapper::Wrapper;
};

class Error final : public Wrapper<tiro_error_t, tiro_error_free> {
public:
    using Wrapper::Wrapper;

    void raise() {
        tiro_error_t err = get();
        assert(err != nullptr && "error must not be null");

        std::string message = fmt::format("{}: {}", tiro_error_name(err), tiro_error_message(err));

        std::string_view details = tiro_error_details(err);
        if (!details.empty()) {
            message += "\n";
            message += details;
        }

        throw std::runtime_error(std::move(message));
    }

    void check() {
        tiro_error_t err = get();
        if (err)
            raise();
    }
};

} // namespace tiro::api

#endif // TIRO_TEST_API_HELPERS_HPP
