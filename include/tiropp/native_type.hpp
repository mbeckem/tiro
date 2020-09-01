#ifndef TIROPP_NATIVE_TYPE_HPP_INCLUDED
#define TIROPP_NATIVE_TYPE_HPP_INCLUDED

#include "tiropp/fwd.hpp"
#include "tiropp/objects.hpp"
#include "tiropp/vm.hpp"

#include <memory>

namespace tiro {

namespace detail {

template<typename T>
struct native_traits {
    struct layout {
        layout()
            : ptr(nullptr) {}

        // Points to `instance` if it has been fully constructed (and has not been destroyed yet).
        // Nullptr otherwise.
        T* ptr;
        union {
            T instance;
        };
    };

    static constexpr size_t size = sizeof(layout);

    static void constructor(T&& instance, void* data) {
        layout* l = new (data) layout();
        new (std::addressof(l->instance)) T(std::move(instance));
        l->ptr = &l->instance;
    }

    static T* accessor(void* data) { return static_cast<layout*>(data)->ptr; }

    static void finalizer(void* data, [[maybe_unused]] size_t size) noexcept {
        layout* l = static_cast<layout*>(data);
        if (l->ptr) {
            l->ptr->~T();
            l->ptr = nullptr;
        }
    }
};

struct native_type_data {
    std::string name;
    tiro_native_type_t descriptor{};
};

} // namespace detail

template<typename T>
class native_type final {
private:
    using native_traits = detail::native_traits<T>;

public:
    // TODO: efficient implementation for pointer types (they can just store a raw pointer).
    static_assert(!std::is_pointer_v<T> && !std::is_reference_v<T>,
        "T must neither be a reference nor a pointer type.");
    static_assert(std::is_nothrow_move_constructible_v<T>, "T must be nothrow move constructible.");
    static_assert(std::is_destructible_v<T>, "T must be destructible.");
    static_assert(std::is_nothrow_destructible_v<T>, "T must not throw from its destructor.");

    explicit native_type(std::string name)
        : holder_(std::make_unique<detail::native_type_data>()) {
        holder_->name = std::move(name);
        holder_->descriptor.name = holder_->name.c_str();
        holder_->descriptor.finalizer = native_traits::finalizer;
    }

    ~native_type() = default;

    native_type(native_type&&) noexcept = default;
    native_type& operator=(native_type&&) noexcept = default;

    /// Returns true is valid, i.e. if this native_type has not been moved from.
    bool valid() const { return holder_ != nullptr; }

    /// Returns the native type's name (the value of original constructor argument).
    const std::string& name() const {
        check_valid();
        return holder_->name;
    }

    /// Returns true if the given native object is an instance of this type.
    bool is_instance(const native& object) const {
        check_valid();
        return object.type_descriptor() == &holder_->descriptor;
    }

    /// Constructs a new object of this type. The contents of `instance` will be moved into the constructed object.
    native make(vm& v, T&& instance) const {
        check_valid();

        handle result(v.raw_vm());
        detail::check_handles(v.raw_vm(), result);
        tiro_make_native(v.raw_vm(), &holder_->descriptor, native_traits::size, result.raw_handle(),
            error_adapter());
        native_traits::constructor(
            std::move(instance), tiro_native_data(v.raw_vm(), result.raw_handle()));
        return native(std::move(result));
    }

    /// Returns the address of the native object instance.
    /// TODO: This API will have to change (or become more dangeous) once the gc starts to move objects around.
    T* access(const native& object) const {
        check_valid();
        check_instance(object);

        TIRO_ASSERT(object.data() != nullptr);
        TIRO_ASSERT(object.size() == native_traits::size);
        T* result = native_traits::accessor(object.data());
        if (!result)
            throw generic_error("The object was already destroyed.");
        return result;
    }

    /// Returns true if the referenced object was already destroyed manually by calling `destroy()`.
    bool is_destroyed(const native& object) const {
        check_valid();
        check_instance(object);

        TIRO_ASSERT(object.data() != nullptr);
        TIRO_ASSERT(object.size() == native_traits::size);
        return native_traits::accessor(object.data()) == nullptr;
    }

    /// Manually destroys the native object. Future `access()` calls to the same object will fail
    /// with an exception.
    /// Note that objects are destroyed automatically when they are collected by the garbage collector.
    /// This function allows the programmer to trigger the destruction at an earlier point in time.
    void destroy(const native& object) const {
        check_valid();
        check_instance(object);

        TIRO_ASSERT(object.data() != nullptr);
        TIRO_ASSERT(object.size() == native_traits::size);
        native_traits::finalizer(object.data(), native_traits::size);
    }

private:
    void check_valid() const {
        if (!valid())
            throw generic_error("This native_type is no longer valid.");
    }

    void check_instance(const native& object) const {
        if (!is_instance(object))
            throw generic_error("The object is not an instance of this type.");
    }

private:
    std::unique_ptr<detail::native_type_data> holder_;
};

} // namespace tiro

#endif // TIROPP_NATIVE_TYPE_HPP_INCLUDED
