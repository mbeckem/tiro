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
        VMObject(name="Null"),
        VMObject(name="Undefined"),
        VMObject(name="Boolean"),
        VMObject(name="Integer"),
        VMObject(name="Float"),
        VMObject(name="SmallInteger"),
        VMObject(name="String"),
        VMObject(name="StringBuilder"),
        VMObject(name="Symbol"),
        VMObject(name="Code"),
        VMObject(name="FunctionTemplate"),
        VMObject(name="Environment"),
        VMObject(name="Function"),
        VMObject(name="NativeFunction"),
        VMObject(name="NativeAsyncFunction"),
        VMObject(name="NativeObject"),
        VMObject(name="NativePointer"),
        VMObject(name="DynamicObject"),
        VMObject(name="Method"),
        VMObject(name="BoundMethod"),
        VMObject(name="Module"),
        VMObject(name="Tuple"),
        VMObject(name="Array"),
        VMObject(name="ArrayStorage"),
        VMObject(name="Buffer"),
        VMObject(name="HashTable"),
        VMObject(name="HashTableStorage"),
        VMObject(name="HashTableIterator"),
        VMObject(name="Coroutine"),
        VMObject(name="CoroutineStack"),
        VMObject(name="Type"),
    ],
    key=lambda o: o.name,
)
