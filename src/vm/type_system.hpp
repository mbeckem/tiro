#ifndef TIRO_VM_TYPES_HPP
#define TIRO_VM_TYPES_HPP

#include "common/adt/span.hpp"
#include "vm/fwd.hpp"
#include "vm/objects/class.hpp"

#include <array>

namespace tiro::vm {

/// Native object types exposed to user code.
/// Every public type maps to one or more native types implemented in `./objects/*`.
///
/// For example, multiple function types (bytecode function, bound function, ...) all share
/// the public type `Function`.
enum class PublicType : int {
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
TIRO_MAP_PUBLIC_TYPE(PublicType::Function, ValueType::BoundMethod, ValueType::Function,
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
TIRO_MAP_PUBLIC_TYPE(PublicType::String, ValueType::String)
TIRO_MAP_PUBLIC_TYPE(PublicType::StringBuilder, ValueType::StringBuilder)
TIRO_MAP_PUBLIC_TYPE(PublicType::StringIterator, ValueType::StringIterator)
TIRO_MAP_PUBLIC_TYPE(PublicType::StringSlice, ValueType::StringSlice)
TIRO_MAP_PUBLIC_TYPE(PublicType::Symbol, ValueType::Symbol)
TIRO_MAP_PUBLIC_TYPE(PublicType::Tuple, ValueType::Tuple)
TIRO_MAP_PUBLIC_TYPE(PublicType::TupleIterator, ValueType::TupleIterator)
TIRO_MAP_PUBLIC_TYPE(PublicType::Type, ValueType::Type)
// [[[end]]]

class TypeSystem {
public:
    // Called by context during construction (Initial phase for setup of internal types):
    void init_internal(Context& ctx);

    // Called by context during construction (Final phase when bootstrapping is complete).
    void init_public(Context& ctx);

    /// Returns the type instance that represents the given public type.
    Type type_of(PublicType pt);

    /// Returns a value that represents the type of the given object.
    /// Equivalent to looking up the public type value for the object's actual vm object type
    /// and then returning the corresponding type instance.
    ///
    /// Throws if the builtin type is not exposed to the public.
    Type type_of(Handle<Value> object);

    /// Returns a value that represents the public type of the given builtin object type.
    /// Equivalent to looking up the public type value for the object's actual vm object type
    /// and then returning the corresponding type instance.
    ///
    /// Throws if the builtin type is not exposed to the public.
    Type type_of(ValueType builtin);

    /// Attempts to retrieve the value at the given index from the given object.
    /// Throws an error if the index was invalid (e.g. out of bounds).
    ///
    /// TODO Exceptions!
    Value load_index(Context& ctx, Handle<Value> object, Handle<Value> index);

    /// Attempts to set the value at the given index on the given object.
    /// Throws an error if the index was invalid (e.g. out of bounds).
    ///
    /// TODO Exceptions!
    void store_index(Context& ctx, Handle<Value> object, Handle<Value> index, Handle<Value> value);

    /// Attempts to retrieve the given member property from the given object.
    /// Returns no value if there is no such member.
    std::optional<Value> load_member(Context& ctx, Handle<Value> object, Handle<Symbol> member);

    /// Attempts to store the given property value. Returns false if the property
    /// could not be written (does not exist, or is read only).
    ///
    /// TODO Exceptions!
    bool
    store_member(Context& ctx, Handle<Value> object, Handle<Symbol> member, Handle<Value> value);

    /// Constructs an iterator for the given object (if supported).
    /// TODO: Implement useful iterator protocol so we dont have to special case stuff in here.
    Value iterator(Context& ctx, Handle<Value> object);

    /// Advances the given iterator to the next element. Returns an empty optional if the iterator is at the end.
    /// TODO: Implement useful iterator protocol so we dont have to special case stuff in here.
    std::optional<Value> iterator_next(Context& ctx, Handle<Value> iterator);

    /// This function is called for the `object.member(...)` method call syntax.
    /// Returns a member function suitable for invocation on the given instance.
    /// Note that, depending on the function returned here, the call must
    /// be made in different ways (native functions, this pointer, etc.).
    ///
    /// The function value returned here does not need to be a real method - it may be a simple function
    /// that is accessible as the property `object.member`.
    std::optional<Value> load_method(Context& ctx, Handle<Value> object, Handle<Symbol> member);

    /// Returns the builtin type object for the given value type, suitable for object construction.
    /// The returned value is always rooted and does not change after initialization.
    /// Special care has to be taken with types during bootstrap, see init().
    template<typename T>
    Header* raw_internal_type() {
        auto type = internal_types_[value_type_index<T>()];
        TIRO_DEBUG_ASSERT(!type.is_null(),
            "The requested type has not been initialized correctly. "
            "This may be an ordering error during the type initialization phase.");
        return type.value().heap_ptr();
    }

    // Walk all object references rooted in this object.
    template<typename Tracer>
    inline void trace(Tracer&& tracer);

private:
    static constexpr size_t total_public_types = static_cast<size_t>(max_public_type) + 1;
    static constexpr size_t total_internal_types = static_cast<size_t>(max_value_type) + 1;

    static constexpr size_t public_type_index(PublicType public_type) {
        size_t index = static_cast<size_t>(public_type);
        TIRO_DEBUG_ASSERT(index < total_public_types, "Public type index out of bounds.");
        return index;
    }

    template<typename T>
    static constexpr size_t value_type_index() {
        return value_type_index(TypeToTag<T>);
    }

    static constexpr size_t value_type_index(ValueType builtin_type) {
        size_t index = static_cast<size_t>(builtin_type);
        TIRO_DEBUG_ASSERT(index < total_internal_types, "Builtin type index out of bounds.");
        return index;
    }

private:
    std::array<Nullable<Type>, total_public_types> public_types_{};
    std::array<Nullable<InternalType>, total_internal_types> internal_types_{};
};

template<typename Tracer>
void TypeSystem::trace(Tracer&& tracer) {
    for (auto& type : public_types_) {
        tracer(type);
    }

    for (auto& type : internal_types_) {
        tracer(type);
    }
}

} // namespace tiro::vm

#endif // TIRO_VM_TYPES_HPP
