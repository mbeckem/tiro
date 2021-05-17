#ifndef TIRO_VM_OBJECTS_PUBLIC_TYPES_HPP
#define TIRO_VM_OBJECTS_PUBLIC_TYPES_HPP

#include "common/adt/span.hpp"
#include "common/assert.hpp"
#include "common/defs.hpp"
#include "vm/objects/types.hpp"

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

template<PublicType pt>
constexpr Span<const ValueType> to_value_types() {
    return PublicTypeToValueTypes<pt>::value_types;
}

#define TIRO_PUBLIC_TO_VALUE_TYPES(pt, ...)                       \
    template<>                                                    \
    struct PublicTypeToValueTypes<pt> {                           \
        static constexpr ValueType value_types[] = {__VA_ARGS__}; \
    };

/* [[[cog
    from cog import outl
    from codegen.objects import PUBLIC_TYPES
    for pt in PUBLIC_TYPES:
        value_types = ", ".join(f"ValueType::{name}" for name in pt.vm_objects)
        outl(f"TIRO_PUBLIC_TO_VALUE_TYPES({pt.type_tag}, {value_types})")
]]] */
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Array, ValueType::Array)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::ArrayIterator, ValueType::ArrayIterator)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Boolean, ValueType::Boolean)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Buffer, ValueType::Buffer)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Coroutine, ValueType::Coroutine)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::CoroutineToken, ValueType::CoroutineToken)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Exception, ValueType::Exception)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Float, ValueType::Float)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Function, ValueType::BoundMethod, ValueType::CodeFunction,
    ValueType::MagicFunction, ValueType::NativeFunction)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Integer, ValueType::HeapInteger, ValueType::SmallInteger)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Map, ValueType::HashTable)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::MapIterator, ValueType::HashTableIterator)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::MapKeyIterator, ValueType::HashTableKeyIterator)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::MapKeyView, ValueType::HashTableKeyView)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::MapValueIterator, ValueType::HashTableValueIterator)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::MapValueView, ValueType::HashTableValueView)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Module, ValueType::Module)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::NativeObject, ValueType::NativeObject)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::NativePointer, ValueType::NativePointer)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Null, ValueType::Null)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Record, ValueType::Record)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Result, ValueType::Result)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Set, ValueType::Set)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::SetIterator, ValueType::SetIterator)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::String, ValueType::String)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::StringBuilder, ValueType::StringBuilder)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::StringIterator, ValueType::StringIterator)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::StringSlice, ValueType::StringSlice)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Symbol, ValueType::Symbol)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Tuple, ValueType::Tuple)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::TupleIterator, ValueType::TupleIterator)
TIRO_PUBLIC_TO_VALUE_TYPES(PublicType::Type, ValueType::Type)
// [[[end]]]

#undef TIRO_PUBLIC_TO_VALUE_TYPES

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_PUBLIC_TYPES_HPP
