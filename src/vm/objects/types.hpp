#ifndef TIRO_VM_OBJECTS_TYPES_HPP
#define TIRO_VM_OBJECTS_TYPES_HPP

#include "common/defs.hpp"
#include "common/format.hpp"
#include "vm/objects/fwd.hpp"

namespace tiro::vm {

// Important: don't use 0 as a value (see object header struct).
enum class ValueType : u8 {
    /* [[[cog
        from cog import outl
        from codegen.objects import VM_OBJECTS
        for (index, object) in enumerate(VM_OBJECTS):
            outl(f"{object.name} = {index + 1},")
    ]]] */
    Array = 1,
    ArrayIterator = 2,
    ArrayStorage = 3,
    Boolean = 4,
    BoundMethod = 5,
    Buffer = 6,
    Code = 7,
    Coroutine = 8,
    CoroutineStack = 9,
    CoroutineToken = 10,
    Environment = 11,
    Exception = 12,
    Float = 13,
    Function = 14,
    FunctionTemplate = 15,
    HashTable = 16,
    HashTableIterator = 17,
    HashTableKeyIterator = 18,
    HashTableKeyView = 19,
    HashTableStorage = 20,
    HashTableValueIterator = 21,
    HashTableValueView = 22,
    Integer = 23,
    InternalType = 24,
    Method = 25,
    Module = 26,
    NativeFunction = 27,
    NativeObject = 28,
    NativePointer = 29,
    Null = 30,
    Record = 31,
    RecordTemplate = 32,
    Result = 33,
    Set = 34,
    SetIterator = 35,
    SmallInteger = 36,
    String = 37,
    StringBuilder = 38,
    StringIterator = 39,
    StringSlice = 40,
    Symbol = 41,
    Tuple = 42,
    TupleIterator = 43,
    Type = 44,
    Undefined = 45,
    UnresolvedImport = 46,
    // [[[end]]]
};

/* [[[cog
    from cog import outl
    from codegen.objects import VM_OBJECTS
    outl(f"inline constexpr u8 max_value_type = static_cast<u8>(ValueType::{VM_OBJECTS[-1].name});")
]]] */
inline constexpr u8 max_value_type = static_cast<u8>(ValueType::UnresolvedImport);
// [[[end]]]

std::string_view to_string(ValueType type);

namespace detail {

template<typename Type>
struct MapTypeToTag;

template<ValueType type>
struct MapTagToType;

#define TIRO_MAP_VM_TYPE(Type, Tag)           \
    template<>                                \
    struct MapTypeToTag<Type> {               \
        static constexpr ValueType tag = Tag; \
    };                                        \
                                              \
    template<>                                \
    struct MapTagToType<Tag> {                \
        using type = Type;                    \
    };

/* [[[cog
    from cog import outl
    from codegen.objects import VM_OBJECTS
    for object in VM_OBJECTS:
        outl(f"TIRO_MAP_VM_TYPE({object.type_name}, {object.type_tag})")
]]] */
TIRO_MAP_VM_TYPE(Array, ValueType::Array)
TIRO_MAP_VM_TYPE(ArrayIterator, ValueType::ArrayIterator)
TIRO_MAP_VM_TYPE(ArrayStorage, ValueType::ArrayStorage)
TIRO_MAP_VM_TYPE(Boolean, ValueType::Boolean)
TIRO_MAP_VM_TYPE(BoundMethod, ValueType::BoundMethod)
TIRO_MAP_VM_TYPE(Buffer, ValueType::Buffer)
TIRO_MAP_VM_TYPE(Code, ValueType::Code)
TIRO_MAP_VM_TYPE(Coroutine, ValueType::Coroutine)
TIRO_MAP_VM_TYPE(CoroutineStack, ValueType::CoroutineStack)
TIRO_MAP_VM_TYPE(CoroutineToken, ValueType::CoroutineToken)
TIRO_MAP_VM_TYPE(Environment, ValueType::Environment)
TIRO_MAP_VM_TYPE(Exception, ValueType::Exception)
TIRO_MAP_VM_TYPE(Float, ValueType::Float)
TIRO_MAP_VM_TYPE(Function, ValueType::Function)
TIRO_MAP_VM_TYPE(FunctionTemplate, ValueType::FunctionTemplate)
TIRO_MAP_VM_TYPE(HashTable, ValueType::HashTable)
TIRO_MAP_VM_TYPE(HashTableIterator, ValueType::HashTableIterator)
TIRO_MAP_VM_TYPE(HashTableKeyIterator, ValueType::HashTableKeyIterator)
TIRO_MAP_VM_TYPE(HashTableKeyView, ValueType::HashTableKeyView)
TIRO_MAP_VM_TYPE(HashTableStorage, ValueType::HashTableStorage)
TIRO_MAP_VM_TYPE(HashTableValueIterator, ValueType::HashTableValueIterator)
TIRO_MAP_VM_TYPE(HashTableValueView, ValueType::HashTableValueView)
TIRO_MAP_VM_TYPE(Integer, ValueType::Integer)
TIRO_MAP_VM_TYPE(InternalType, ValueType::InternalType)
TIRO_MAP_VM_TYPE(Method, ValueType::Method)
TIRO_MAP_VM_TYPE(Module, ValueType::Module)
TIRO_MAP_VM_TYPE(NativeFunction, ValueType::NativeFunction)
TIRO_MAP_VM_TYPE(NativeObject, ValueType::NativeObject)
TIRO_MAP_VM_TYPE(NativePointer, ValueType::NativePointer)
TIRO_MAP_VM_TYPE(Null, ValueType::Null)
TIRO_MAP_VM_TYPE(Record, ValueType::Record)
TIRO_MAP_VM_TYPE(RecordTemplate, ValueType::RecordTemplate)
TIRO_MAP_VM_TYPE(Result, ValueType::Result)
TIRO_MAP_VM_TYPE(Set, ValueType::Set)
TIRO_MAP_VM_TYPE(SetIterator, ValueType::SetIterator)
TIRO_MAP_VM_TYPE(SmallInteger, ValueType::SmallInteger)
TIRO_MAP_VM_TYPE(String, ValueType::String)
TIRO_MAP_VM_TYPE(StringBuilder, ValueType::StringBuilder)
TIRO_MAP_VM_TYPE(StringIterator, ValueType::StringIterator)
TIRO_MAP_VM_TYPE(StringSlice, ValueType::StringSlice)
TIRO_MAP_VM_TYPE(Symbol, ValueType::Symbol)
TIRO_MAP_VM_TYPE(Tuple, ValueType::Tuple)
TIRO_MAP_VM_TYPE(TupleIterator, ValueType::TupleIterator)
TIRO_MAP_VM_TYPE(Type, ValueType::Type)
TIRO_MAP_VM_TYPE(Undefined, ValueType::Undefined)
TIRO_MAP_VM_TYPE(UnresolvedImport, ValueType::UnresolvedImport)
// [[[end]]]

#undef TIRO_MAP_VM_TYPE

} // namespace detail

template<typename Type>
inline constexpr ValueType TypeToTag = detail::MapTypeToTag<Type>::tag;

template<ValueType tag>
using TagToType = typename detail::MapTagToType<tag>::type;

} // namespace tiro::vm

TIRO_ENABLE_FREE_TO_STRING(tiro::vm::ValueType)

#endif // TIRO_VM_OBJECTS_TYPES_HPP
