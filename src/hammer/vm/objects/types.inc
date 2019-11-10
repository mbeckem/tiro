/*  
 * This file lists all vm types used in this library.
 * The HAMMER_VM_TYPE macro is invoked for every type name.
 */

#ifndef HAMMER_VM_TYPE
#    define HAMMER_VM_TYPE(Name)
#endif

HAMMER_VM_TYPE(Null)
HAMMER_VM_TYPE(Undefined)
HAMMER_VM_TYPE(Boolean)
HAMMER_VM_TYPE(Integer)
HAMMER_VM_TYPE(Float)
HAMMER_VM_TYPE(String)
HAMMER_VM_TYPE(StringBuilder)
HAMMER_VM_TYPE(Symbol)
HAMMER_VM_TYPE(SpecialValue)
HAMMER_VM_TYPE(Code)
HAMMER_VM_TYPE(FunctionTemplate)
HAMMER_VM_TYPE(ClosureContext)
HAMMER_VM_TYPE(Function)
HAMMER_VM_TYPE(NativeFunction)
HAMMER_VM_TYPE(Module)
HAMMER_VM_TYPE(Tuple)
HAMMER_VM_TYPE(Array)
HAMMER_VM_TYPE(ArrayStorage)
HAMMER_VM_TYPE(U8Array)
HAMMER_VM_TYPE(U16Array)
HAMMER_VM_TYPE(U32Array)
HAMMER_VM_TYPE(U64Array)
HAMMER_VM_TYPE(I8Array)
HAMMER_VM_TYPE(I16Array)
HAMMER_VM_TYPE(I32Array)
HAMMER_VM_TYPE(I64Array)
HAMMER_VM_TYPE(F32Array)
HAMMER_VM_TYPE(F64Array)
HAMMER_VM_TYPE(HashTable)
HAMMER_VM_TYPE(HashTableStorage)
HAMMER_VM_TYPE(HashTableIterator)
HAMMER_VM_TYPE(Coroutine)
HAMMER_VM_TYPE(CoroutineStack)

#undef HAMMER_VM_TYPE
