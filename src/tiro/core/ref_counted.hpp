#ifndef TIRO_CORE_REF_COUNTED_HPP
#define TIRO_CORE_REF_COUNTED_HPP

#include "tiro/core/defs.hpp"

#include <type_traits>
#include <utility>

namespace tiro {

namespace detail {

template<typename T>
class RefCountedPtr;

} // namespace detail

template<typename T>
class Ref;

template<typename T>
class WeakRef;

/// Base class for reference counted objects.
// TODO not threadsafe
class RefCounted {
public:
    virtual ~RefCounted();

    RefCounted(const RefCounted&) = delete;
    RefCounted& operator=(const RefCounted&) = delete;

protected:
    RefCounted();

private:
    // A very simplistic implementation for weak pointers.
    // Weak pointers point into the WeakData struct (which is ref conuted).
    // When the main instance goes away, it sets the "self" pointer to null.
    struct WeakData {
        RefCounted* self;
        int refcount;

        void inc_ref();
        void dec_ref();
    };

    template<typename T>
    friend class detail::RefCountedPtr;

    template<typename T>
    friend class Ref;

    template<typename T>
    friend class WeakRef;

private:
    void inc_ref();
    void dec_ref(); // Careful -> "delete this" possible

    WeakData* weak_ref();

private:
    WeakData* weak_;
    int refcount_;
};

namespace detail {

template<typename T>
class RefCountedPtr {
public:
    RefCountedPtr() noexcept
        : ptr_(nullptr) {}

    RefCountedPtr(std::nullptr_t) noexcept
        : ptr_(nullptr) {}

    explicit RefCountedPtr(T* ptr, bool inc_ref = true) noexcept
        : ptr_(ptr) {
        if (ptr_ && inc_ref)
            ptr_->inc_ref();
    }

    RefCountedPtr(const RefCountedPtr& other) noexcept
        : ptr_(other.ptr_) {
        if (ptr_)
            ptr_->inc_ref();
    }

    RefCountedPtr(RefCountedPtr&& other) noexcept
        : ptr_(std::exchange(other.ptr_, nullptr)) {}

    ~RefCountedPtr() noexcept { reset(); }

    RefCountedPtr& operator=(const RefCountedPtr& other) noexcept {
        reset(other.ptr_);
        return *this;
    }

    RefCountedPtr& operator=(RefCountedPtr&& other) noexcept {
        TIRO_ASSERT(this != &other, "Move assignment to self is forbidden.");
        if (ptr_)
            ptr_->dec_ref();
        ptr_ = std::exchange(other.ptr_, nullptr);
        return *this;
    }

    T* release() noexcept { return std::exchange(ptr_, nullptr); }

    void reset() noexcept {
        if (ptr_) {
            ptr_->dec_ref();
            ptr_ = nullptr;
        }
    }

    void reset(T* ptr) noexcept {
        if (ptr)
            ptr->inc_ref();
        if (ptr_)
            ptr_->dec_ref();
        ptr_ = ptr;
    }

    void reset(std::nullptr_t) noexcept { reset(); }

    T* get() const noexcept { return ptr_; }

    T* operator->() const {
        TIRO_ASSERT(ptr_, "Dereferencing an invalid reference.");
        return ptr_;
    }

    T& operator*() const {
        TIRO_ASSERT(ptr_, "Dereferencing an invalid reference.");
        return *ptr_;
    }

    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    friend bool
    operator==(const RefCountedPtr& a, const RefCountedPtr& b) noexcept {
        return a.get() == b.get();
    }

    friend bool
    operator!=(const RefCountedPtr& a, const RefCountedPtr& b) noexcept {
        return a.get() != b.get();
    }

    friend bool operator==(const RefCountedPtr& a, std::nullptr_t) noexcept {
        return !static_cast<bool>(a);
    }

    friend bool operator!=(const RefCountedPtr& a, std::nullptr_t) noexcept {
        return static_cast<bool>(a);
    }

    friend bool operator==(std::nullptr_t, const RefCountedPtr& b) noexcept {
        return !static_cast<bool>(b);
    }

    friend bool operator!=(std::nullptr_t, const RefCountedPtr& b) noexcept {
        return static_cast<bool>(b);
    }

private:
    T* ptr_;
};

} // namespace detail

template<typename T>
class Ref final : public detail::RefCountedPtr<T> {
public:
    Ref() noexcept
        : detail::RefCountedPtr<T>(nullptr) {}

    Ref(std::nullptr_t) noexcept
        : detail::RefCountedPtr<T>(nullptr) {}

    explicit Ref(T* ptr, bool inc_ref = true) noexcept
        : detail::RefCountedPtr<T>(ptr, inc_ref) {
        static_assert(std::is_base_of_v<RefCounted, T>,
            "Type must publicly inherit from RefCounted.");
    }

    template<typename Derived,
        std::enable_if_t<std::is_base_of_v<T, Derived>>* = nullptr>
    Ref(const Ref<Derived>& other) noexcept
        : Ref(other.get()) {}

    template<typename Derived,
        std::enable_if_t<std::is_base_of_v<T, Derived>>* = nullptr>
    Ref(Ref<Derived>&& other) noexcept
        : Ref(other.release(), false) {}

    operator T*() const noexcept { return this->get(); }
};

template<typename T>
Ref<T> ref(T* ptr) {
    return Ref<T>(ptr, true);
}

template<typename T, typename... Args>
Ref<T> make_ref(Args&&... args) {
    T* obj = new T(std::forward<Args>(args)...);
    return Ref<T>(obj, false);
}

template<typename To, typename From>
Ref<To> static_ref_cast(Ref<From> from) {
    To* ptr = static_cast<To*>(from.release());
    return Ref<To>(ptr, false);
}

template<typename T>
class WeakRef final : private detail::RefCountedPtr<RefCounted::WeakData> {
public:
    using detail::RefCountedPtr<RefCounted::WeakData>::RefCountedPtr;

    WeakRef(const Ref<T>& ref)
        : WeakRef(ref.get()) {}

    template<typename U, std::enable_if_t<std::is_base_of_v<T, U>>* = nullptr>
    WeakRef(const Ref<U>& ref)
        : WeakRef(ref.get()) {}

    explicit WeakRef(T* object)
        : RefCountedPtr(
            object ? static_cast<RefCounted*>(object)->weak_ref() : nullptr) {}

    Ref<T> lock() const {
        auto data = get();
        if (data) {
            return Ref<T>(static_cast<T*>(data->self));
        }
        return Ref<T>(nullptr);
    }
};

} // namespace tiro

namespace std {

template<typename T>
struct hash<tiro::Ref<T>> {
    size_t operator()(const tiro::Ref<T>& ref) const noexcept {
        return hash<void*>()(ref.get());
    }
};

} // namespace std

#endif // TIRO_CORE_REF_COUNTED_HPP
