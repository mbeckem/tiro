class Node:
    def __init__(self, name, public=False, children=None):
        self.id_ = None
        self.parent = None
        self.name = name
        self.children = children or []

        if isinstance(public, bool):
            self.public = public
            self._public_name = None if not public else name
        elif isinstance(public, str):
            self.public = True
            self._public_name = public
        else:
            raise RuntimeError(
                f"Unexpected value for 'public' (expected bool or str): {repr(public)}"
            )

    @property
    def is_leaf(self):
        return len(self.children) == 0

    @property
    def is_base(self):
        return len(self.children) > 0

    @property
    def is_root(self):
        return self.parent is None

    @property
    def id(self):
        if not self.is_leaf:
            raise RuntimeError(f"Type '{self.name}' is not a leaf")
        if not self.id_:
            raise RuntimeError(f"Type id for leaf type '{self.name}' was not allocated")
        return self.id_

    @property
    def id_range(self):
        if self.is_leaf:
            return (self.id, self.id)

        lo = None
        hi = None
        for child in self.children:
            (a, b) = child.id_range
            lo = a if lo is None else min(a, lo)
            hi = b if hi is None else max(b, hi)
        return (lo, hi)

    @property
    def public_name(self):
        if self._public_name is None:
            raise RuntimeError(f"'{self.name}'' is not public")
        return self._public_name


class VMObject:
    def __init__(self, name, id):
        self.id = id
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


def init_tree(root):
    next_id = 1
    lookup = dict()

    def visit(node, parent):
        nonlocal next_id
        node.parent = parent

        if node.name in lookup:
            raise RuntimeError(f"Node name '{node.name}' already defined")
        lookup[node.name] = node

        if node.is_leaf:
            node.id_ = next_id
            next_id += 1
        else:
            for child in node.children:
                visit(child, node)

    visit(root, None)
    return (root, lookup)


def gather_vm_objects(root):
    leaves = []

    def visit(node):
        nonlocal leaves

        if node.is_leaf:
            leaves.append(node)
        else:
            for child in node.children:
                visit(child)

    visit(root)
    return [VMObject(node.name, node.id) for node in leaves]


def gather_public_types(root):
    public = []

    def visit(node, public_parent):
        if node.public:
            public_parent = PublicType(node.public_name, [])
            public.append(public_parent)

        if public_parent is not None and node.is_leaf:
            public_parent.vm_objects.append(node.name)

        for child in node.children:
            visit(child, public_parent)

    visit(root, None)
    return public


(HIERARCHY, HIERARCHY_LOOKUP) = init_tree(
    Node(
        "Value",
        children=[
            #
            # Primitives
            # ----------
            Node("Null", public=True),
            Node("Boolean", public=True),
            Node("Float", public=True),
            Node(
                "Integer",
                public=True,
                children=[Node("HeapInteger"), Node("SmallInteger")],
            ),
            Node("Symbol", public=True),
            #
            # Strings
            # -------
            Node("String", public=True),
            Node("StringSlice", public=True),
            Node("StringIterator", public=True),
            Node("StringBuilder", public=True),
            #
            # Functions
            # ---------
            Node(
                "Function",
                public=True,
                children=[
                    Node("BoundMethod"),
                    Node("CodeFunction"),
                    Node("MagicFunction"),
                    Node("NativeFunction"),
                ],
            ),
            Node("Code"),
            Node("Environment"),
            Node("CodeFunctionTemplate"),
            Node("HandlerTable"),
            #
            # Types
            # -----
            Node("Type", public=True),
            Node("Method"),
            Node("InternalType"),
            #
            # Containers
            # ----------
            Node("Array", public=True),
            Node("ArrayIterator", public=True),
            Node("ArrayStorage"),
            Node("Buffer", public=True),
            Node("HashTable", public="Map"),
            Node("HashTableIterator", public="MapIterator"),
            Node("HashTableKeyView", public="MapKeyView"),
            Node("HashTableKeyIterator", public="MapKeyIterator"),
            Node("HashTableValueView", public="MapValueView"),
            Node("HashTableValueIterator", public="MapValueIterator"),
            Node("HashTableStorage"),
            Node("Record", public=True),
            Node("RecordTemplate"),
            Node("Set", public=True),
            Node("SetIterator", public=True),
            Node("Tuple", public=True),
            Node("TupleIterator", public=True),
            #
            # Native objects
            # --------------
            Node("NativeObject", public=True),
            Node("NativePointer", public=True),
            #
            # Other
            # -----
            Node("Exception", public=True),
            Node("Result", public=True),
            Node("Coroutine", public=True),
            Node("CoroutineStack"),
            Node("CoroutineToken", public=True),
            Node("Module", public=True),
            Node("Undefined"),
            Node("UnresolvedImport"),
        ],
    )
)

# List of leaf types that must have a corresponding c++ class
VM_OBJECTS = sorted(gather_vm_objects(HIERARCHY), key=lambda o: o.name)

# Maps public type names to vm object type names
PUBLIC_TYPES = sorted(gather_public_types(HIERARCHY), key=lambda p: p.name)

