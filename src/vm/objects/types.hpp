#ifndef TIRO_VM_OBJECTS_TYPES_HPP
#define TIRO_VM_OBJECTS_TYPES_HPP

#include "common/defs.hpp"
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
    ArrayStorage = 2,
    Boolean = 3,
    BoundMethod = 4,
    Buffer = 5,
    Code = 6,
    Coroutine = 7,
    CoroutineStack = 8,
    DynamicObject = 9,
    Environment = 10,
    Float = 11,
    Function = 12,
    FunctionTemplate = 13,
    HashTable = 14,
    HashTableIterator = 15,
    HashTableStorage = 16,
    Integer = 17,
    InternalType = 18,
    Method = 19,
    Module = 20,
    NativeFunction = 21,
    NativeObject = 22,
    NativePointer = 23,
    Null = 24,
    SmallInteger = 25,
    String = 26,
    StringBuilder = 27,
    StringSlice = 28,
    Symbol = 29,
    Tuple = 30,
    Type = 31,
    Undefined = 32,
    // [[[end]]]
};

/* [[[cog
    from cog import outl
    from codegen.objects import VM_OBJECTS
    outl(f"inline constexpr u8 max_value_type = static_cast<u8>(ValueType::{VM_OBJECTS[-1].name});")
]]] */
inline constexpr u8 max_value_type = static_cast<u8>(ValueType::Undefined);
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
TIRO_MAP_VM_TYPE(ArrayStorage, ValueType::ArrayStorage)
TIRO_MAP_VM_TYPE(Boolean, ValueType::Boolean)
TIRO_MAP_VM_TYPE(BoundMethod, ValueType::BoundMethod)
TIRO_MAP_VM_TYPE(Buffer, ValueType::Buffer)
TIRO_MAP_VM_TYPE(Code, ValueType::Code)
TIRO_MAP_VM_TYPE(Coroutine, ValueType::Coroutine)
TIRO_MAP_VM_TYPE(CoroutineStack, ValueType::CoroutineStack)
TIRO_MAP_VM_TYPE(DynamicObject, ValueType::DynamicObject)
TIRO_MAP_VM_TYPE(Environment, ValueType::Environment)
TIRO_MAP_VM_TYPE(Float, ValueType::Float)
TIRO_MAP_VM_TYPE(Function, ValueType::Function)
TIRO_MAP_VM_TYPE(FunctionTemplate, ValueType::FunctionTemplate)
TIRO_MAP_VM_TYPE(HashTable, ValueType::HashTable)
TIRO_MAP_VM_TYPE(HashTableIterator, ValueType::HashTableIterator)
TIRO_MAP_VM_TYPE(HashTableStorage, ValueType::HashTableStorage)
TIRO_MAP_VM_TYPE(Integer, ValueType::Integer)
TIRO_MAP_VM_TYPE(InternalType, ValueType::InternalType)
TIRO_MAP_VM_TYPE(Method, ValueType::Method)
TIRO_MAP_VM_TYPE(Module, ValueType::Module)
TIRO_MAP_VM_TYPE(NativeFunction, ValueType::NativeFunction)
TIRO_MAP_VM_TYPE(NativeObject, ValueType::NativeObject)
TIRO_MAP_VM_TYPE(NativePointer, ValueType::NativePointer)
TIRO_MAP_VM_TYPE(Null, ValueType::Null)
TIRO_MAP_VM_TYPE(SmallInteger, ValueType::SmallInteger)
TIRO_MAP_VM_TYPE(String, ValueType::String)
TIRO_MAP_VM_TYPE(StringBuilder, ValueType::StringBuilder)
TIRO_MAP_VM_TYPE(StringSlice, ValueType::StringSlice)
TIRO_MAP_VM_TYPE(Symbol, ValueType::Symbol)
TIRO_MAP_VM_TYPE(Tuple, ValueType::Tuple)
TIRO_MAP_VM_TYPE(Type, ValueType::Type)
TIRO_MAP_VM_TYPE(Undefined, ValueType::Undefined)
// [[[end]]]

#undef TIRO_MAP_VM_TYPE

} // namespace detail

template<typename Type>
inline constexpr ValueType TypeToTag = detail::MapTypeToTag<Type>::tag;

template<ValueType tag>
using TagToType = typename detail::MapTagToType<tag>::type;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_TYPES_HPP
