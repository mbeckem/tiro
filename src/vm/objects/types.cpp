#include "vm/objects/types.hpp"

namespace tiro::vm {

std::string_view to_string(ValueType type) {
    switch (type) {
#define TIRO_CASE(Name)   \
    case ValueType::Name: \
        return #Name;

        /* [[[cog
            from cog import outl
            from codegen.objects import VM_OBJECTS
            for object in VM_OBJECTS:
                outl(f"TIRO_CASE({object.name})")
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
        TIRO_CASE(Float)
        TIRO_CASE(Function)
        TIRO_CASE(FunctionTemplate)
        TIRO_CASE(HashTable)
        TIRO_CASE(HashTableIterator)
        TIRO_CASE(HashTableKeyIterator)
        TIRO_CASE(HashTableKeyView)
        TIRO_CASE(HashTableStorage)
        TIRO_CASE(HashTableValueIterator)
        TIRO_CASE(HashTableValueView)
        TIRO_CASE(Integer)
        TIRO_CASE(InternalType)
        TIRO_CASE(Method)
        TIRO_CASE(Module)
        TIRO_CASE(NativeFunction)
        TIRO_CASE(NativeObject)
        TIRO_CASE(NativePointer)
        TIRO_CASE(Null)
        TIRO_CASE(Record)
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
        // [[[end]]]
    }
#undef TIRO_CASE

    TIRO_UNREACHABLE("Invalid value type.");
}

} // namespace tiro::vm
