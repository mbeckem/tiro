class VMObject:
    def __init__(self, name):
        self.name = name

    @property
    def type_name(self):
        return self.name

    @property
    def type_tag(self):
        return f"ValueType::{self.name}"


VM_OBJECTS = sorted(
    [
        VMObject(name="Array"),
        VMObject(name="ArrayIterator"),
        VMObject(name="ArrayStorage"),
        VMObject(name="Boolean"),
        VMObject(name="BoundMethod"),
        VMObject(name="Buffer"),
        VMObject(name="Code"),
        VMObject(name="Coroutine"),
        VMObject(name="CoroutineStack"),
        VMObject(name="CoroutineToken"),
        VMObject(name="Environment"),
        VMObject(name="Float"),
        VMObject(name="Function"),
        VMObject(name="FunctionTemplate"),
        VMObject(name="HashTable"),
        VMObject(name="HashTableIterator"),
        VMObject(name="HashTableKeyIterator"),
        VMObject(name="HashTableKeyView"),
        VMObject(name="HashTableStorage"),
        VMObject(name="HashTableValueIterator"),
        VMObject(name="HashTableValueView"),
        VMObject(name="Integer"),
        VMObject(name="InternalType"),
        VMObject(name="Method"),
        VMObject(name="Module"),
        VMObject(name="NativeFunction"),
        VMObject(name="NativeObject"),
        VMObject(name="NativePointer"),
        VMObject(name="Null"),
        VMObject(name="Result"),
        VMObject(name="Record"),
        VMObject(name="RecordTemplate"),
        VMObject(name="Set"),
        VMObject(name="SetIterator"),
        VMObject(name="SmallInteger"),
        VMObject(name="String"),
        VMObject(name="StringBuilder"),
        VMObject(name="StringIterator"),
        VMObject(name="StringSlice"),
        VMObject(name="Symbol"),
        VMObject(name="Tuple"),
        VMObject(name="TupleIterator"),
        VMObject(name="Type"),
        VMObject(name="Undefined"),
        VMObject(name="UnresolvedImport"),
    ],
    key=lambda o: o.name,
)
