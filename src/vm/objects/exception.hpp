#ifndef TIRO_VM_OBJECTS_EXCEPTION_HPP
#define TIRO_VM_OBJECTS_EXCEPTION_HPP

#include "vm/object_support/layout.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

class Exception final : public HeapValue {
private:
    enum Slots { MessageSlot, SlotCount_ };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    static Exception make(Context& ctx, Handle<String> message);

    explicit Exception(Value v)
        : HeapValue(v, DebugCheck<Exception>()) {}

    String message();

    Layout* layout() const { return access_heap<Layout>(); }
};

/// Represents a value that is either a `T` or an exception object.
/// Objects of these type are returned by functions that can fail.
/// If the function failed with an exception, that exception must either be
/// returned or wrapped with another exception and then returned.
///
/// Note that Fallible<T> is not convertible to Value by design, so it must
/// always be checked before using it as a plain value.
template<typename T>
class Fallible final {
    static_assert(!std::is_same_v<T, Exception>, "Fallible<Exception> does not make sense.");

public:
    using ValueType = T;

    /// Constructs a fallible that contains an exception.
    Fallible(Exception ex)
        : is_exception_(true)
        , value_(ex) {}

    /// Constructs a fallible that contains a valid value.
    Fallible(T value)
        : is_exception_(false)
        , value_(value) {}

    bool has_value() const { return !is_exception_; }
    bool has_exception() const { return is_exception_; }
    explicit operator bool() const { return has_value(); }

    T value() const {
        TIRO_DEBUG_ASSERT(has_value(), "Fallible<T> does not contain a value.");
        return static_cast<T>(value_);
    }

    Exception exception() const {
        TIRO_DEBUG_ASSERT(has_exception(), "Fallible<T> does not contain an exception.");
        return static_cast<Exception>(value_);
    }

private:
    bool is_exception_;
    Value value_;
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_EXCEPTION_HPP
