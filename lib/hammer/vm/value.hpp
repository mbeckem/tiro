#ifndef HAMMER_VM_VALUE_HPP
#define HAMMER_VM_VALUE_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/type_traits.hpp"

#include <string_view>

namespace hammer::vm {

class Collector;
class ObjectList;
class Heap;
class Value;

#define HAMMER_HEAP_TYPES(X) \
    X(Null)                  \
    X(Undefined)             \
    X(Boolean)               \
    X(Integer)               \
    X(Float)                 \
    X(String)                \
    X(Code)                  \
    X(FunctionTemplate)      \
    X(Function)              \
    X(Module)                \
    X(Array)                 \
    X(Coroutine)             \
    X(CoroutineStack)

enum class ValueType : u8 {
#define HAMMER_DECLARE_ENUM(Name) Name,

    HAMMER_HEAP_TYPES(HAMMER_DECLARE_ENUM)

#undef HAMMER_DECLARE_ENUM
};

std::string_view to_string(ValueType type);

template<typename Type>
struct MapTypeToValueType : undefined_type {};

template<ValueType type>
struct MapValueTypeToType : undefined_type {};

#define HAMMER_TYPE_MAP(X)                              \
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

HAMMER_HEAP_TYPES(HAMMER_TYPE_MAP)

#undef HAMMER_TYPE_MAP

class Header {
    enum flags : u32 {
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
        HAMMER_ASSERT(class_ != 0, "Invalid type.");
    }

    struct InvalidTag {};

    Header(InvalidTag) {}
};

/**
 * The uniform representation for all values managed by the VM.
 * A value has pointer size and contains either a pointer to some object allocated
 * on the heap or a small integer (without any indirection).
 * 
 * TODO: Implement small integers!
 */
class Value {
public:
    /// Indicates the (intended) absence of a value.
    static Value null() noexcept { return Value(); }

    /// Returns a value that points to the heap-allocated object.
    static Value from_heap(Header* object) noexcept {
        HAMMER_ASSERT_NOT_NULL(object);
        return Value(HeapPointerTag(), object);
    }

    /// Same as Value::null().
    Value() noexcept
        : raw_(0) {
        HAMMER_ASSERT(reinterpret_cast<uintptr_t>(nullptr) == 0,
                      "Invalid null pointer representation.");
    }

    /// Returns true if the value is null.
    bool is_null() const noexcept { return raw_ == 0; }

    /// Returns true if the value is not null.
    explicit operator bool() const { return !is_null(); }

    /// Returns the value type of this value.
    ValueType type() const noexcept {
        if (is_null()) {
            return ValueType::Null;
        } else {
            return static_cast<ValueType>(heap_ptr()->class_);
        }
    }

    /// Returns true if the value is of the specified type.
    template<typename T>
    bool is() const noexcept {
        if constexpr (std::is_same_v<remove_cvref_t<T>, Value>) {
            return true;
        } else {
            using traits = MapTypeToValueType<remove_cvref_t<T>>;
            static_assert(!is_undefined<traits>, "Type is not registered as a value type.");
            static constexpr ValueType type = traits::type;

            // TODO small integer
            if constexpr (type == ValueType::Null) {
                return is_null();
            } else {
                return static_cast<ValueType>(heap_ptr()->class_) == type;
            }
        }
    }

    /// Casts the object to the given type. This casts propagates null values, i.e.
    /// a cast to some heap type "T" will work if the current type is either "T" or Null.
    template<typename T>
    T as() const noexcept {
        // TODO cannot cast small integer this way
        return is_null() ? T() : as_strict<T>();
    }

    /// Like cast, but does not permit null values to propagate. The cast will work only
    /// if the exact type is "T".
    template<typename T>
    T as_strict() const noexcept {
        // TODO put this somewhere else?
        static_assert(sizeof(T) == sizeof(Value), "All derived types must have the same size.");

        HAMMER_ASSERT(is<T>(), "Value is not an instance of this type.");
        return T(*this);
    }

    /// Returns the raw representation of this value.
    uintptr_t raw() const noexcept { return raw_; }

    /// Returns true if this value contains a pointer to the heap.
    bool is_heap_ptr() const noexcept { return (raw_ & 1) == 0; }

    /// Returns the heap pointer stored in this value.
    /// Requires is_heap_ptr() to be true.
    Header* heap_ptr() const noexcept {
        HAMMER_ASSERT(is_heap_ptr(), "Raw value is not a heap pointer.");
        return reinterpret_cast<Header*>(raw_);
    }

    /// Returns the size of this value on the heap.
    size_t object_size() const noexcept;

protected:
    struct HeapPointerTag {};

    explicit Value(HeapPointerTag, Header* ptr) noexcept
        : raw_(reinterpret_cast<uintptr_t>(ptr)) {
        HAMMER_ASSERT((raw_ & 1) == 0, "Heap pointer is not aligned correctly.");
    }

    // Unchecked cast to the inner data object. Must be a derived class of Header.
    // Used by derived classes to access their private data.
    template<typename T>
    T* access_heap() const noexcept {
        HAMMER_ASSERT(is_heap_ptr() && heap_ptr() != nullptr, "Must be a valid heap pointer.");
        static_assert(std::is_base_of_v<Header, T>, "T must be a base class of Header.");
        return static_cast<T*>(heap_ptr());
    }

private:
    uintptr_t raw_;
};

} // namespace hammer::vm

#endif // HAMMER_VM_VALUE_HPP
