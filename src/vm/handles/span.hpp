#ifndef TIRO_VM_HANDLES_SPAN_HPP
#define TIRO_VM_HANDLES_SPAN_HPP

#include "common/span.hpp"
#include "vm/handles/fwd.hpp"
#include "vm/handles/handle.hpp"

namespace tiro::vm {

namespace detail {

// Iterates over slots and maps the slot pointer to a handle instance.
template<typename Handle>
class HandleSpanIterator final {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = Handle;
    using difference_type = std::ptrdiff_t;
    using pointer = Handle*;
    using reference = Handle;

    explicit HandleSpanIterator()
        : slot_(nullptr) {}

    explicit HandleSpanIterator(Value* slot)
        : slot_(slot) {}

    Handle operator*() const { return Handle::from_raw_slot(slot_); }

    Handle operator->() const { return this->operator*(); }

    HandleSpanIterator& operator++() {
        ++slot_;
        return *this;
    }

    HandleSpanIterator& operator++(int) {
        auto old = *this;
        ++slot_;
        return old;
    }

    friend bool operator==(const HandleSpanIterator& lhs, const HandleSpanIterator& rhs) {
        return lhs.slot_ == rhs.slot_;
    }

    friend bool operator!=(const HandleSpanIterator& lhs, const HandleSpanIterator& rhs) {
        return !(lhs == rhs);
    }

private:
    Value* slot_;
};

// Base class for handle spans. Accessing an element of the span returns a handle of the specified type.
template<typename T, typename HandleType, typename Derived>
class HandleSpanBase {
public:
    using value_type = T;
    using handle_type = HandleType;
    using iterator = HandleSpanIterator<HandleType>;

    static Derived from_raw_slots(Span<Value> slots) { return Derived(slots, InternalTag()); }

    HandleSpanBase() = default;

    explicit HandleSpanBase(Span<T> slots)
        // Note: conversion from T to Value is safe because T is a sub type of Value and
        // has the same size as Value.
        : HandleSpanBase(Span<Value>(slots.data(), slots.size()), InternalTag()) {}

    iterator begin() const { return iterator(slots_.begin()); }
    iterator end() const { return iterator(slots_.end()); }

    bool empty() const { return size() == 0; }
    size_t size() const { return slots_.size(); }
    HandleType operator[](size_t index) const { return HandleType::from_raw_slot(&slots_[index]); }

protected:
    struct InternalTag {};

    explicit HandleSpanBase(Span<Value> slots, InternalTag)
        : slots_(slots) {}

private:
    friend SlotAccess;

    Span<Value> get_slots() const { return slots_; }

protected:
    Span<Value> slots_;
};

// Enables implicit conversion from Derived to SpanTemplate<To> if T is
// implicitly convertible to To.
template<typename T, template<typename To> typename SpanTemplate, typename Derived>
class HandleSpanConversion {
public:
    template<typename To, std::enable_if_t<std::is_convertible_v<T, To>>* = nullptr>
    operator SpanTemplate<To>() const {
        return SpanTemplate<To>::from_raw_slots(get_slots(derived()));
    }

private:
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

} // namespace detail

/// Provides typed read access to a span of rooted values.
/// HandleSpans are implicitly convertible to parent types.
template<typename T>
class HandleSpan final : public detail::HandleSpanBase<T, Handle<T>, HandleSpan<T>>,
                         public detail::HandleSpanConversion<T, HandleSpan, HandleSpan<T>> {
public:
    using detail::HandleSpanBase<T, Handle<T>, HandleSpan<T>>::HandleSpanBase;

    Span<Value> raw_slots() const { return get_slots(*this); }
};

/// Provides read and write access to a span of rooted values.
/// MutHandleSpans are implicitly convertible to their immutable counterpart.
/// They are however *not* implicitly convertible to parent types in order to guarantee type safety.
template<typename T>
class MutHandleSpan final : public detail::HandleSpanBase<T, MutHandle<T>, MutHandleSpan<T>>,
                            public detail::HandleSpanConversion<T, HandleSpan, MutHandleSpan<T>> {
public:
    using detail::HandleSpanBase<T, MutHandle<T>, MutHandleSpan<T>>::HandleSpanBase;

    Span<Value> raw_slots() const { return get_slots(*this); }
};

} // namespace tiro::vm

#endif // TIRO_VM_HANDLES_SPAN_HPP
