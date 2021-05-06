#include "vm/heap/header.hpp"

#include "vm/objects/all.hpp"

namespace tiro::vm {

template<typename T>
size_t object_size_impl([[maybe_unused]] T value) {
    if constexpr (std::is_base_of_v<HeapValue, T>) {
        using Layout = typename T::Layout;
        using Traits = LayoutTraits<Layout>;
        if constexpr (Traits::has_static_size) {
            return Traits::static_size;
        } else {
            return Traits::dynamic_size(value.layout());
        }
    } else {
        TIRO_DEBUG_ASSERT(false, "Cannot be called on a non-heap value.");
        return 0;
    }
}

size_t object_size(Header* header) {
    HeapValue value(header);
    InternalType type = value.type_instance();
    switch (type.builtin_type()) {

#define TIRO_CASE(Type)   \
    case TypeToTag<Type>: \
        return object_size_impl(Type(value));

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
        TIRO_CASE(HeapInteger)
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

} // namespace tiro::vm
