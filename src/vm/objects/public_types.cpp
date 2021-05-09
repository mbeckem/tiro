#include "vm/objects/public_types.hpp"

namespace tiro::vm {

std::string_view to_string(PublicType pt) {
#define TIRO_CASE(pt)    \
    case PublicType::pt: \
        return #pt;

    switch (pt) {
        /* [[[cog
            from cog import outl
            from codegen.objects import PUBLIC_TYPES
            for pt in PUBLIC_TYPES:
                outl(f"TIRO_CASE({pt.name})")
            ]]] */
        TIRO_CASE(Array)
        TIRO_CASE(ArrayIterator)
        TIRO_CASE(Boolean)
        TIRO_CASE(Buffer)
        TIRO_CASE(Coroutine)
        TIRO_CASE(CoroutineToken)
        TIRO_CASE(Exception)
        TIRO_CASE(Float)
        TIRO_CASE(Function)
        TIRO_CASE(Integer)
        TIRO_CASE(Map)
        TIRO_CASE(MapIterator)
        TIRO_CASE(MapKeyIterator)
        TIRO_CASE(MapKeyView)
        TIRO_CASE(MapValueIterator)
        TIRO_CASE(MapValueView)
        TIRO_CASE(Module)
        TIRO_CASE(NativeObject)
        TIRO_CASE(NativePointer)
        TIRO_CASE(Null)
        TIRO_CASE(Record)
        TIRO_CASE(Result)
        TIRO_CASE(Set)
        TIRO_CASE(SetIterator)
        TIRO_CASE(String)
        TIRO_CASE(StringBuilder)
        TIRO_CASE(StringIterator)
        TIRO_CASE(StringSlice)
        TIRO_CASE(Symbol)
        TIRO_CASE(Tuple)
        TIRO_CASE(TupleIterator)
        TIRO_CASE(Type)
        // [[[end]]]
    }

#undef TIRO_CASE

    TIRO_UNREACHABLE("Invalid public type");
}

} // namespace tiro::vm
