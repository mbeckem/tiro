#ifndef TIRO_VM_VALUE_HPP
#define TIRO_VM_VALUE_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/math.hpp"
#include "tiro/core/type_traits.hpp"
#include "tiro/vm/fwd.hpp"
#include "tiro/vm/objects/fwd.hpp"

#include <string_view>

namespace tiro::vm {

enum class ValueType : u8 {
#define TIRO_VM_TYPE(Name) Name,
#include "tiro/vm/objects/types.inc"
};

std::string_view to_string(ValueType type);

template<typename Type>
struct MapTypeToValueType : undefined_type {};

template<ValueType type>
struct MapValueTypeToType : undefined_type {};

#define TIRO_VM_TYPE(X)                               \
    class X;                                            \
                                                        \
    template<>                                          \
    struct MapTypeToValueType<X> {                      \
        static constexpr ValueType type = ValueType::X; \
    };                                                  \
                                                        \
    template<>                                          \
    struct MapValueTypeToType<ValueType::X> {           \
        using type = X;                                 \
    };
#include "tiro/vm/objects/types.inc"

class Header {
    enum Flags : u32 {
        FLAG_MARKED = 1 << 0,
    };

    u32 class_ = 0;
    u32 flags_ = 0;

    // FIXME less stupid algorithm (areas of cells; marking bitmaps)
    Header* next = nullptr;

    friend Value;
    friend Heap;
    friend ObjectList;
    friend Collector;

public:
    // TODO more elaborate class field
    Header(ValueType type)
        : class_(static_cast<u32>(type)) {
        TIRO_ASSERT(class_ != 0, "Invalid type.");
    }

    struct InvalidTag {};

    Header(InvalidTag) {}
};

/// The uniform representation for all values managed by the VM.
/// A value has pointer size and contains either a pointer to some object allocated
/// on the heap or a small integer (without any indirection).
///
/// TODO: Implement small integers!
class Value {
public:
    static constexpr uintptr_t embedded_integer_flag = 1;

    // Number of bits to shift integers by to encode/decode them into uintptr_t values.
    static constexpr uintptr_t embedded_integer_shift = 1;

    // Number of available bits for integer storage.
    static constexpr uintptr_t embedded_integer_bits =
        CHAR_BIT * sizeof(uintptr_t) - embedded_integer_shift;

    /// Indicates the (intended) absence of a value.
    static constexpr Value null() noexcept { return Value(); }

    /// Returns a value that points to the heap-allocated object.
    /// The object pointer must not be null.
    static Value from_heap(Header* object) {
        TIRO_ASSERT_NOT_NULL(object);
        return Value(HeapPointerTag(), object);
    }

    static Value from_embedded_integer(uintptr_t raw) {
        return Value(EmbeddedIntegerTag(), raw);
    }

    /// Same as Value::null().
    constexpr Value() noexcept
        : raw_(0) {}

    /// Returns true if the value is null.
    constexpr bool is_null() const noexcept { return raw_ == 0; }

    /// Returns true if the value is not null.
    constexpr explicit operator bool() const { return !is_null(); }

    /// Returns the value type of this value.
    ValueType type() const {
        if (is_null()) {
            return ValueType::Null;
        } else if (is_embedded_integer()) {
            return ValueType::SmallInteger;
        } else {
            return static_cast<ValueType>(heap_ptr()->class_);
        }
    }

    /// Returns true if the value is of the specified type.
    template<typename T>
    bool is() const {
        if constexpr (std::is_same_v<remove_cvref_t<T>, Value>) {
            return true;
        } else {
            using traits = MapTypeToValueType<remove_cvref_t<T>>;
            static_assert(!is_undefined<traits>,
                "Type is not registered as a value type.");
            static constexpr ValueType type = traits::type;

            if constexpr (type == ValueType::Null) {
                return is_null();
            } else if constexpr (type == ValueType::SmallInteger) {
                return is_embedded_integer();
            } else {
                return !is_null() && is_heap_ptr()
                       && static_cast<ValueType>(heap_ptr()->class_) == type;
            }
        }
    }

    /// Casts the object to the given type. This casts propagates null values, i.e.
    /// a cast to some heap type "T" will work if the current type is either "T" or Null.
    /// FIXME remove nulls
    template<typename T>
    T as() const {
        return is_null() ? T() : as_strict<T>();
    }

    /// Like cast, but does not permit null values to propagate. The cast will work only
    /// if the exact type is "T".
    template<typename T>
    T as_strict() const {
        // TODO put this somewhere else?
        static_assert(sizeof(T) == sizeof(Value),
            "All derived types must have the same size.");

        TIRO_ASSERT(is<T>(), "Value is not an instance of this type.");
        return T(*this);
    }

    /// Returns the raw representation of this value.
    uintptr_t raw() const noexcept { return raw_; }

    /// Returns true if this value contains a pointer to the heap.
    /// Note: the pointer may still be NULL.
    bool is_heap_ptr() const noexcept {
        return (raw_ & embedded_integer_flag) == 0;
    }

    /// Returns true if this value contains an embedded integer.
    bool is_embedded_integer() const noexcept {
        return (raw_ & embedded_integer_flag) != 0;
    }

    /// Returns the heap pointer stored in this value.
    /// Requires is_heap_ptr() to be true.
    Header* heap_ptr() const {
        TIRO_ASSERT(is_heap_ptr(), "Raw value is not a heap pointer.");
        return reinterpret_cast<Header*>(raw_);
    }

    /// True if these are the same objects/values.
    bool same(const Value& other) const noexcept { return raw_ == other.raw_; }

protected:
    struct HeapPointerTag {};
    struct EmbeddedIntegerTag {};

    explicit Value(HeapPointerTag, Header* ptr)
        : raw_(reinterpret_cast<uintptr_t>(ptr)) {
        TIRO_ASSERT((raw_ & embedded_integer_flag) == 0,
            "Heap pointer is not aligned correctly.");
    }

    explicit Value(EmbeddedIntegerTag, uintptr_t value)
        : raw_(value) {
        TIRO_ASSERT(raw_ & embedded_integer_flag,
            "Value does not represent an embedded integer.");
    }

    // Unchecked cast to the inner data object. Must be a derived class of Header.
    // Used by derived classes to access their private data.
    template<typename T>
    T* access_heap() const {
        TIRO_ASSERT(is_heap_ptr() && heap_ptr() != nullptr,
            "Must be a valid heap pointer.");
        static_assert(
            std::is_base_of_v<Header, T>, "T must be a base class of Header.");
        return static_cast<T*>(heap_ptr());
    }

private:
    uintptr_t raw_;
};

// TODO move it
template<typename BaseType, typename ValueType>
constexpr size_t variable_allocation(size_t values) {
    // TODO these should be language level errors.

    size_t trailer = 0;
    if (TIRO_UNLIKELY(!checked_mul(sizeof(ValueType), values, trailer)))
        TIRO_ERROR("Allocation size overflow.");

    size_t total = 0;
    if (TIRO_UNLIKELY(!checked_add(sizeof(BaseType), trailer, total)))
        TIRO_ERROR("Allocation size overflow.");

    return total;
}

/// This class is used when the garbage collector
/// visits the individual elements of an array-like object.
/// The visitor keeps track of the current position in the large array.
/// With this approach, we don't have to push the entire array's contents
/// on the marking stack at once.
// TODO: put somewhere else
template<typename T>
class ArrayVisitor {
public:
    ArrayVisitor(T* begin_, T* end_)
        : next(begin_)
        , end(end_) {}

    ArrayVisitor(T* begin_, size_t size)
        : next(begin_)
        , end(begin_ + size) {}

    bool has_item() const { return next != end; }

    size_t remaining() const { return static_cast<size_t>(end - next); }

    T& get_item() const {
        TIRO_ASSERT(has_item(), "ArrayVisitor is at the end.");
        return *next;
    }

    void advance() {
        TIRO_ASSERT(has_item(), "Array visitor is at the end.");
        ++next;
    }

private:
    T* next;
    T* end;
};

/// True iff objects of the given type might contain references.
bool may_contain_references(ValueType type);

/// Returns the size of this value on the heap, in bytes.
size_t object_size(Value v);

/// Finalizes the object (calls destructors for native objects).
/// FIXME: A bit in the header or a common base class should indicate
/// which values must be finalized. Only finalizable objects should
/// be visited by the gc for cleanup.
void finalize(Value v);

/// Returns the hash value of `v`.
/// For two values a and b, equal(a, b) implies hash(a) == hash(b).
/// Equal hash values DO NOT imply equality.
size_t hash(Value v);

/// Returns true if a is equal to b, as defined by the language's equality rules.
bool equal(Value a, Value b);

/// Format the value as a string. For debug only.
std::string to_string(Value v);

/// Appends a string representation of the given value to the provided builder.
void to_string(Context& ctx, Handle<StringBuilder> builder, Handle<Value> v);

} // namespace tiro::vm

#endif // TIRO_VM_VALUE_HPP
