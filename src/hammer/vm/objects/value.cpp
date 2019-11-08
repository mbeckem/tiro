#include "hammer/vm/objects/value.hpp"

#include "hammer/vm/objects/raw_arrays.hpp"

#include "hammer/vm/objects/array.ipp"
#include "hammer/vm/objects/coroutine.ipp"
#include "hammer/vm/objects/function.ipp"
#include "hammer/vm/objects/hash_table.ipp"
#include "hammer/vm/objects/object.ipp"

namespace hammer::vm {

std::string_view to_string(ValueType type) {
    switch (type) {
#define TYPE_TO_STR(Name) \
    case ValueType::Name: \
        return #Name;

        HAMMER_HEAP_TYPES(TYPE_TO_STR)

#undef TYPE_TO_STR
    }

    HAMMER_UNREACHABLE("Invalid value type.");
}

bool may_contain_references(ValueType type) {
    switch (type) {
    case ValueType::Null:
    case ValueType::Undefined:
    case ValueType::Boolean:
    case ValueType::Integer:
    case ValueType::Float:
    case ValueType::String:
    case ValueType::U8Array:
    case ValueType::U16Array:
    case ValueType::U32Array:
    case ValueType::U64Array:
    case ValueType::I8Array:
    case ValueType::I16Array:
    case ValueType::I32Array:
    case ValueType::I64Array:
    case ValueType::F32Array:
    case ValueType::F64Array:
        return false;

    case ValueType::SpecialValue:
    case ValueType::Code:
    case ValueType::FunctionTemplate:
    case ValueType::ClosureContext:
    case ValueType::Function:
    case ValueType::Module:
    case ValueType::Tuple:
    case ValueType::Array:
    case ValueType::ArrayStorage:
    case ValueType::HashTable:
    case ValueType::HashTableStorage:
    case ValueType::HashTableIterator:
    case ValueType::Coroutine:
    case ValueType::CoroutineStack:
        return true;
    }

    HAMMER_UNREACHABLE("Invalid value type.");
}

size_t object_size(Value v) {
    switch (v.type()) {

#define HANDLE_HEAP_TYPE(Name) \
    case ValueType::Name:      \
        return Name(v).object_size();

        HAMMER_HEAP_TYPES(HANDLE_HEAP_TYPE)

#undef HANDLE_HEAP_TYPE
    }

    HAMMER_UNREACHABLE("Invalid value type.");
}

size_t hash(Value v) {
    // FIXME need a better hash function for integers and floats,
    // the std one is terrible.

    switch (v.type()) {
    case ValueType::Null:
    case ValueType::Undefined:
        return 0;
    case ValueType::Boolean:
        return Boolean(v).value() ? 1 : 0;
    case ValueType::Integer:
        return std::hash<i64>()(Integer(v).value());
    case ValueType::Float:
        return std::hash<f64>()(Float(v).value()); /* ugh */
    case ValueType::String:
        return String(v).hash();

    // Anything else is a reference type:
    case ValueType::SpecialValue:
    case ValueType::Code:
    case ValueType::FunctionTemplate:
    case ValueType::ClosureContext:
    case ValueType::Function:
    case ValueType::Module:
    case ValueType::Tuple:
    case ValueType::Array:
    case ValueType::ArrayStorage:
    case ValueType::U8Array:
    case ValueType::U16Array:
    case ValueType::U32Array:
    case ValueType::U64Array:
    case ValueType::I8Array:
    case ValueType::I16Array:
    case ValueType::I32Array:
    case ValueType::I64Array:
    case ValueType::F32Array:
    case ValueType::F64Array:
    case ValueType::HashTable:
    case ValueType::HashTableStorage:
    case ValueType::HashTableIterator:
    case ValueType::Coroutine:
    case ValueType::CoroutineStack:
        // TODO: MUST update once we have moving gc, the heap addr will NOT
        // remain stable!
        // Stable hash codes: https://stackoverflow.com/a/3796963
        return static_cast<size_t>(reinterpret_cast<uintptr_t>(v.heap_ptr()));
    }

    HAMMER_UNREACHABLE("Invalid value type.");
}

// TODO think about float / integer equality.
bool equal(Value a, Value b) {
    const ValueType ta = a.type();
    const ValueType tb = b.type();

    switch (ta) {
    case ValueType::Null:
        return tb == ValueType::Null;
    case ValueType::Undefined:
        return tb == ValueType::Undefined;
    case ValueType::Boolean: {
        switch (tb) {
        case ValueType::Boolean:
            return a.as<Boolean>().value() == b.as<Boolean>().value();
        default:
            return false;
        }
    }
    case ValueType::Integer: {
        switch (tb) {
        case ValueType::Integer:
            return a.as<Integer>().value() == b.as<Integer>().value();
        case ValueType::Float:
            return a.as<Integer>().value()
                   == b.as<Float>().value(); // TODO correct?
        default:
            return false;
        }
    }
    case ValueType::Float: {
        switch (tb) {
        case ValueType::Integer:
            return a.as<Float>().value()
                   == b.as<Integer>().value(); // TODO correct?
        case ValueType::Float:
            return a.as<Float>().value() == b.as<Float>().value();
        default:
            return false;
        }
    }
    case ValueType::String:
        return tb == ValueType::String && a.as<String>().equal(b.as<String>());

    // Reference semantics
    default:
        return ta == tb && a.heap_ptr() == b.heap_ptr();
    }
}

std::string to_string(Value v) {
    switch (v.type()) {
    case ValueType::Null:
        return "null";
    case ValueType::Undefined:
        return "undefined";
    case ValueType::Boolean:
        return Boolean(v).value() ? "true" : "false";
    case ValueType::Integer:
        return std::to_string(Integer(v).value());
    case ValueType::Float:
        return std::to_string(Float(v).value());
    case ValueType::String:
        return std::string(String(v).view());
    case ValueType::SpecialValue:
        return std::string(SpecialValue(v).name());

    // Heap types
    default:
        return fmt::format(
            "{}@{}", to_string(v.type()), (const void*) v.heap_ptr());
    }
}

#define HAMMER_CHECK_TYPE_PROPS(X)               \
    static_assert(sizeof(X) == sizeof(void*));   \
    static_assert(alignof(X) == alignof(void*)); \
    static_assert(std::is_trivially_destructible_v<X>);

HAMMER_CHECK_TYPE_PROPS(Value)
HAMMER_HEAP_TYPES(HAMMER_CHECK_TYPE_PROPS)

#undef HAMMER_CHECK_TYPE_PROPS

} // namespace hammer::vm
