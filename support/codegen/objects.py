class VMObject:
    def __init__(self, name):
        self.name = name

    @property
    def type_name(self):
        return self.name

    @property
    def type_tag(self):
        return f"ValueType::{self.name}"


class PublicType:
    def __init__(self, name, vm_objects):
        self.name = name
        self.vm_objects = vm_objects

    @property
    def type_tag(self):
        return f"PublicType::{self.name}"


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
        VMObject(name="Exception"),
        VMObject(name="Float"),
        VMObject(name="Function"),
        VMObject(name="FunctionTemplate"),
        VMObject(name="HandlerTable"),
        VMObject(name="HashTable"),
        VMObject(name="HashTableIterator"),
        VMObject(name="HashTableKeyIterator"),
        VMObject(name="HashTableKeyView"),
        VMObject(name="HashTableStorage"),
        VMObject(name="HashTableValueIterator"),
        VMObject(name="HashTableValueView"),
        VMObject(name="Integer"),
        VMObject(name="InternalType"),
        VMObject(name="MagicFunction"),
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

# Maps public type names to vm object type names
PUBLIC_TYPES = sorted(
    [
        PublicType("Array", ["Array"]),
        PublicType("ArrayIterator", ["ArrayIterator"]),
        PublicType("Boolean", ["Boolean"]),
        PublicType("Buffer", ["Buffer"]),
        PublicType("Coroutine", ["Coroutine"]),
        PublicType("CoroutineToken", ["CoroutineToken"]),
        PublicType("Exception", ["Exception"]),
        PublicType("Float", ["Float"]),
        PublicType(
            "Function", ["BoundMethod", "Function", "MagicFunction", "NativeFunction"]
        ),
        PublicType("Map", ["HashTable"]),
        PublicType("MapKeyView", ["HashTableKeyView"]),
        PublicType("MapValueView", ["HashTableValueView"]),
        PublicType("MapIterator", ["HashTableIterator"]),
        PublicType("MapKeyIterator", ["HashTableKeyIterator"]),
        PublicType("MapValueIterator", ["HashTableValueIterator"]),
        PublicType("Integer", ["Integer", "SmallInteger"]),
        PublicType("Module", ["Module"]),
        PublicType("NativeObject", ["NativeObject"]),
        PublicType("NativePointer", ["NativePointer"]),
        PublicType("Null", ["Null"]),
        PublicType("Record", ["Record"]),
        PublicType("Result", ["Result"]),
        PublicType("Set", ["Set"]),
        PublicType("String", ["String"]),
        PublicType("StringIterator", ["StringIterator"]),
        PublicType("StringBuilder", ["StringBuilder"]),
        PublicType("StringSlice", ["StringSlice"]),
        PublicType("Symbol", ["Symbol"]),
        PublicType("Tuple", ["Tuple"]),
        PublicType("TupleIterator", ["TupleIterator"]),
        PublicType("Type", ["Type"]),
    ],
    key=lambda p: p.name,
)

