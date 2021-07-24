#ifndef TIROPP_OBJECTS_HPP_INCLUDED
#define TIROPP_OBJECTS_HPP_INCLUDED

#include "tiropp/def.hpp"
#include "tiropp/detail/handle_check.hpp"
#include "tiropp/detail/resource_holder.hpp"
#include "tiropp/error.hpp"
#include "tiropp/fwd.hpp"
#include "tiropp/vm.hpp"

#include "tiro/objects.h"

#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace tiro {

/// Represents the kind of a value.
enum class value_kind : int {
    null = TIRO_KIND_NULL,
    boolean = TIRO_KIND_BOOLEAN,
    integer = TIRO_KIND_INTEGER,
    float_ = TIRO_KIND_FLOAT,
    string = TIRO_KIND_STRING,
    function = TIRO_KIND_FUNCTION,
    tuple = TIRO_KIND_TUPLE,
    record = TIRO_KIND_RECORD,
    array = TIRO_KIND_ARRAY,
    result = TIRO_KIND_RESULT,
    exception = TIRO_KIND_EXCEPTION,
    coroutine = TIRO_KIND_COROUTINE,
    module = TIRO_KIND_MODULE,
    type = TIRO_KIND_TYPE,
    native = TIRO_KIND_NATIVE,
    internal = TIRO_KIND_INTERNAL,
    invalid = TIRO_KIND_INVALID,
};

inline const char* to_string(value_kind k) {
    return tiro_kind_str(static_cast<tiro_kind>(k));
}

/// Thrown when an invalid cast is attempted.
class bad_handle_cast final : public error {
public:
    explicit bad_handle_cast(value_kind actual, value_kind expected) {
        details_ += "expected a value of kind ";
        details_ += to_string(expected);
        details_ += " but encountered a ";
        details_ += to_string(actual);

        what_ += "Bad handle cast (";
        what_ += details_;
        what_ += ")";
    }

    virtual const char* message() const noexcept override { return "Bad handle cast"; }
    virtual const char* details() const noexcept override { return details_.c_str(); }
    virtual const char* what() const noexcept override { return what_.c_str(); }

private:
    std::string details_;
    std::string what_;
};

/// A handle represents a reference to an object.
/// Valid handles point to an object slot which is managed by the tiro runtime.
/// All handles internally refer to the virtual machine they belong to.
class handle {
public:
    /// Constructs a new handle instance. The handle will belong to the given
    /// virtual machine and will be initialized with null.
    ///
    /// \pre `raw_vm != nullptr`.
    explicit handle(tiro_vm_t raw_vm)
        : h_(raw_vm, allocate_handle(raw_vm)) {}

    /// Constructs a new handle and initialize is it with the same value as `other`.
    /// If `other` is invalid, then the new handle will also become invalid.
    handle(const handle& other)
        : h_(other.raw_vm(), nullptr) {
        if (other.valid()) {
            h_.assign(allocate_handle(raw_vm()));
            detail::check_handles(raw_vm(), *this, other);
            tiro_value_copy(raw_vm(), other.raw_handle(), raw_handle(), error_adapter());
        }
    }

    /// Move constructs a handle. The other handle will become invalid: it may not longer
    /// be used for any operations other than destruction and assignments.
    handle(handle&& other) noexcept = default;

    ~handle() = default;

    /// Copy assigns a handle. This handle will contain the same value as `other`. If other was invalid, then
    /// this handle will also become invalid.
    handle& operator=(const handle& other) {
        // Fast path: simple value from other to this.
        if (raw_vm() == other.raw_vm() && valid() && other.valid()) {
            if (raw_handle() != other.raw_handle()) {
                detail::check_handles(raw_vm(), *this, other);
                tiro_value_copy(raw_vm(), other.raw_handle(), raw_handle(), error_adapter());
            }
        } else {
            *this = handle(other);
        }
        return *this;
    }

    /// Move assigns a handle. The other handle will become invalid: it may not longer
    /// be used for any operations other than destruction and assignments.
    handle& operator=(handle&& other) noexcept = default;

    /// Returns true if this handle points to an object (i.e. it was not moved from).
    bool valid() const { return h_.handle != nullptr; }

    /// Returns the kind of the value currently held by this handle.
    value_kind kind() const {
        detail::check_handles(raw_vm(), *this);
        return static_cast<value_kind>(tiro_value_kind(raw_vm(), raw_handle()));
    }

    /// Converts this value to the target type.
    template<typename T>
    T as() const& {
        static_assert(std::is_base_of_v<handle, T>, "target type must be derived from handle.");
        return T(*this);
    }

    template<typename T>
    T as() && {
        static_assert(std::is_base_of_v<handle, T>, "target type must be derived from handle.");
        return T(std::move(*this));
    }

    /// Returns the type of the value currently held by this handle.
    inline type type_of() const;

    /// Returns a string that represents the current value.
    inline string to_string() const;

    /// Returns the raw vm instance associated with this handle.
    tiro_vm_t raw_vm() const { return h_.vm; }

    /// Returns the raw handle instance (nullptr for invalid handles).
    tiro_handle_t raw_handle() const { return h_.handle; }

protected:
    struct check_kind_t {};

    static constexpr check_kind_t check_kind{};

    handle(check_kind_t, handle&& other, value_kind expected)
        : handle(std::move(other)) {
        value_kind actual = kind();
        if (actual != expected)
            throw bad_handle_cast(actual, expected);
    }

private:
    struct holder {
        tiro_vm_t vm;
        tiro_handle_t handle; // Handle may be invalid!

        holder(tiro_vm_t vm_, tiro_handle_t handle_)
            : vm(vm_)
            , handle(handle_) {
            TIRO_ASSERT(vm_);
        }

        holder(holder&& other) noexcept
            : vm(other.vm)
            , handle(std::exchange(other.handle, nullptr)) {}

        ~holder() { reset(); }

        holder& operator=(holder&& other) noexcept {
            if (this != &other) {
                reset();
                vm = other.vm;
                handle = std::exchange(other.handle, nullptr);
            }
            return *this;
        }

        void assign(tiro_handle_t h) {
            reset();
            handle = h;
        }

        void reset() {
            if (handle) {
                tiro_global_free(vm, handle);
                handle = nullptr;
            }
        }
    };

    static tiro_handle_t allocate_handle(tiro_vm_t raw_vm) {
        TIRO_ASSERT(raw_vm);

        tiro_handle_t raw_handle = tiro_global_new(raw_vm, error_adapter());
        TIRO_ASSERT(raw_handle); // Returns error on failure
        return raw_handle;
    }

private:
    holder h_;
};

/// Constructs a new handle as a copy of the given value.
inline handle make_copy(vm& v, tiro_handle_t value) {
    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), result);
    tiro_value_copy(v.raw_vm(), value, result.raw_handle(), error_adapter());
    return result;
}

/// Returns true if and only if `a` and `b` refer to the same value.
inline bool same(vm& v, const handle& a, const handle& b) {
    detail::check_handles(v.raw_vm(), a, b);
    return tiro_value_same(v.raw_vm(), a.raw_handle(), b.raw_handle());
}

class null final : public handle {
public:
    explicit null(handle h)
        : handle(check_kind, std::move(h), value_kind::null) {}

    null(const null&) = default;
    null(null&&) noexcept = default;

    null& operator=(const null&) = default;
    null& operator=(null&&) noexcept = default;
};

/// Constructs a new handle, initialized to null.
inline null make_null(vm& v) {
    // Fresh handles are always initialized to null.
    handle result(v.raw_vm());
    TIRO_ASSERT(result.kind() == value_kind::null);
    return null(std::move(result));
}

class boolean final : public handle {
public:
    explicit boolean(handle h)
        : handle(check_kind, std::move(h), value_kind::boolean) {}

    boolean(const boolean&) = default;
    boolean(boolean&&) noexcept = default;

    boolean& operator=(const boolean&) = default;
    boolean& operator=(boolean&&) noexcept = default;

    bool value() const {
        detail::check_handles(raw_vm(), *this);
        return tiro_boolean_value(raw_vm(), raw_handle());
    }
};

/// Constructs a new boolean value.
inline boolean make_boolean(vm& v, bool value) {
    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), result);
    tiro_make_boolean(v.raw_vm(), value, result.raw_handle(), error_adapter());
    return boolean(std::move(result));
}

class integer final : public handle {
public:
    explicit integer(handle h)
        : handle(check_kind, std::move(h), value_kind::integer) {}

    integer(const integer&) = default;
    integer(integer&&) noexcept = default;

    integer& operator=(const integer&) = default;
    integer& operator=(integer&&) noexcept = default;

    int64_t value() const {
        detail::check_handles(raw_vm(), *this);
        return tiro_integer_value(raw_vm(), raw_handle());
    }
};

/// Constructs a new integer value.
inline integer make_integer(vm& v, int64_t value) {
    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), result);
    tiro_make_integer(v.raw_vm(), value, result.raw_handle(), error_adapter());
    return integer(std::move(result));
}

class float_ final : public handle {
public:
    explicit float_(handle h)
        : handle(check_kind, std::move(h), value_kind::float_) {}

    float_(const float_&) = default;
    float_(float_&&) noexcept = default;

    float_& operator=(const float_&) = default;
    float_& operator=(float_&&) noexcept = default;

    double value() const {
        detail::check_handles(raw_vm(), *this);
        return tiro_float_value(raw_vm(), raw_handle());
    }
};

/// Constructs a new float value.
inline float_ make_float(vm& v, double value) {
    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), result);
    tiro_make_float(v.raw_vm(), value, result.raw_handle(), error_adapter());
    return float_(std::move(result));
}

class string final : public handle {
public:
    explicit string(handle h)
        : handle(check_kind, std::move(h), value_kind::string) {}

    string(const string&) = default;
    string(string&&) noexcept = default;

    string& operator=(const string&) = default;
    string& operator=(string&&) noexcept = default;

    /// Returns an unowned view into the string's storage without performing a copy. The view is not zero terminated.
    ///
    /// \warning
    ///     The view points into the string's current storage. Because objects may move on the heap (e.g. because of garbage collection)
    ///     this data may be invalidated. The data may only be used immediately after calling this function, before any other vm function
    ///     might allocate and therefore might trigger a garbage collection cycle.
    std::string_view view() const {
        detail::check_handles(raw_vm(), *this);

        tiro_string_t value;
        tiro_string_value(raw_vm(), raw_handle(), &value, error_adapter());
        return std::string_view(value.data, value.length);
    }

    /// Returns a copy of the string's content, converted to a std::string.
    std::string value() const { return std::string(view()); }
};

/// Constructs a new string value.
inline string make_string(vm& v, std::string_view value) {
    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), result);
    tiro_make_string(
        v.raw_vm(), {value.data(), value.size()}, result.raw_handle(), error_adapter());
    return string(std::move(result));
}

class function final : public handle {
public:
    explicit function(handle h)
        : handle(check_kind, std::move(h), value_kind::function) {}

    function(const function&) = default;
    function(function&&) noexcept = default;

    function& operator=(const function&) = default;
    function& operator=(function&&) noexcept = default;

    // TODO: Remove sync API!

    /// Calls `func` with the provided arguments and returns the result.
    /// `args` may be null (for 0 arguments), or a tuple.
    inline handle call(const handle& args);

    /// Calls `func` with zero arguments and returns the result.
    inline handle call();
};

class sync_frame final {
public:
    sync_frame(tiro_vm_t raw_vm, tiro_sync_frame_t raw_frame)
        : raw_vm_(raw_vm)
        , raw_frame_(raw_frame) {
        TIRO_ASSERT(raw_vm);
        TIRO_ASSERT(raw_frame);
    }

    sync_frame(const sync_frame&) = delete;
    sync_frame& operator=(const sync_frame&) = delete;

    size_t argc() const { return tiro_sync_frame_argc(raw_frame_); }

    handle arg(size_t index) const {
        handle result(raw_vm_);
        detail::check_handles(raw_vm_, result);
        tiro_sync_frame_arg(raw_frame_, index, result.raw_handle(), error_adapter());
        return result;
    }

    handle closure() const {
        handle result(raw_vm_);
        detail::check_handles(raw_vm_, result);
        tiro_sync_frame_closure(raw_frame_, result.raw_handle(), error_adapter());
        return result;
    }

    tiro_vm_t raw_vm() const { return raw_vm_; }

    tiro_sync_frame_t raw_frame() const { return raw_frame_; }

private:
    // Both are unowned.
    tiro_vm_t raw_vm_;
    tiro_sync_frame_t raw_frame_;
};

template<auto Function>
function make_sync_function(vm& v, const string& name, size_t argc, const handle& closure) {
    constexpr tiro_sync_function_t func = [](tiro_vm_t raw_vm, tiro_sync_frame_t raw_frame) {
        try {
            vm& inner_v = vm::unsafe_from_raw_vm(raw_vm);
            sync_frame frame(raw_vm, raw_frame);
            handle result = Function(inner_v, frame);
            detail::check_handles(raw_vm, result);
            tiro_sync_frame_return_value(raw_frame, result.raw_handle(), error_adapter());
        } catch (...) {
            std::terminate(); // TODO Exceptions!
        }
    };

    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), name, closure, result);
    tiro_make_sync_function(v.raw_vm(), name.raw_handle(), func, argc, closure.raw_handle(),
        result.raw_handle(), error_adapter());
    return function(std::move(result));
}

class async_frame final {
public:
    async_frame(tiro_vm_t raw_vm, tiro_async_frame_t raw_frame)
        : raw_vm_(raw_vm)
        , raw_frame_(raw_frame) {
        TIRO_ASSERT(raw_vm);
        TIRO_ASSERT(raw_frame);
    }

    async_frame(async_frame&&) noexcept = default;
    async_frame& operator=(async_frame&&) noexcept = default;

    size_t argc() const { return tiro_async_frame_argc(raw_frame_); }

    handle arg(size_t index) const {
        handle result(raw_vm_);
        detail::check_handles(raw_vm_, result);
        tiro_async_frame_arg(raw_frame_, index, result.raw_handle(), error_adapter());
        return result;
    }

    handle closure() const {
        handle result(raw_vm_);
        detail::check_handles(raw_vm_, result);
        tiro_async_frame_closure(raw_frame_, result.raw_handle(), error_adapter());
        return result;
    }

    void return_value(const handle& value) {
        detail::check_handles(raw_vm_, value);
        tiro_async_frame_return_value(raw_frame_, value.raw_handle(), error_adapter());
    }

    tiro_vm_t raw_vm() const { return raw_vm_; }

    tiro_async_frame_t raw_frame() const { return raw_frame_; }

private:
    tiro_vm_t raw_vm_; // Unowned
    detail::resource_holder<tiro_async_frame_t, tiro_async_frame_free> raw_frame_;
};

template<auto Function>
function make_async_function(vm& v, const string& name, size_t argc, const handle& closure) {
    constexpr tiro_async_function_t func = [](tiro_vm_t raw_vm, tiro_async_frame_t raw_frame) {
        try {
            vm& inner_v = vm::unsafe_from_raw_vm(raw_vm);
            async_frame frame(raw_vm, raw_frame);
            Function(inner_v, std::move(frame));
        } catch (...) {
            std::terminate(); // TODO Exceptions
        }
    };

    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), name, closure, result);
    tiro_make_async_function(v.raw_vm(), name.raw_handle(), func, argc, closure.raw_handle(),
        result.raw_handle(), error_adapter());
    return function(std::move(result));
}

class tuple final : public handle {
public:
    explicit tuple(handle h)
        : handle(check_kind, std::move(h), value_kind::tuple) {}

    tuple(const tuple&) = default;
    tuple(tuple&&) noexcept = default;

    tuple& operator=(const tuple&) = default;
    tuple& operator=(tuple&&) noexcept = default;

    size_t size() const {
        detail::check_handles(raw_vm(), *this);
        return tiro_tuple_size(raw_vm(), raw_handle());
    }

    handle get(size_t index) const {
        handle result(raw_vm());
        detail::check_handles(raw_vm(), *this, result);
        tiro_tuple_get(raw_vm(), raw_handle(), index, result.raw_handle(), error_adapter());
        return result;
    }

    void set(size_t index, const handle& value) {
        detail::check_handles(raw_vm(), *this, value);
        tiro_tuple_set(raw_vm(), raw_handle(), index, value.raw_handle(), error_adapter());
    }
};

/// Constructs a new tuple value with the given size. All elements of that tuple
/// will be initialized to null.
inline tuple make_tuple(vm& v, size_t size) {
    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), result);
    tiro_make_tuple(v.raw_vm(), size, result.raw_handle(), error_adapter());
    return tuple(std::move(result));
}

class record final : public handle {
public:
    explicit record(handle h)
        : handle(check_kind, std::move(h), value_kind::record) {}

    record(const record&) = default;
    record(record&&) noexcept = default;

    record& operator=(const record&) = default;
    record& operator=(record&&) noexcept = default;

    inline array keys() const;

    handle get(const string& key) const {
        handle result(raw_vm());
        detail::check_handles(raw_vm(), *this, key, result);
        tiro_record_get(
            raw_vm(), raw_handle(), key.raw_handle(), result.raw_handle(), error_adapter());
        return result;
    }

    void set(const string& key, const handle& value) {
        detail::check_handles(raw_vm(), *this, key, value);
        tiro_record_set(
            raw_vm(), raw_handle(), key.raw_handle(), value.raw_handle(), error_adapter());
    }
};

/// Constructs a new record with the given keys. All keys must be unique strings.
/// All values of the record will be initialized to null.
inline record make_record(vm& v, const array& keys);

class array final : public handle {
public:
    explicit array(handle h)
        : handle(check_kind, std::move(h), value_kind::array) {}

    array(const array&) = default;
    array(array&&) noexcept = default;

    array& operator=(const array&) = default;
    array& operator=(array&&) noexcept = default;

    size_t size() const {
        detail::check_handles(raw_vm(), *this);
        return tiro_array_size(raw_vm(), raw_handle());
    }

    handle get(size_t index) const {
        handle result(raw_vm());
        detail::check_handles(raw_vm(), *this, result);
        tiro_array_get(raw_vm(), raw_handle(), index, result.raw_handle(), error_adapter());
        return result;
    }

    void set(size_t index, const handle& value) {
        detail::check_handles(raw_vm(), *this, value);
        tiro_array_set(raw_vm(), raw_handle(), index, value.raw_handle(), error_adapter());
    }

    void push(const handle& value) {
        detail::check_handles(raw_vm(), *this, value);
        tiro_array_push(raw_vm(), raw_handle(), value.raw_handle(), error_adapter());
    }

    void pop() {
        detail::check_handles(raw_vm(), *this);
        tiro_array_pop(raw_vm(), raw_handle(), error_adapter());
    }

    void clear() {
        detail::check_handles(raw_vm(), *this);
        tiro_array_clear(raw_vm(), raw_handle(), error_adapter());
    }
};

/// Constructs a new array with the given initial capacity. The array will be empty.
inline array make_array(vm& v, size_t initial_capacity = 0) {
    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), result);
    tiro_make_array(v.raw_vm(), initial_capacity, result.raw_handle(), error_adapter());
    return array(std::move(result));
}

class result final : public handle {
public:
    explicit result(handle h)
        : handle(check_kind, std::move(h), value_kind::result) {}

    result(const result&) = default;
    result(result&&) noexcept = default;

    result& operator=(const result&) = default;
    result& operator=(result&&) noexcept = default;

    bool is_success() const {
        detail::check_handles(raw_vm(), *this);
        return tiro_result_is_success(raw_vm(), raw_handle());
    }

    bool is_error() const {
        detail::check_handles(raw_vm(), *this);
        return tiro_result_is_error(raw_vm(), raw_handle());
    }

    handle value() const {
        handle value(raw_vm());
        detail::check_handles(raw_vm(), *this, value);
        tiro_result_value(raw_vm(), raw_handle(), value.raw_handle(), error_adapter());
        return value;
    }

    handle error() const {
        handle reason(raw_vm());
        detail::check_handles(raw_vm(), *this, reason);
        tiro_result_error(raw_vm(), raw_handle(), reason.raw_handle(), error_adapter());
        return reason;
    }
};

inline result make_success(vm& v, const handle& value) {
    handle out(v.raw_vm());
    detail::check_handles(v.raw_vm(), value, out);
    tiro_make_success(v.raw_vm(), value.raw_handle(), out.raw_handle(), error_adapter());
    return result(std::move(out));
}

inline result make_error(vm& v, const handle& err) {
    handle out(v.raw_vm());
    detail::check_handles(v.raw_vm(), err, out);
    tiro_make_error(v.raw_vm(), err.raw_handle(), out.raw_handle(), error_adapter());
    return result(std::move(out));
}

class exception final : public handle {
public:
    explicit exception(handle h)
        : handle(check_kind, std::move(h), value_kind::exception) {}

    exception(const exception&) = default;
    exception(exception&&) noexcept = default;

    exception& operator=(const exception&) = default;
    exception& operator=(exception&&) noexcept = default;

    string message() const {
        handle result(raw_vm());
        detail::check_handles(raw_vm(), *this, result);
        tiro_exception_message(raw_vm(), raw_handle(), result.raw_handle(), error_adapter());
        return string(std::move(result));
    }
};

class coroutine final : public handle {
public:
    explicit coroutine(handle h)
        : handle(check_kind, std::move(h), value_kind::coroutine) {}

    coroutine(const coroutine&) = default;
    coroutine(coroutine&&) noexcept = default;

    coroutine& operator=(const coroutine&) = default;
    coroutine& operator=(coroutine&&) noexcept = default;

    bool started() const {
        detail::check_handles(raw_vm(), *this);
        return tiro_coroutine_started(raw_vm(), raw_handle());
    }

    bool completed() const {
        detail::check_handles(raw_vm(), *this);
        return tiro_coroutine_completed(raw_vm(), raw_handle());
    }

    tiro::result result() const {
        handle result(raw_vm());
        detail::check_handles(raw_vm(), *this, result);
        tiro_coroutine_result(raw_vm(), raw_handle(), result.raw_handle(), error_adapter());
        return tiro::result(std::move(result));
    }

    template<typename Callback>
    void set_callback(Callback&& on_complete) {
        using wrapper_type = callback_wrapper<std::decay_t<Callback>>;
        auto wrapper = std::make_unique<wrapper_type>(
            in_place_t(), std::forward<Callback>(on_complete));

        detail::check_handles(raw_vm(), *this);
        tiro_coroutine_set_callback(raw_vm(), raw_handle(), &wrapper_type::invoke,
            &wrapper_type::cleanup, wrapper.release(), error_adapter());
    }

    void start() {
        detail::check_handles(raw_vm(), *this);
        tiro_coroutine_start(raw_vm(), raw_handle(), error_adapter());
    }

private:
    struct in_place_t {};

    template<typename Callback>
    struct callback_wrapper {
        Callback cb;

        template<typename T>
        callback_wrapper(in_place_t, T&& t)
            : cb(std::forward<T>(t)) {}

        callback_wrapper(callback_wrapper&&) noexcept = delete;
        callback_wrapper& operator=(callback_wrapper&&) noexcept = delete;

        static void invoke(tiro_vm_t raw_vm, tiro_handle_t raw_coroutine, void* userdata) {
            try {
                vm& v = vm::unsafe_from_raw_vm(raw_vm);
                coroutine coro = make_copy(v, raw_coroutine).as<coroutine>();
                static_cast<callback_wrapper*>(userdata)->cb(v, coro);
            } catch (...) {
                // Cannot throw across c call boundary.
                // TODO: Implement exception mechanism
                std::terminate();
            }
        }

        static void cleanup(void* userdata) { delete static_cast<callback_wrapper*>(userdata); }
    };
};

/// Constructs a new coroutine value. The coroutine will call the given function with the provided
/// arguments, once it has been started.
inline coroutine make_coroutine(vm& v, const function& func, const handle& arguments) {
    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), func, arguments, result);
    tiro_make_coroutine(v.raw_vm(), func.raw_handle(), arguments.raw_handle(), result.raw_handle(),
        error_adapter());
    return coroutine(std::move(result));
}

/// Constructs a new coroutine value. The coroutine will call the given function without any
/// arguments, once it has been started.
inline coroutine make_coroutine(vm& v, const function& func) {
    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), func, result);
    tiro_make_coroutine(
        v.raw_vm(), func.raw_handle(), nullptr, result.raw_handle(), error_adapter());
    return coroutine(std::move(result));
}

class module final : public handle {
public:
    explicit module(handle h)
        : handle(check_kind, std::move(h), value_kind::module) {}

    module(const module&) = default;
    module(module&&) noexcept = default;

    module& operator=(const module&) = default;
    module& operator=(module&&) noexcept = default;

    handle get_export(const char* export_name) const {
        handle result(raw_vm());
        detail::check_handles(raw_vm(), *this, result);
        tiro_module_get_export(
            raw_vm(), raw_handle(), export_name, result.raw_handle(), error_adapter());
        return result;
    }
};

// TODO: API not good enough (vector, strings).
inline module
make_module(vm& v, const char* name, const std::vector<std::pair<std::string, handle>>& exports) {
    const size_t exports_size = exports.size();
    std::vector<tiro_module_member_t> raw_exports(exports_size);
    for (size_t i = 0; i < exports_size; ++i) {
        raw_exports[i].name = exports[i].first.c_str();
        raw_exports[i].value = exports[i].second.raw_handle();
    }

    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), result);
    tiro_make_module(
        v.raw_vm(), name, raw_exports.data(), exports_size, result.raw_handle(), error_adapter());
    return module(std::move(result));
}

class native final : public handle {
public:
    explicit native(handle h)
        : handle(check_kind, std::move(h), value_kind::native) {}

    native(const native&) = default;
    native(native&&) noexcept = default;

    native& operator=(const native&) = default;
    native& operator=(native&&) noexcept = default;

    const tiro_native_type_t* type_descriptor() const {
        detail::check_handles(raw_vm(), *this);
        const tiro_native_type_t* result = tiro_native_type_descriptor(raw_vm(), raw_handle());
        if (!result)
            throw generic_error("Invalid access to native object.");
        return result;
    }

    void* data() const {
        detail::check_handles(raw_vm(), *this);
        void* result = tiro_native_data(raw_vm(), raw_handle());
        if (!result)
            throw generic_error("Invalid access to native object.");
        return result;
    }

    size_t size() const {
        detail::check_handles(raw_vm(), *this);
        return tiro_native_size(raw_vm(), raw_handle());
    }
};

class type final : public handle {
public:
    explicit type(handle h)
        : handle(check_kind, std::move(h), value_kind::type) {}

    type(const type&) = default;
    type(type&&) noexcept = default;

    type& operator=(const type&) = default;
    type& operator=(type&&) noexcept = default;

    string name() const {
        handle result(raw_vm());
        detail::check_handles(raw_vm(), *this, result);
        tiro_type_name(raw_vm(), raw_handle(), result.raw_handle(), error_adapter());
        return string(std::move(result));
    }
};

type handle::type_of() const {
    handle result(raw_vm());
    detail::check_handles(raw_vm(), *this, result);
    tiro_value_type(raw_vm(), raw_handle(), result.raw_handle(), error_adapter());
    return type(std::move(result));
}

string handle::to_string() const {
    handle result(raw_vm());
    detail::check_handles(raw_vm(), *this, result);
    tiro_value_to_string(raw_vm(), raw_handle(), result.raw_handle(), error_adapter());
    return string(std::move(result));
}

array record::keys() const {
    handle result(raw_vm());
    detail::check_handles(raw_vm(), *this, result);
    tiro_record_keys(raw_vm(), raw_handle(), result.raw_handle(), error_adapter());
    return array(std::move(result));
}

handle function::call(const handle& args) {
    handle result(raw_vm());
    detail::check_handles(raw_vm(), *this, args, result);
    tiro_vm_call(raw_vm(), raw_handle(), args.raw_handle(), result.raw_handle(), error_adapter());
    return result;
}

handle function::call() {
    handle result(raw_vm());
    detail::check_handles(raw_vm(), *this, result);
    tiro_vm_call(raw_vm(), raw_handle(), nullptr, result.raw_handle(), error_adapter());
    return result;
}

inline record make_record(vm& v, const array& keys) {
    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), keys, result);
    tiro_make_record(v.raw_vm(), keys.raw_handle(), result.raw_handle(), error_adapter());
    return record(std::move(result));
}

/// Attempts to find an exported value called `export_name` in the module `module_name`.
inline handle get_export(const vm& v, const char* module_name, const char* export_name) {
    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), result);
    tiro_vm_get_export(v.raw_vm(), module_name, export_name, result.raw_handle(), error_adapter());
    return result;
}

/// Attempts to load the given module into the virtual machine.
inline void load_module(const vm& v, const module& m) {
    detail::check_handles(v.raw_vm(), m);
    tiro_vm_load_module(v.raw_vm(), m.raw_handle(), error_adapter());
}

/// Schedules execution of `func` in a new coroutine without any arguments.
/// The callback `cb` will be called once `func` has completed its execution.
/// Note that `func` will not be executed from within this function (the next call to
/// `vm.run_ready()` will do that).
/// TODO: Extract the result() from the coroutine and pass it directly to the callback.
template<typename Callback>
inline void run_async(vm& v, const function& func, Callback&& cb) {
    tiro::coroutine coro = make_coroutine(v, func);
    coro.set_callback(std::forward<Callback>(cb));
    coro.start();
}

/// Schedules execution of `func` in a new coroutine, with the provided arguments.
/// The callback `cb` will be called once `func` has completed its execution.
/// Note that `func` will not be executed from within this function (the next call to
/// `vm.run_ready()` will do that).
/// TODO: Extract the result() from the coroutine and pass it directly to the callback.
template<typename Callback>
inline void run_async(vm& v, const function& func, const handle& args, Callback&& cb) {
    tiro::coroutine coro = make_coroutine(v, func, args);
    coro.set_callback(std::forward<Callback>(cb));
    coro.start();
}

} // namespace tiro

#endif // TIROPP_OBJECTS_HPP_INCLUDED
