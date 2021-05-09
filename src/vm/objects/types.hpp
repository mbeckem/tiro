#ifndef TIRO_VM_OBJECTS_TYPES_HPP
#define TIRO_VM_OBJECTS_TYPES_HPP

#include "common/defs.hpp"
#include "common/format.hpp"
#include "vm/objects/fwd.hpp"

namespace tiro::vm {

// Important: don't use 0 as a value (see object header struct).
//
// NOTE: This enum is eventually going away. A subset of its values will still be
// used in some limited form in the class layout (currently called InternalType).
// Once we have user defined types, we cannot discriminate between all types using an enum anyway.
enum class ValueType : u8 {
    /* [[[cog
        from cog import outl
        from codegen.objects import VM_OBJECTS
        objects = sorted(VM_OBJECTS, key = lambda o: o.id)
        for object in objects:
            outl(f"{object.name} = {object.id},")
    ]]] */
    Null = 1,
    Boolean = 2,
    Float = 3,
    HeapInteger = 4,
    SmallInteger = 5,
    Symbol = 6,
    String = 7,
    StringSlice = 8,
    StringIterator = 9,
    StringBuilder = 10,
    BoundMethod = 11,
    CodeFunction = 12,
    MagicFunction = 13,
    NativeFunction = 14,
    Code = 15,
    Environment = 16,
    CodeFunctionTemplate = 17,
    HandlerTable = 18,
    Type = 19,
    Method = 20,
    InternalType = 21,
    Array = 22,
    ArrayIterator = 23,
    ArrayStorage = 24,
    Buffer = 25,
    HashTable = 26,
    HashTableIterator = 27,
    HashTableKeyView = 28,
    HashTableKeyIterator = 29,
    HashTableValueView = 30,
    HashTableValueIterator = 31,
    HashTableStorage = 32,
    Record = 33,
    RecordTemplate = 34,
    Set = 35,
    SetIterator = 36,
    Tuple = 37,
    TupleIterator = 38,
    NativeObject = 39,
    NativePointer = 40,
    Exception = 41,
    Result = 42,
    Coroutine = 43,
    CoroutineStack = 44,
    CoroutineToken = 45,
    Module = 46,
    Undefined = 47,
    UnresolvedImport = 48,
    // [[[end]]]
};

/* [[[cog
    from cog import outl
    from codegen.objects import VM_OBJECTS
    max_object = max(VM_OBJECTS, key = lambda o: o.id)
    outl(f"inline constexpr u8 max_value_type = static_cast<u8>({max_object.type_tag});")
]]] */
inline constexpr u8 max_value_type = static_cast<u8>(ValueType::UnresolvedImport);
// [[[end]]]

std::string_view to_string(ValueType type);

namespace detail {

template<typename Type>
struct MapTypeToTag;

template<ValueType type>
struct MapTagToType;

template<typename Type>
struct MapBaseToValueTypes {
    static constexpr bool is_base_type = false;
};

#define TIRO_REGISTER_VM_TYPE(Type, Tag)        \
    template<>                                  \
    struct MapTypeToTag<Type> {                 \
        static constexpr ValueType tag = (Tag); \
    };                                          \
                                                \
    template<>                                  \
    struct MapTagToType<Tag> {                  \
        using type = Type;                      \
    };

#define TIRO_REGISTER_VM_BASE_TYPE(Type, MinTag, MaxTag)                       \
    template<>                                                                 \
    struct MapBaseToValueTypes<Type> {                                         \
        static constexpr bool is_base_type = true;                             \
        static constexpr std::underlying_type_t<ValueType> min_tag = (MinTag); \
        static constexpr std::underlying_type_t<ValueType> max_tag = (MaxTag); \
    };

/* [[[cog
    from cog import outl
    from codegen.objects import VM_OBJECTS, VM_OBJECT_BASES
    for object in VM_OBJECTS:
        outl(f"TIRO_REGISTER_VM_TYPE({object.type_name}, {object.type_tag})")
    for base in VM_OBJECT_BASES:
        outl(f"TIRO_REGISTER_VM_BASE_TYPE({base.name}, {base.min_id}, {base.max_id})")
]]] */
TIRO_REGISTER_VM_TYPE(Array, ValueType::Array)
TIRO_REGISTER_VM_TYPE(ArrayIterator, ValueType::ArrayIterator)
TIRO_REGISTER_VM_TYPE(ArrayStorage, ValueType::ArrayStorage)
TIRO_REGISTER_VM_TYPE(Boolean, ValueType::Boolean)
TIRO_REGISTER_VM_TYPE(BoundMethod, ValueType::BoundMethod)
TIRO_REGISTER_VM_TYPE(Buffer, ValueType::Buffer)
TIRO_REGISTER_VM_TYPE(Code, ValueType::Code)
TIRO_REGISTER_VM_TYPE(CodeFunction, ValueType::CodeFunction)
TIRO_REGISTER_VM_TYPE(CodeFunctionTemplate, ValueType::CodeFunctionTemplate)
TIRO_REGISTER_VM_TYPE(Coroutine, ValueType::Coroutine)
TIRO_REGISTER_VM_TYPE(CoroutineStack, ValueType::CoroutineStack)
TIRO_REGISTER_VM_TYPE(CoroutineToken, ValueType::CoroutineToken)
TIRO_REGISTER_VM_TYPE(Environment, ValueType::Environment)
TIRO_REGISTER_VM_TYPE(Exception, ValueType::Exception)
TIRO_REGISTER_VM_TYPE(Float, ValueType::Float)
TIRO_REGISTER_VM_TYPE(HandlerTable, ValueType::HandlerTable)
TIRO_REGISTER_VM_TYPE(HashTable, ValueType::HashTable)
TIRO_REGISTER_VM_TYPE(HashTableIterator, ValueType::HashTableIterator)
TIRO_REGISTER_VM_TYPE(HashTableKeyIterator, ValueType::HashTableKeyIterator)
TIRO_REGISTER_VM_TYPE(HashTableKeyView, ValueType::HashTableKeyView)
TIRO_REGISTER_VM_TYPE(HashTableStorage, ValueType::HashTableStorage)
TIRO_REGISTER_VM_TYPE(HashTableValueIterator, ValueType::HashTableValueIterator)
TIRO_REGISTER_VM_TYPE(HashTableValueView, ValueType::HashTableValueView)
TIRO_REGISTER_VM_TYPE(HeapInteger, ValueType::HeapInteger)
TIRO_REGISTER_VM_TYPE(InternalType, ValueType::InternalType)
TIRO_REGISTER_VM_TYPE(MagicFunction, ValueType::MagicFunction)
TIRO_REGISTER_VM_TYPE(Method, ValueType::Method)
TIRO_REGISTER_VM_TYPE(Module, ValueType::Module)
TIRO_REGISTER_VM_TYPE(NativeFunction, ValueType::NativeFunction)
TIRO_REGISTER_VM_TYPE(NativeObject, ValueType::NativeObject)
TIRO_REGISTER_VM_TYPE(NativePointer, ValueType::NativePointer)
TIRO_REGISTER_VM_TYPE(Null, ValueType::Null)
TIRO_REGISTER_VM_TYPE(Record, ValueType::Record)
TIRO_REGISTER_VM_TYPE(RecordTemplate, ValueType::RecordTemplate)
TIRO_REGISTER_VM_TYPE(Result, ValueType::Result)
TIRO_REGISTER_VM_TYPE(Set, ValueType::Set)
TIRO_REGISTER_VM_TYPE(SetIterator, ValueType::SetIterator)
TIRO_REGISTER_VM_TYPE(SmallInteger, ValueType::SmallInteger)
TIRO_REGISTER_VM_TYPE(String, ValueType::String)
TIRO_REGISTER_VM_TYPE(StringBuilder, ValueType::StringBuilder)
TIRO_REGISTER_VM_TYPE(StringIterator, ValueType::StringIterator)
TIRO_REGISTER_VM_TYPE(StringSlice, ValueType::StringSlice)
TIRO_REGISTER_VM_TYPE(Symbol, ValueType::Symbol)
TIRO_REGISTER_VM_TYPE(Tuple, ValueType::Tuple)
TIRO_REGISTER_VM_TYPE(TupleIterator, ValueType::TupleIterator)
TIRO_REGISTER_VM_TYPE(Type, ValueType::Type)
TIRO_REGISTER_VM_TYPE(Undefined, ValueType::Undefined)
TIRO_REGISTER_VM_TYPE(UnresolvedImport, ValueType::UnresolvedImport)
TIRO_REGISTER_VM_BASE_TYPE(Function, 11, 14)
TIRO_REGISTER_VM_BASE_TYPE(Integer, 4, 5)
// [[[end]]]

#undef TIRO_REGISTER_VM_TYPE
#undef TIRO_REGISTER_VM_BASE_TYPE

} // namespace detail

template<typename Type>
inline constexpr ValueType TypeToTag = detail::MapTypeToTag<Type>::tag;

template<ValueType tag>
using TagToType = typename detail::MapTagToType<tag>::type;

} // namespace tiro::vm

TIRO_ENABLE_FREE_TO_STRING(tiro::vm::ValueType)

#endif // TIRO_VM_OBJECTS_TYPES_HPP
