#ifndef HAMMER_CORE_REF_COUNTED_HPP
#define HAMMER_CORE_REF_COUNTED_HPP

#include "hammer/core/defs.hpp"

#include <type_traits>
#include <utility>

namespace hammer {

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
    HAMMER_FORCE_INLINE RefCountedPtr() noexcept
        : ptr_(nullptr) {}

    HAMMER_FORCE_INLINE RefCountedPtr(std::nullptr_t) noexcept
        : ptr_(nullptr) {}

    HAMMER_FORCE_INLINE explicit RefCountedPtr(
        T* ptr, bool inc_ref = true) noexcept
        : ptr_(ptr) {
        if (ptr_ && inc_ref)
            ptr_->inc_ref();
    }

    HAMMER_FORCE_INLINE RefCountedPtr(const RefCountedPtr& other) noexcept
        : ptr_(other.ptr_) {
        if (ptr_)
            ptr_->inc_ref();
    }

    HAMMER_FORCE_INLINE RefCountedPtr(RefCountedPtr&& other) noexcept
        : ptr_(std::exchange(other.ptr_, nullptr)) {}

    HAMMER_FORCE_INLINE ~RefCountedPtr() noexcept {
        if (ptr_) {
            ptr_->dec_ref();
        }
    }

    HAMMER_FORCE_INLINE RefCountedPtr&
    operator=(const RefCountedPtr& other) noexcept {
        if (other.ptr_)
            other.ptr_->inc_ref();
        if (ptr_)
            ptr_->dec_ref();
        ptr_ = other.ptr_;
        return *this;
    }

    HAMMER_FORCE_INLINE RefCountedPtr&
    operator=(RefCountedPtr&& other) noexcept {
        if (this != &other) {
            if (ptr_)
                ptr_->dec_ref();
            ptr_ = std::exchange(other.ptr_, nullptr);
        }
        return *this;
    }

    HAMMER_FORCE_INLINE T* release() noexcept {
        return std::exchange(ptr_, nullptr);
    }

    T* get() const noexcept { return ptr_; }

    HAMMER_FORCE_INLINE T* operator->() const {
        HAMMER_ASSERT(ptr_, "Dereferencing an invalid Ref<T>.");
        return ptr_;
    }

    HAMMER_FORCE_INLINE T& operator*() const {
        HAMMER_ASSERT(ptr_, "Dereferencing an invalid Ref<T>.");
        return *ptr_;
    }

    HAMMER_FORCE_INLINE explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    HAMMER_FORCE_INLINE friend bool
    operator==(const RefCountedPtr& a, const RefCountedPtr& b) noexcept {
        return a.get() == b.get();
    }

    HAMMER_FORCE_INLINE friend bool
    operator!=(const RefCountedPtr& a, const RefCountedPtr& b) noexcept {
        return a.get() != b.get();
    }

    HAMMER_FORCE_INLINE friend bool
    operator==(const RefCountedPtr& a, std::nullptr_t) noexcept {
        return !static_cast<bool>(a);
    }

    HAMMER_FORCE_INLINE friend bool
    operator!=(const RefCountedPtr& a, std::nullptr_t) noexcept {
        return static_cast<bool>(a);
    }

    HAMMER_FORCE_INLINE friend bool
    operator==(std::nullptr_t, const RefCountedPtr& b) noexcept {
        return !static_cast<bool>(b);
    }

    HAMMER_FORCE_INLINE friend bool
    operator!=(std::nullptr_t, const RefCountedPtr& b) noexcept {
        return static_cast<bool>(b);
    }

private:
    T* ptr_;
};

} // namespace detail

template<typename T>
class Ref final : public detail::RefCountedPtr<T> {
public:
    using detail::RefCountedPtr<T>::RefCountedPtr;

    HAMMER_FORCE_INLINE explicit Ref(T* ptr, bool inc_ref = true) noexcept
        : detail::RefCountedPtr<T>(ptr, inc_ref) {
        static_assert(std::is_base_of_v<RefCounted, T>,
            "Type must publicly inherit from RefCounted.");
    }

    template<typename Derived,
        std::enable_if_t<std::is_base_of_v<T, Derived>>* = nullptr>
    HAMMER_FORCE_INLINE Ref(const Ref<Derived>& other) noexcept
        : Ref(other.get()) {}

    HAMMER_FORCE_INLINE operator T*() const noexcept { return this->get(); }
};

template<typename T, typename... Args>
HAMMER_FORCE_INLINE Ref<T> make_ref(Args&&... args) {
    T* obj = new T(std::forward<Args>(args)...);
    return Ref<T>(obj, false);
}

template<typename To, typename From>
HAMMER_FORCE_INLINE Ref<To> static_ref_cast(Ref<From> from) {
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

} // namespace hammer

namespace std {

template<typename T>
struct hash<hammer::Ref<T>> {
    size_t operator()(const hammer::Ref<T>& ref) const noexcept {
        return hash<void*>()(ref.get());
    }
};

} // namespace std

#endif // HAMMER_CORE_REF_COUNTED_HPP
