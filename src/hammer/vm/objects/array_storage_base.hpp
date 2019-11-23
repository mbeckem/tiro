#ifndef HAMMER_VM_OBJECTS_ARRAY_STORAGE_BASE_HPP
#define HAMMER_VM_OBJECTS_ARRAY_STORAGE_BASE_HPP

#include "hammer/core/span.hpp"
#include "hammer/vm/objects/value.hpp"

namespace hammer::vm {

/**
 * Provides the underyling storage for array objects that can contain references to
 * other objects. ArrayStorage objects are contiguous in memory. 
 * They consist of an occupied part (from index 0 to size()) and an uninitialized part (from size() to capacity()).
 * 
 * This has the advantage that the garbage collector only has to scan the occupied part, as the uninitialized part
 * is guaranteed not to contain any valid references.
 */
template<typename T, typename Derived>
class ArrayStorageBase : public Value {
private:
    static_assert(std::is_trivially_destructible_v<T>,
        "Must be trivially destructible as destructors are not called.");

    static constexpr ValueType concrete_type =
        MapTypeToValueType<Derived>::type;

    struct Data : Header {
        template<typename Init>
        Data(size_t capacity_, Init&& init)
            : Header(concrete_type)
            , size(0)
            , capacity(capacity_) {
            init(this);
        }

        size_t size;     // The first size values are occupied
        size_t capacity; // Total number of available values
        T values[];
    };

    Data* access_heap() const { return Value::access_heap<Data>(); }

    template<typename Init>
    inline static Derived make_impl(Context& ctx, size_t capacity, Init&& init);

public:
    static Derived make(Context& ctx, size_t capacity) {
        // The storage remains uninitialized!
        return make_impl(ctx, capacity, [](Data*) {});
    }

    static Derived make(Context& ctx,
        /* FIXME rooted */ Span<const T> initial_content, size_t capacity) {
        HAMMER_ASSERT(initial_content.size() <= capacity,
            "ArrayStorageBase::make(): initial content does not fit into the "
            "capacity.");

        // Only the initial_content parts gets initialized.
        return make_impl(ctx, capacity, [&](Data* d) {
            std::uninitialized_copy_n(
                initial_content.data(), initial_content.size(), d->values);
            d->size = initial_content.size();
        });
    }

    ArrayStorageBase() = default;

    explicit ArrayStorageBase(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Derived>(), "Value is of the wrong type.");
    }

    size_t size() const { return access_heap()->size; }
    size_t capacity() const { return access_heap()->capacity; }
    const T* data() const { return access_heap()->values; }
    Span<const T> values() const { return {data(), size()}; }

    bool empty() const {
        Data* d = access_heap();
        HAMMER_ASSERT(d->size <= d->capacity,
            "Size must never be larger than the capacity.");
        return d->size == 0;
    }

    bool full() const {
        Data* d = access_heap();
        HAMMER_ASSERT(d->size <= d->capacity,
            "Size must never be larger than the capacity.");
        return d->size == d->capacity;
    }

    T get(size_t index) const {
        HAMMER_ASSERT(
            index < size(), "ArrayStorageBase::get(): index out of bounds.");
        return access_heap()->values[index];
    }

    void set(size_t index, T value) const {
        HAMMER_ASSERT(
            index < size(), "ArrayStorageBase::set(): index out of bounds.");
        access_heap()->values[index] = value;
    }

    void append(const T& value) const {
        HAMMER_ASSERT(size() < capacity(),
            "ArrayStorageBase::append(): no free capacity remaining.");

        Data* d = access_heap();
        new (d->values + d->size) T(value);
        d->size += 1;
    }

    void clear() const { access_heap()->size = 0; }

    void remove_last() const {
        HAMMER_ASSERT(
            size() > 0, "ArrayStorageBase::remove_last(): storage is empty.");
        access_heap()->size -= 1;
    }

    void remove_last(size_t n) const {
        HAMMER_ASSERT(n <= size(),
            "ArrayStorageBase::remove_last(): cannot remove that many "
            "elements.");
        access_heap()->size -= n;
    }

    inline size_t object_size() const noexcept {
        return sizeof(Data) + capacity() * sizeof(T);
    }

    template<typename W>
    inline void walk(W&& w) {
        Data* d = access_heap();
        w.array(ArrayVisitor(d->values, d->size));
    }
};

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_ARRAY_STORAGE_BASE_HPP
