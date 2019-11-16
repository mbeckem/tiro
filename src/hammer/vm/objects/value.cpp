#include "hammer/vm/objects/value.hpp"

#include "hammer/vm/hash.hpp"
#include "hammer/vm/objects/raw_arrays.hpp"
#include "hammer/vm/objects/small_integer.hpp"

#include "hammer/vm/objects/array.ipp"
#include "hammer/vm/objects/coroutine.ipp"
#include "hammer/vm/objects/function.ipp"
#include "hammer/vm/objects/hash_table.ipp"
#include "hammer/vm/objects/object.ipp"
#include "hammer/vm/objects/string.ipp"

namespace hammer::vm {

std::string_view to_string(ValueType type) {
    switch (type) {
#define HAMMER_VM_TYPE(Name) \
    case ValueType::Name:    \
        return #Name;

#include "hammer/vm/objects/types.inc"
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
    case ValueType::SmallInteger:
    case ValueType::String:
    case ValueType::StringBuilder:
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

    case ValueType::Symbol:
    case ValueType::SpecialValue:
    case ValueType::Code:
    case ValueType::FunctionTemplate:
    case ValueType::ClosureContext:
    case ValueType::Function:
    case ValueType::NativeFunction:
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

#define HAMMER_VM_TYPE(Name) \
    case ValueType::Name:    \
        return Name(v).object_size();

#include "hammer/vm/objects/types.inc"
    }

    HAMMER_UNREACHABLE("Invalid value type.");
}

void finalize(Value v) {
    switch (v.type()) {
    case ValueType::NativeFunction:
        NativeFunction(v).finalize();
        break;

    default:
        break;
    }
}

size_t hash(Value v) {
    switch (v.type()) {
    case ValueType::Null:
    case ValueType::Undefined:
        return 0;
    case ValueType::Boolean:
        return Boolean(v).value() ? 1 : 0;
    case ValueType::Integer:
        return integer_hash(static_cast<u64>(Integer(v).value()));
    case ValueType::Float:
        return float_hash(Float(v).value());
    case ValueType::SmallInteger:
        return integer_hash(static_cast<u64>(SmallInteger(v).value()));
    case ValueType::String:
        return String(v).hash();

    // Anything else is a reference type:
    case ValueType::StringBuilder:
    case ValueType::Symbol:
    case ValueType::SpecialValue:
    case ValueType::Code:
    case ValueType::FunctionTemplate:
    case ValueType::ClosureContext:
    case ValueType::Function:
    case ValueType::NativeFunction:
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
// Equality could be optimized by forcing all small values into SmallIteger instances.
// This way, a type mismatch would also indicate non-equality for integers.
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
        case ValueType::SmallInteger:
            return a.as<Integer>().value()
                   == b.as_strict<SmallInteger>().value();
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
        case ValueType::SmallInteger:
            return a.as<Float>().value() == b.as_strict<SmallInteger>().value();
        default:
            return false;
        }
    }
    case ValueType::SmallInteger: {
        switch (tb) {
        case ValueType::Integer:
            return a.as_strict<SmallInteger>().value()
                   == b.as<Integer>().value();
        case ValueType::Float:
            return a.as_strict<SmallInteger>().value()
                   == b.as<Float>().value(); // TODO correct?
        case ValueType::SmallInteger:
            return a.as_strict<SmallInteger>().value()
                   == b.as_strict<SmallInteger>().value();
        default:
            return false;
        }
    }
    case ValueType::String:
        return tb == ValueType::String && a.as<String>().equal(b.as<String>());
    case ValueType::Symbol:
        return tb == ValueType::Symbol && a.as<Symbol>().equal(b.as<Symbol>());

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
    case ValueType::SmallInteger:
        return std::to_string(SmallInteger(v).value());
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

void to_string(Context& ctx, Handle<StringBuilder> builder, Handle<Value> v) {
    switch (v->type()) {
    case ValueType::Null:
        return builder->append(ctx, "null");
    case ValueType::Undefined:
        return builder->append(ctx, "undefined");
    case ValueType::Boolean:
        return builder->append(
            ctx, v.strict_cast<Boolean>()->value() ? "true" : "false");
    case ValueType::Integer:
        return builder->format(ctx, "{}", v.strict_cast<Integer>()->value());
    case ValueType::Float:
        return builder->format(ctx, "{}", v.strict_cast<Float>()->value());
    case ValueType::SmallInteger:
        return builder->format(
            ctx, "{}", v.strict_cast<SmallInteger>()->value());
    case ValueType::String:
        return builder->append(ctx, v.strict_cast<String>()->view());
    case ValueType::SpecialValue:
        return builder->append(ctx, v.strict_cast<SpecialValue>()->name());

    default:
        return builder->format(
            ctx, "{}@{}", to_string(v->type()), (const void*) v->heap_ptr());
    }
}

#define HAMMER_VM_TYPE(X)                        \
    static_assert(sizeof(X) == sizeof(void*));   \
    static_assert(alignof(X) == alignof(void*)); \
    static_assert(std::is_trivially_destructible_v<X>);

#include "hammer/vm/objects/types.inc"

} // namespace hammer::vm
