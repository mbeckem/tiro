#ifndef TIRO_VM_OBJECTS_PUBLIC_TYPES_HPP
#define TIRO_VM_OBJECTS_PUBLIC_TYPES_HPP

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

#define TIRO_MAP_PUBLIC_TYPE(pt, ...)                             \
    template<>                                                    \
    struct PublicTypeToValueTypes<pt> {                           \
        static constexpr ValueType value_types[] = {__VA_ARGS__}; \
    };

/* [[[cog
    from cog import outl
    from codegen.objects import PUBLIC_TYPES
    for pt in PUBLIC_TYPES:
        value_types = ", ".join(f"ValueType::{name}" for name in pt.vm_objects)
        outl(f"TIRO_MAP_PUBLIC_TYPE({pt.type_tag}, {value_types})")
]]] */
TIRO_MAP_PUBLIC_TYPE(PublicType::Array, ValueType::Array)
TIRO_MAP_PUBLIC_TYPE(PublicType::ArrayIterator, ValueType::ArrayIterator)
TIRO_MAP_PUBLIC_TYPE(PublicType::Boolean, ValueType::Boolean)
TIRO_MAP_PUBLIC_TYPE(PublicType::Buffer, ValueType::Buffer)
TIRO_MAP_PUBLIC_TYPE(PublicType::Coroutine, ValueType::Coroutine)
TIRO_MAP_PUBLIC_TYPE(PublicType::CoroutineToken, ValueType::CoroutineToken)
TIRO_MAP_PUBLIC_TYPE(PublicType::Exception, ValueType::Exception)
TIRO_MAP_PUBLIC_TYPE(PublicType::Float, ValueType::Float)
TIRO_MAP_PUBLIC_TYPE(PublicType::Function, ValueType::BoundMethod, ValueType::CodeFunction,
    ValueType::MagicFunction, ValueType::NativeFunction)
TIRO_MAP_PUBLIC_TYPE(PublicType::Integer, ValueType::HeapInteger, ValueType::SmallInteger)
TIRO_MAP_PUBLIC_TYPE(PublicType::Map, ValueType::HashTable)
TIRO_MAP_PUBLIC_TYPE(PublicType::MapIterator, ValueType::HashTableIterator)
TIRO_MAP_PUBLIC_TYPE(PublicType::MapKeyIterator, ValueType::HashTableKeyIterator)
TIRO_MAP_PUBLIC_TYPE(PublicType::MapKeyView, ValueType::HashTableKeyView)
TIRO_MAP_PUBLIC_TYPE(PublicType::MapValueIterator, ValueType::HashTableValueIterator)
TIRO_MAP_PUBLIC_TYPE(PublicType::MapValueView, ValueType::HashTableValueView)
TIRO_MAP_PUBLIC_TYPE(PublicType::Module, ValueType::Module)
TIRO_MAP_PUBLIC_TYPE(PublicType::NativeObject, ValueType::NativeObject)
TIRO_MAP_PUBLIC_TYPE(PublicType::NativePointer, ValueType::NativePointer)
TIRO_MAP_PUBLIC_TYPE(PublicType::Null, ValueType::Null)
TIRO_MAP_PUBLIC_TYPE(PublicType::Record, ValueType::Record)
TIRO_MAP_PUBLIC_TYPE(PublicType::Result, ValueType::Result)
TIRO_MAP_PUBLIC_TYPE(PublicType::Set, ValueType::Set)
TIRO_MAP_PUBLIC_TYPE(PublicType::SetIterator, ValueType::SetIterator)
TIRO_MAP_PUBLIC_TYPE(PublicType::String, ValueType::String)
TIRO_MAP_PUBLIC_TYPE(PublicType::StringBuilder, ValueType::StringBuilder)
TIRO_MAP_PUBLIC_TYPE(PublicType::StringIterator, ValueType::StringIterator)
TIRO_MAP_PUBLIC_TYPE(PublicType::StringSlice, ValueType::StringSlice)
TIRO_MAP_PUBLIC_TYPE(PublicType::Symbol, ValueType::Symbol)
TIRO_MAP_PUBLIC_TYPE(PublicType::Tuple, ValueType::Tuple)
TIRO_MAP_PUBLIC_TYPE(PublicType::TupleIterator, ValueType::TupleIterator)
TIRO_MAP_PUBLIC_TYPE(PublicType::Type, ValueType::Type)
// [[[end]]]
} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_PUBLIC_TYPES_HPP
