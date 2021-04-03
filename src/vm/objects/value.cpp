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
    switch (category()) {
    case ValueCategory::Null:
        return ValueType::Null;
    case ValueCategory::EmbeddedInteger:
        return ValueType::SmallInteger;
    case ValueCategory::Heap: {
        auto type = HeapValue(*this).type_instance();
        return type.builtin_type();
    }
    }

    TIRO_UNREACHABLE("Invalid value category.");
}

InternalType HeapValue::type_instance() {
    auto self = heap_ptr();
    auto type = self->type();
    TIRO_DEBUG_ASSERT(type, "Object header does not point to a valid type.");

    // Avoid infinite recursion in debug mode type checks for the root type.
    return self == type ? InternalType(self, InternalType::Unchecked())
                        : InternalType(Value::from_heap(type));
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
        TIRO_CASE(ArrayIterator)
        TIRO_CASE(ArrayStorage)
        TIRO_CASE(Boolean)
        TIRO_CASE(BoundMethod)
        TIRO_CASE(Buffer)
        TIRO_CASE(Code)
        TIRO_CASE(Coroutine)
        TIRO_CASE(CoroutineStack)
        TIRO_CASE(CoroutineToken)
        TIRO_CASE(Environment)
        TIRO_CASE(Exception)
        TIRO_CASE(Float)
        TIRO_CASE(Function)
        TIRO_CASE(FunctionTemplate)
        TIRO_CASE(HandlerTable)
        TIRO_CASE(HashTable)
        TIRO_CASE(HashTableIterator)
        TIRO_CASE(HashTableKeyIterator)
        TIRO_CASE(HashTableKeyView)
        TIRO_CASE(HashTableStorage)
        TIRO_CASE(HashTableValueIterator)
        TIRO_CASE(HashTableValueView)
        TIRO_CASE(Integer)
        TIRO_CASE(InternalType)
        TIRO_CASE(MagicFunction)
        TIRO_CASE(Method)
        TIRO_CASE(Module)
        TIRO_CASE(NativeFunction)
        TIRO_CASE(NativeObject)
        TIRO_CASE(NativePointer)
        TIRO_CASE(Null)
        TIRO_CASE(Record)
        TIRO_CASE(RecordTemplate)
        TIRO_CASE(Result)
        TIRO_CASE(Set)
        TIRO_CASE(SetIterator)
        TIRO_CASE(SmallInteger)
        TIRO_CASE(String)
        TIRO_CASE(StringBuilder)
        TIRO_CASE(StringIterator)
        TIRO_CASE(StringSlice)
        TIRO_CASE(Symbol)
        TIRO_CASE(Tuple)
        TIRO_CASE(TupleIterator)
        TIRO_CASE(Type)
        TIRO_CASE(Undefined)
        TIRO_CASE(UnresolvedImport)
        // [[[end]]]

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid value type.");
}

size_t object_size(Value v) {
    return v.is_heap_ptr() ? object_size(HeapValue(v).heap_ptr()) : 0;
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
    case ValueType::StringSlice:
        return StringSlice(v).hash();

    // Anything else is a reference type:
    case ValueType::Array:
    case ValueType::ArrayIterator:
    case ValueType::ArrayStorage:
    case ValueType::BoundMethod:
    case ValueType::Buffer:
    case ValueType::Code:
    case ValueType::Coroutine:
    case ValueType::CoroutineStack:
    case ValueType::CoroutineToken:
    case ValueType::Environment:
    case ValueType::Function:
    case ValueType::FunctionTemplate:
    case ValueType::Exception:
    case ValueType::HandlerTable:
    case ValueType::HashTable:
    case ValueType::HashTableKeyView:
    case ValueType::HashTableValueView:
    case ValueType::HashTableIterator:
    case ValueType::HashTableKeyIterator:
    case ValueType::HashTableValueIterator:
    case ValueType::HashTableStorage:
    case ValueType::InternalType:
    case ValueType::MagicFunction:
    case ValueType::Method:
    case ValueType::Module:
    case ValueType::NativeFunction:
    case ValueType::NativeObject:
    case ValueType::NativePointer:
    case ValueType::Record:
    case ValueType::RecordTemplate:
    case ValueType::Result:
    case ValueType::Set:
    case ValueType::SetIterator:
    case ValueType::StringBuilder:
    case ValueType::StringIterator:
    case ValueType::Symbol:
    case ValueType::Tuple:
    case ValueType::TupleIterator:
    case ValueType::Type:
    case ValueType::UnresolvedImport:
        // TODO: MUST update once we have moving gc, the heap addr will NOT
        // remain stable!
        // Stable hash codes: https://stackoverflow.com/a/3796963
        return static_cast<size_t>(reinterpret_cast<uintptr_t>(HeapValue(v).heap_ptr()));
    }

    TIRO_UNREACHABLE("Invalid value type.");
}

static bool int_float_equal(i64 lhs, f64 rhs) {
    if (!std::isfinite(rhs))
        return false;

    // Check whether converting the float to int and back preserves the value.
    // If that is the case, the two integers can be equal.
    i64 rhs_int = rhs;
    f64 rhs_roundtrip = rhs_int;
    return rhs_roundtrip == rhs && lhs == rhs_int;
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
            return a.must_cast<Boolean>().value() == b.must_cast<Boolean>().value();
        default:
            return false;
        }
    }
    case ValueType::SmallInteger: {
        auto ai = a.must_cast<SmallInteger>();
        switch (tb) {
        case ValueType::SmallInteger:
            return ai.value() == b.must_cast<SmallInteger>().value();
        case ValueType::Integer:
            return ai.value() == b.must_cast<Integer>().value();
        case ValueType::Float:
            return int_float_equal(ai.value(), b.must_cast<Float>().value());
        default:
            return false;
        }
    }
    case ValueType::Integer: {
        auto ai = a.must_cast<Integer>();
        switch (tb) {
        case ValueType::SmallInteger:
            return ai.value() == b.must_cast<SmallInteger>().value();
        case ValueType::Integer:
            return ai.value() == b.must_cast<Integer>().value();
        case ValueType::Float:
            return int_float_equal(ai.value(), b.must_cast<Float>().value());
        default:
            return false;
        }
    }
    case ValueType::Float: {
        auto af = a.must_cast<Float>();
        switch (tb) {
        case ValueType::SmallInteger:
            return int_float_equal(b.must_cast<SmallInteger>().value(), af.value());
        case ValueType::Integer:
            return int_float_equal(b.must_cast<Integer>().value(), af.value());
        case ValueType::Float:
            return af.value() == b.must_cast<Float>().value();
        default:
            return false;
        }
    }
    case ValueType::String:
        return a.must_cast<String>().equal(b);
    case ValueType::StringSlice:
        return a.must_cast<StringSlice>().equal(b);
    case ValueType::Symbol:
        return tb == ValueType::Symbol && a.must_cast<Symbol>().equal(b.must_cast<Symbol>());

    // Reference semantics
    default:
        return a.same(b);
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
    case ValueType::StringSlice:
        return std::string(StringSlice(v).view());
    case ValueType::Symbol:
        return fmt::format("#{}", Symbol(v).name().view());
    case ValueType::Exception:
        return fmt::format("Exception: {}", Exception(v).message().view());

    // Heap types
    default:
        return fmt::format("{}@{}", to_string(v.type()), (const void*) HeapValue(v).heap_ptr());
    }
}

void to_string(Context& ctx, Handle<StringBuilder> builder, Handle<Value> v) {
    switch (v->type()) {
    case ValueType::Null:
        return builder->append(ctx, "null");
    case ValueType::Undefined:
        return builder->append(ctx, "undefined");
    case ValueType::Boolean:
        return builder->append(ctx, v.must_cast<Boolean>()->value() ? "true" : "false");
    case ValueType::Integer:
        return builder->format(ctx, "{}", v.must_cast<Integer>()->value());
    case ValueType::Float:
        return builder->format(ctx, "{}", v.must_cast<Float>()->value());
    case ValueType::SmallInteger:
        return builder->format(ctx, "{}", v.must_cast<SmallInteger>()->value());
    case ValueType::String:
        return builder->append(ctx, v.must_cast<String>());
    case ValueType::StringSlice:
        return builder->append(ctx, v.must_cast<StringSlice>());
    case ValueType::Symbol: {
        Scope sc(ctx);
        Local name = sc.local(v.must_cast<Symbol>()->name());
        builder->append(ctx, "#");
        builder->append(ctx, name);
        break;
    }
    case ValueType::Exception: {
        Scope sc(ctx);
        Local message = sc.local(v.must_cast<Exception>()->message());
        builder->append(ctx, "Exception: ");
        builder->append(ctx, message);
        break;
    }
    default:
        return builder->format(
            ctx, "{}@{}", to_string(v->type()), (const void*) v.must_cast<HeapValue>()->heap_ptr());
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
TIRO_CHECK_VM_TYPE(ArrayIterator)
TIRO_CHECK_VM_TYPE(ArrayStorage)
TIRO_CHECK_VM_TYPE(Boolean)
TIRO_CHECK_VM_TYPE(BoundMethod)
TIRO_CHECK_VM_TYPE(Buffer)
TIRO_CHECK_VM_TYPE(Code)
TIRO_CHECK_VM_TYPE(Coroutine)
TIRO_CHECK_VM_TYPE(CoroutineStack)
TIRO_CHECK_VM_TYPE(CoroutineToken)
TIRO_CHECK_VM_TYPE(Environment)
TIRO_CHECK_VM_TYPE(Exception)
TIRO_CHECK_VM_TYPE(Float)
TIRO_CHECK_VM_TYPE(Function)
TIRO_CHECK_VM_TYPE(FunctionTemplate)
TIRO_CHECK_VM_TYPE(HandlerTable)
TIRO_CHECK_VM_TYPE(HashTable)
TIRO_CHECK_VM_TYPE(HashTableIterator)
TIRO_CHECK_VM_TYPE(HashTableKeyIterator)
TIRO_CHECK_VM_TYPE(HashTableKeyView)
TIRO_CHECK_VM_TYPE(HashTableStorage)
TIRO_CHECK_VM_TYPE(HashTableValueIterator)
TIRO_CHECK_VM_TYPE(HashTableValueView)
TIRO_CHECK_VM_TYPE(Integer)
TIRO_CHECK_VM_TYPE(InternalType)
TIRO_CHECK_VM_TYPE(MagicFunction)
TIRO_CHECK_VM_TYPE(Method)
TIRO_CHECK_VM_TYPE(Module)
TIRO_CHECK_VM_TYPE(NativeFunction)
TIRO_CHECK_VM_TYPE(NativeObject)
TIRO_CHECK_VM_TYPE(NativePointer)
TIRO_CHECK_VM_TYPE(Null)
TIRO_CHECK_VM_TYPE(Record)
TIRO_CHECK_VM_TYPE(RecordTemplate)
TIRO_CHECK_VM_TYPE(Result)
TIRO_CHECK_VM_TYPE(Set)
TIRO_CHECK_VM_TYPE(SetIterator)
TIRO_CHECK_VM_TYPE(SmallInteger)
TIRO_CHECK_VM_TYPE(String)
TIRO_CHECK_VM_TYPE(StringBuilder)
TIRO_CHECK_VM_TYPE(StringIterator)
TIRO_CHECK_VM_TYPE(StringSlice)
TIRO_CHECK_VM_TYPE(Symbol)
TIRO_CHECK_VM_TYPE(Tuple)
TIRO_CHECK_VM_TYPE(TupleIterator)
TIRO_CHECK_VM_TYPE(Type)
TIRO_CHECK_VM_TYPE(Undefined)
TIRO_CHECK_VM_TYPE(UnresolvedImport)
// [[[end]]]

TIRO_CHECK_VM_TYPE(Nullable<Value>)

#undef TIRO_CHECK_VM_TYPE

} // namespace tiro::vm
