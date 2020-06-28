#include "vm/objects/value.hpp"

#include "vm/hash.hpp"
#include "vm/objects/all.hpp"

#include <type_traits>

namespace tiro::vm {

// Only heap values can point to other objects.
// Some heap values (e.g. strings or byte buffers) never point to other objects
// and need not be traced.
// Their layout has the concrete information.
template<typename T>
bool may_contain_references_impl() {
    if constexpr (std::is_base_of_v<HeapValue, T>) {
        using Layout = typename T::Layout;
        return LayoutTraits<Layout>::may_contain_references;
    } else {
        return false;
    }
}

ValueType Value::type() const {
    if (is_null()) {
        return ValueType::Null;
    } else if (is_embedded_integer()) {
        return ValueType::SmallInteger;
    } else {
        auto self = heap_ptr();
        auto type = heap_ptr()->type();
        TIRO_DEBUG_ASSERT(type, "Object header does not point to a valid type.");

        // Avoid infinite recursion for the root type.
        return self == type ? ValueType::InternalType
                            : InternalType(Value::from_heap(type)).builtin_type();
    }
}

bool may_contain_references(ValueType type) {
    switch (type) {
#define TIRO_CASE(Type)   \
    case TypeToTag<Type>: \
        return may_contain_references_impl<Type>();

        /* [[[cog
            from cog import outl
            from codegen.objects import VM_OBJECTS
            for object in VM_OBJECTS:
                outl(f"TIRO_CASE({object.type_name})")
        ]]] */
        TIRO_CASE(Array)
        TIRO_CASE(ArrayStorage)
        TIRO_CASE(Boolean)
        TIRO_CASE(BoundMethod)
        TIRO_CASE(Buffer)
        TIRO_CASE(Code)
        TIRO_CASE(Coroutine)
        TIRO_CASE(CoroutineStack)
        TIRO_CASE(DynamicObject)
        TIRO_CASE(Environment)
        TIRO_CASE(Float)
        TIRO_CASE(Function)
        TIRO_CASE(FunctionTemplate)
        TIRO_CASE(HashTable)
        TIRO_CASE(HashTableIterator)
        TIRO_CASE(HashTableStorage)
        TIRO_CASE(Integer)
        TIRO_CASE(InternalType)
        TIRO_CASE(Method)
        TIRO_CASE(Module)
        TIRO_CASE(NativeAsyncFunction)
        TIRO_CASE(NativeFunction)
        TIRO_CASE(NativeObject)
        TIRO_CASE(NativePointer)
        TIRO_CASE(Null)
        TIRO_CASE(SmallInteger)
        TIRO_CASE(String)
        TIRO_CASE(StringBuilder)
        TIRO_CASE(Symbol)
        TIRO_CASE(Tuple)
        TIRO_CASE(Type)
        TIRO_CASE(Undefined)
        // [[[end]]]

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid value type.");
}

size_t object_size(Value v) {
    return v.is_heap_ptr() ? object_size(v.heap_ptr()) : 0;
}

void finalize(Value v) {
    switch (v.type()) {
    case ValueType::NativeObject:
        NativeObject(v).finalize();
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
    case ValueType::Array:
    case ValueType::ArrayStorage:
    case ValueType::BoundMethod:
    case ValueType::Buffer:
    case ValueType::Code:
    case ValueType::Coroutine:
    case ValueType::CoroutineStack:
    case ValueType::DynamicObject:
    case ValueType::Environment:
    case ValueType::Function:
    case ValueType::FunctionTemplate:
    case ValueType::HashTable:
    case ValueType::HashTableIterator:
    case ValueType::HashTableStorage:
    case ValueType::InternalType:
    case ValueType::Method:
    case ValueType::Module:
    case ValueType::NativeAsyncFunction:
    case ValueType::NativeFunction:
    case ValueType::NativeObject:
    case ValueType::NativePointer:
    case ValueType::StringBuilder:
    case ValueType::Symbol:
    case ValueType::Tuple:
    case ValueType::Type:
        // TODO: MUST update once we have moving gc, the heap addr will NOT
        // remain stable!
        // Stable hash codes: https://stackoverflow.com/a/3796963
        return static_cast<size_t>(reinterpret_cast<uintptr_t>(v.heap_ptr()));
    }

    TIRO_UNREACHABLE("Invalid value type.");
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
            return a.as<Integer>().value() == b.as<Float>().value(); // TODO correct?
        case ValueType::SmallInteger:
            return a.as<Integer>().value() == b.as_strict<SmallInteger>().value();
        default:
            return false;
        }
    }
    case ValueType::Float: {
        switch (tb) {
        case ValueType::Integer:
            return a.as<Float>().value() == b.as<Integer>().value(); // TODO correct?
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
            return a.as_strict<SmallInteger>().value() == b.as<Integer>().value();
        case ValueType::Float:
            return a.as_strict<SmallInteger>().value() == b.as<Float>().value(); // TODO correct?
        case ValueType::SmallInteger:
            return a.as_strict<SmallInteger>().value() == b.as_strict<SmallInteger>().value();
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

    // Heap types
    default:
        return fmt::format("{}@{}", to_string(v.type()), (const void*) v.heap_ptr());
    }
}

void to_string(Context& ctx, Handle<StringBuilder> builder, Handle<Value> v) {
    switch (v->type()) {
    case ValueType::Null:
        return builder->append(ctx, "null");
    case ValueType::Undefined:
        return builder->append(ctx, "undefined");
    case ValueType::Boolean:
        return builder->append(ctx, v.strict_cast<Boolean>()->value() ? "true" : "false");
    case ValueType::Integer:
        return builder->format(ctx, "{}", v.strict_cast<Integer>()->value());
    case ValueType::Float:
        return builder->format(ctx, "{}", v.strict_cast<Float>()->value());
    case ValueType::SmallInteger:
        return builder->format(ctx, "{}", v.strict_cast<SmallInteger>()->value());
    case ValueType::String:
        return builder->append(ctx, v.strict_cast<String>());
    case ValueType::Symbol: {
        Root name(ctx, v.strict_cast<Symbol>()->name());
        builder->append(ctx, "#");
        builder->append(ctx, name);
        break;
    }
    default:
        return builder->format(ctx, "{}@{}", to_string(v->type()), (const void*) v->heap_ptr());
    }
}

#define TIRO_CHECK_VM_TYPE(X)                           \
    static_assert(sizeof(X) == sizeof(void*));          \
    static_assert(alignof(X) == alignof(void*));        \
    static_assert(std::is_trivially_destructible_v<X>); \
    static_assert(std::is_final_v<X>);

/* [[[cog
    from cog import outl
    from codegen.objects import VM_OBJECTS
    for object in VM_OBJECTS:
        outl(f"TIRO_CHECK_VM_TYPE({object.type_name})")
]]] */
TIRO_CHECK_VM_TYPE(Array)
TIRO_CHECK_VM_TYPE(ArrayStorage)
TIRO_CHECK_VM_TYPE(Boolean)
TIRO_CHECK_VM_TYPE(BoundMethod)
TIRO_CHECK_VM_TYPE(Buffer)
TIRO_CHECK_VM_TYPE(Code)
TIRO_CHECK_VM_TYPE(Coroutine)
TIRO_CHECK_VM_TYPE(CoroutineStack)
TIRO_CHECK_VM_TYPE(DynamicObject)
TIRO_CHECK_VM_TYPE(Environment)
TIRO_CHECK_VM_TYPE(Float)
TIRO_CHECK_VM_TYPE(Function)
TIRO_CHECK_VM_TYPE(FunctionTemplate)
TIRO_CHECK_VM_TYPE(HashTable)
TIRO_CHECK_VM_TYPE(HashTableIterator)
TIRO_CHECK_VM_TYPE(HashTableStorage)
TIRO_CHECK_VM_TYPE(Integer)
TIRO_CHECK_VM_TYPE(InternalType)
TIRO_CHECK_VM_TYPE(Method)
TIRO_CHECK_VM_TYPE(Module)
TIRO_CHECK_VM_TYPE(NativeAsyncFunction)
TIRO_CHECK_VM_TYPE(NativeFunction)
TIRO_CHECK_VM_TYPE(NativeObject)
TIRO_CHECK_VM_TYPE(NativePointer)
TIRO_CHECK_VM_TYPE(Null)
TIRO_CHECK_VM_TYPE(SmallInteger)
TIRO_CHECK_VM_TYPE(String)
TIRO_CHECK_VM_TYPE(StringBuilder)
TIRO_CHECK_VM_TYPE(Symbol)
TIRO_CHECK_VM_TYPE(Tuple)
TIRO_CHECK_VM_TYPE(Type)
TIRO_CHECK_VM_TYPE(Undefined)
// [[[end]]]

#undef TIRO_CHECK_VM_TYPE

} // namespace tiro::vm
