#ifndef TIRO_VM_OBJECTS_PUBLIC_TYPES_HPP
#define TIRO_VM_OBJECTS_PUBLIC_TYPES_HPP

#include "common/adt/span.hpp"
#include "common/assert.hpp"
#include "common/defs.hpp"
#include "common/format.hpp"
#include "vm/objects/types.hpp"

#include <optional>
#include <string_view>

namespace tiro::vm {

/// Native object types exposed to user code.
/// Every public type maps to one or more native types implemented in `./objects/*`.
///
/// For example, multiple function types (bytecode function, bound function, ...) all share
/// the public type `Function`.
enum class PublicType : u8 {
    /* [[[cog
    from cog import outl
    from codegen.objects import PUBLIC_TYPES
    for pt in PUBLIC_TYPES:
        outl(f"{pt.name},")
    ]]] */
    Array,
    ArrayIterator,
    Boolean,
    Buffer,
    Coroutine,
    CoroutineToken,
    Exception,
    Float,
    Function,
    Integer,
    Map,
    MapIterator,
    MapKeyIterator,
    MapKeyView,
    MapValueIterator,
    MapValueView,
    Module,
    NativeObject,
    NativePointer,
    Null,
    Record,
    RecordSchema,
    Result,
    Set,
    SetIterator,
    String,
    StringBuilder,
    StringIterator,
    StringSlice,
    Symbol,
    Tuple,
    TupleIterator,
    Type,
    // [[[end]]]
};

/* [[[cog
    from cog import outl
    from codegen.objects import PUBLIC_TYPES
    outl(f"inline constexpr u8 max_public_type = static_cast<u8>({PUBLIC_TYPES[-1].type_tag});")
]]] */
inline constexpr u8 max_public_type = static_cast<u8>(PublicType::Type);
// [[[end]]]

std::string_view to_string(PublicType pt);

template<PublicType pt>
struct PublicTypeToValueTypes;

#define TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(pt, ...)                  \
    template<>                                                    \
    struct PublicTypeToValueTypes<pt> {                           \
        static constexpr ValueType value_types[] = {__VA_ARGS__}; \
    };

/* [[[cog
    from cog import outl
    from codegen.objects import PUBLIC_TYPES
    for pt in PUBLIC_TYPES:
        value_types = ", ".join(f"ValueType::{name}" for name in pt.vm_objects)
        outl(f"TIRO_PUBLIC_TYPE_TO_VALUE_TYPES({pt.type_tag}, {value_types})")
]]] */
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Array, ValueType::Array)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::ArrayIterator, ValueType::ArrayIterator)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Boolean, ValueType::Boolean)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Buffer, ValueType::Buffer)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Coroutine, ValueType::Coroutine)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::CoroutineToken, ValueType::CoroutineToken)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Exception, ValueType::Exception)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Float, ValueType::Float)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Function, ValueType::BoundMethod,
    ValueType::CodeFunction, ValueType::MagicFunction, ValueType::NativeFunction)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(
    PublicType::Integer, ValueType::HeapInteger, ValueType::SmallInteger)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Map, ValueType::HashTable)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::MapIterator, ValueType::HashTableIterator)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::MapKeyIterator, ValueType::HashTableKeyIterator)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::MapKeyView, ValueType::HashTableKeyView)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::MapValueIterator, ValueType::HashTableValueIterator)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::MapValueView, ValueType::HashTableValueView)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Module, ValueType::Module)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::NativeObject, ValueType::NativeObject)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::NativePointer, ValueType::NativePointer)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Null, ValueType::Null)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Record, ValueType::Record)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::RecordSchema, ValueType::RecordSchema)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Result, ValueType::Result)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Set, ValueType::Set)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::SetIterator, ValueType::SetIterator)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::String, ValueType::String)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::StringBuilder, ValueType::StringBuilder)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::StringIterator, ValueType::StringIterator)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::StringSlice, ValueType::StringSlice)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Symbol, ValueType::Symbol)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Tuple, ValueType::Tuple)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::TupleIterator, ValueType::TupleIterator)
TIRO_PUBLIC_TYPE_TO_VALUE_TYPES(PublicType::Type, ValueType::Type)
// [[[end]]]

#undef TIRO_PUBLIC_TYPE_TO_VALUE_TYPES

inline constexpr Span<const ValueType> to_value_types(PublicType pt) {
    switch (pt) {

#define TIRO_MAP(pt) \
case pt:             \
    return PublicTypeToValueTypes<pt>::value_types;

        /* [[[cog
            from cog import outl
            from codegen.objects import PUBLIC_TYPES
            for pt in PUBLIC_TYPES:
                outl(f"TIRO_MAP({pt.type_tag})")
        ]]] */
        TIRO_MAP(PublicType::Array)
        TIRO_MAP(PublicType::ArrayIterator)
        TIRO_MAP(PublicType::Boolean)
        TIRO_MAP(PublicType::Buffer)
        TIRO_MAP(PublicType::Coroutine)
        TIRO_MAP(PublicType::CoroutineToken)
        TIRO_MAP(PublicType::Exception)
        TIRO_MAP(PublicType::Float)
        TIRO_MAP(PublicType::Function)
        TIRO_MAP(PublicType::Integer)
        TIRO_MAP(PublicType::Map)
        TIRO_MAP(PublicType::MapIterator)
        TIRO_MAP(PublicType::MapKeyIterator)
        TIRO_MAP(PublicType::MapKeyView)
        TIRO_MAP(PublicType::MapValueIterator)
        TIRO_MAP(PublicType::MapValueView)
        TIRO_MAP(PublicType::Module)
        TIRO_MAP(PublicType::NativeObject)
        TIRO_MAP(PublicType::NativePointer)
        TIRO_MAP(PublicType::Null)
        TIRO_MAP(PublicType::Record)
        TIRO_MAP(PublicType::RecordSchema)
        TIRO_MAP(PublicType::Result)
        TIRO_MAP(PublicType::Set)
        TIRO_MAP(PublicType::SetIterator)
        TIRO_MAP(PublicType::String)
        TIRO_MAP(PublicType::StringBuilder)
        TIRO_MAP(PublicType::StringIterator)
        TIRO_MAP(PublicType::StringSlice)
        TIRO_MAP(PublicType::Symbol)
        TIRO_MAP(PublicType::Tuple)
        TIRO_MAP(PublicType::TupleIterator)
        TIRO_MAP(PublicType::Type)
        // [[[end]]]

#undef TIRO_MAP
    }
}

inline constexpr std::optional<PublicType> to_public_type(ValueType vt) {
    switch (vt) {

#define TIRO_MAP(vt, pt) \
case ValueType::vt:      \
    return (pt);

        /* [[[cog
            from cog import outl
            from codegen.objects import PUBLIC_TYPES
            for pt in PUBLIC_TYPES:
                for vt in pt.vm_objects:
                    outl(f"TIRO_MAP({vt}, {pt.type_tag});")
        ]]] */
        TIRO_MAP(Array, PublicType::Array);
        TIRO_MAP(ArrayIterator, PublicType::ArrayIterator);
        TIRO_MAP(Boolean, PublicType::Boolean);
        TIRO_MAP(Buffer, PublicType::Buffer);
        TIRO_MAP(Coroutine, PublicType::Coroutine);
        TIRO_MAP(CoroutineToken, PublicType::CoroutineToken);
        TIRO_MAP(Exception, PublicType::Exception);
        TIRO_MAP(Float, PublicType::Float);
        TIRO_MAP(BoundMethod, PublicType::Function);
        TIRO_MAP(CodeFunction, PublicType::Function);
        TIRO_MAP(MagicFunction, PublicType::Function);
        TIRO_MAP(NativeFunction, PublicType::Function);
        TIRO_MAP(HeapInteger, PublicType::Integer);
        TIRO_MAP(SmallInteger, PublicType::Integer);
        TIRO_MAP(HashTable, PublicType::Map);
        TIRO_MAP(HashTableIterator, PublicType::MapIterator);
        TIRO_MAP(HashTableKeyIterator, PublicType::MapKeyIterator);
        TIRO_MAP(HashTableKeyView, PublicType::MapKeyView);
        TIRO_MAP(HashTableValueIterator, PublicType::MapValueIterator);
        TIRO_MAP(HashTableValueView, PublicType::MapValueView);
        TIRO_MAP(Module, PublicType::Module);
        TIRO_MAP(NativeObject, PublicType::NativeObject);
        TIRO_MAP(NativePointer, PublicType::NativePointer);
        TIRO_MAP(Null, PublicType::Null);
        TIRO_MAP(Record, PublicType::Record);
        TIRO_MAP(RecordSchema, PublicType::RecordSchema);
        TIRO_MAP(Result, PublicType::Result);
        TIRO_MAP(Set, PublicType::Set);
        TIRO_MAP(SetIterator, PublicType::SetIterator);
        TIRO_MAP(String, PublicType::String);
        TIRO_MAP(StringBuilder, PublicType::StringBuilder);
        TIRO_MAP(StringIterator, PublicType::StringIterator);
        TIRO_MAP(StringSlice, PublicType::StringSlice);
        TIRO_MAP(Symbol, PublicType::Symbol);
        TIRO_MAP(Tuple, PublicType::Tuple);
        TIRO_MAP(TupleIterator, PublicType::TupleIterator);
        TIRO_MAP(Type, PublicType::Type);
        // [[[end]]]

#undef TIRO_MAP
    default:
        return {};
    }
}

} // namespace tiro::vm

TIRO_ENABLE_FREE_TO_STRING(tiro::vm::PublicType)

#endif // TIRO_VM_OBJECTS_PUBLIC_TYPES_HPP
