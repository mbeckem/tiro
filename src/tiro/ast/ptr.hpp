#ifndef TIRO_AST_PTR_HPP
#define TIRO_AST_PTR_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/core/defs.hpp"

#include <utility>

namespace tiro {

template<typename T>
struct AstPtrDeleter {
    void operator()(const T* ptr) noexcept;
};

/// Place this in the source file of your ast node to implement ast pointer deletion.
#define TIRO_IMPLEMENT_AST_DELETER(Type)                                     \
    template<>                                                               \
    void ::tiro::AstPtrDeleter<Type>::operator()(const Type* ptr) noexcept { \
        delete ptr;                                                          \
    }

/// Unique pointer to an AST node. This wraps std::unique_ptr
/// for better support of incomplete types. Types that use this pointer
/// must impement the deleter function, which forward declared in this file.
template<typename T>
class AstPtr final {
public:
    AstPtr() = default;

    AstPtr(T* ptr) noexcept
        : ptr_(ptr) {}

    AstPtr(AstPtr&&) noexcept = default;

    ~AstPtr() = default;

    AstPtr& operator=(AstPtr&&) noexcept = default;

    T* get() const noexcept { return ptr_.get(); }

    T* operator->() const {
        TIRO_DEBUG_ASSERT(ptr_, "Cannot dereference an invalid pointer.");
        return ptr_.operator->();
    }

    T& operator*() const {
        TIRO_DEBUG_ASSERT(ptr_, "Cannot dereference an invalid pointer.");
        return *ptr_;
    }

    void reset(T* ptr) noexcept { ptr_.reset(ptr); }
    void reset() noexcept { ptr_.reset(); }

    T* release() noexcept { return ptr_.release(); }

private:
    std::unique_ptr<T, AstPtrDeleter<T>> ptr_;
};

template<typename T, typename... Args>
AstPtr<T> make_node(Args&&... args) {
    return AstPtr<T>(new T(std::forward<Args>(args)...));
}

} // namespace tiro

#endif // TIRO_AST_PTR_HPP
