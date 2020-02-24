from codegen import Tag, TaggedUnion, UnionMemberStruct, UnionMemberAlias, StructMember
from textwrap import dedent

EdgeType = Tag("EdgeType", "u8")

Edge = TaggedUnion(
    name="Edge",
    tag=EdgeType,
    doc="Represents an edge that connects two basic blocks.",
    members=[
        UnionMemberStruct(
            name="None",
            doc=dedent("""\
               The block has no outgoing edge. This is the initial value after a new block has been created.
               must be changed to one of the valid edge types when construction is complete."""),
            members=[]
        ),
        UnionMemberStruct(
            name="Jump",
            doc="A single successor block, reached through an unconditional jump.",
            members=[
                StructMember(
                    "target", "BlockID",
                    doc="The jump target."
                ),
            ]
        ),
        UnionMemberStruct(
            name="Branch",
            doc="A conditional jump with two successor blocks.",
            members=[
                StructMember(
                    "type", "BranchType",
                    doc="The kind of conditional jump."
                ),
                StructMember(
                    "value", "LocalID",
                    doc="The value that is being tested."
                ),
                StructMember(
                    "target", "BlockID",
                    doc="The jump target for successful tests."
                ),
                StructMember(
                    "fallthrough", "BlockID",
                    doc="The jump target for failed tests."
                ),
            ]
        ),
        UnionMemberStruct(
            name="Return",
            doc="The block returns from the function.",
            members=[]
        ).set_argument_name("return_"),
        UnionMemberStruct(
            name="AssertFail",
            doc="An assertion failure is an unconditional hard exit.",
            members=[
                StructMember(
                    "message", "LocalID",
                    doc="The message that will be printed when the assertion fails."
                ),
            ]
        ),
        UnionMemberStruct(
            name="Never",
            doc="The block never returns (e.g. contains a statement that never terminates).",
            members=[]
        ),
    ]
).format_function("define")

LValueType = Tag("LValueType", "u8")

LValue = TaggedUnion(
    name="LValue",
    tag=LValueType,
    doc=dedent("""\
        LValues can appear as the left hand side of an assignment.
        They are associated with a mutable storage location.
        LValues do not use SSA form since they may reference memory shared
        with other parts of the program."""),
    members=[
        UnionMemberStruct(
            name="Argument",
            doc="Reference to a function argument.",
            members=[
                StructMember("index", "u32",
                             doc="Argument index in parameter list.")
            ]
        ),
        UnionMemberStruct(
            name="Closure",
            doc="Reference to a variable captured from an outer scope.",
            members=[
                StructMember(
                    "context", "LocalID",
                    doc="The context to search. Either a local variable or the function's outer context."
                ),
                StructMember(
                    "levels", "u32",
                    doc="Levels to \"go up\" the closure hierarchy. 0 is the closure context itself."
                ),
                StructMember(
                    "index", "u32",
                    doc="Index into the closure context."
                )
            ]
        ),
        UnionMemberStruct(
            name="Module",
            doc="Reference to a variable at module scope.",
            members=[
                StructMember("index", "u32", doc="Index into the module.")
            ]
        ),
        UnionMemberStruct(
            name="Field",
            doc="Reference to the field of an object (i.e. `object.foo`).",
            members=[
                StructMember("object", "LocalID",
                             doc="Dereferenced object."),
                StructMember("name", "InternedString",
                             doc="Field name to access.")
            ]
        ),
        UnionMemberStruct(
            name="TupleField",
            doc="Referencce to a tuple field of a tuple (i.e. `tuple.3`).",
            members=[
                StructMember("object", "LocalID",
                             doc="Dereferenced tuple object."),
                StructMember("index", "u32", doc="Index of the tuple member.")
            ]
        ),
        UnionMemberStruct(
            name="Index",
            doc="Reference to an index of an array (or a map), i.e. `thing[foo]`.",
            members=[
                StructMember("object", "LocalID",
                             doc="Dereferenced arraylike object."),
                StructMember("index", "LocalID",
                             doc="Index into the array."),
            ]
        ),
    ]
).format_function("define")

ConstantType = Tag("ConstantType", "u8")

Constant = TaggedUnion(
    name="Constant",
    tag=ConstantType,
    doc="Represents a compile time constant.",
    members=[
        UnionMemberStruct(
            "Integer", members=[StructMember("value", "i64")]
        ),
        UnionMemberStruct(
            "Float", members=[StructMember("value", "f64")]
        ),
        UnionMemberStruct(
            "String", members=[StructMember("value", "InternedString")]
        ),
        UnionMemberStruct(
            "Symbol", members=[StructMember("value", "InternedString")]
        ),
        UnionMemberStruct("Null"),
        UnionMemberStruct("True"),
        UnionMemberStruct("False"),
    ]
).format_function("define")

RValueType = Tag("RValueType", "u8")

RValue = TaggedUnion(
    name="RValue",
    tag=RValueType,
    doc=dedent("""\
        Represents an rvalue.
        RValues can be used as the right hand side of an assignment or definition.

        RValues at this compilation stage do not allow inner control flow. Nested
        language-level expressions that contain loops or conditionals are split up
        so that only "simple" expressions remain."""),
    members=[
        UnionMemberStruct(
            name="UseLValue",
            doc="References an lvalue to produce a value.",
            members=[
                StructMember("target", "LValue",
                             doc="Dereferenced lvalue.")
            ]
        ),
        UnionMemberStruct(
            name="UseLocal",
            doc="References a local variable.",
            members=[
                StructMember("target", "LocalID",
                             doc="Dereferenced local.")
            ]
        ),
        UnionMemberAlias("Constant", "mir::Constant", doc="A constant."),
        UnionMemberStruct(
            name="OuterContext",
            doc="Deferences the function's outer closure context"
        ),
        UnionMemberStruct(
            name="BinaryOp",
            doc="Simple binary operation.",
            members=[
                StructMember("op", "BinaryOpType"),
                StructMember("left", "LocalID", doc="Left operand."),
                StructMember("right", "LocalID", doc="Right operand.")
            ]
        ),
        UnionMemberStruct(
            name="UnaryOp",
            doc="Simple unary operation.",
            members=[
                StructMember("op", "UnaryOpType"),
                StructMember("operand", "LocalID")
            ]
        ),
        UnionMemberStruct(
            name="Call",
            doc=" Function call expression, i.e. `f(a, b, c)`.",
            members=[
                StructMember("func", "LocalID", doc="Function to call."),
                StructMember("args", "LocalListID",
                             doc="The list of function arguments.")
            ]
        ),
        UnionMemberStruct(
            name="MethodCall",
            doc="Method call expression, i.e `a.b(c, d)`.",
            members=[
                StructMember("object", "LocalID",
                             doc="Object whose method we're going to invoke."),
                StructMember("method", "InternedString",
                             doc="Name of the method to be called."),
                StructMember("args", "LocalListID",
                             doc="List of method arguments.")
            ]
        ),
        UnionMemberStruct(
            name="Container",
            doc=dedent("""\
                Construct a container from the argument list,
                such as an array, a tuple or a map."""),
            members=[
                StructMember("container", "ContainerType",
                             doc="Container type we're going to construct."),
                StructMember("args", "LocalListID", doc=dedent("""\
                    Arguments for the container constructor (list of elements,
                    or list of key/value-pairs in the case of Map)."""))
            ]
        )
    ]
).format_function("define")

StmtType = Tag("StmtType", "u8")

Stmt = TaggedUnion(
    name="Stmt",
    tag=StmtType,
    doc=dedent("""\
        Represents a statement, i.e. a single instruction inside a basic block."""),
    members=[
        UnionMemberStruct(
            name="Assign",
            doc="Assigns a value to a memory location (non-SSA operations).",
            members=[
                StructMember("target", "LValue",
                             doc="The assignment target."),
                StructMember("value", "RValue",
                             doc="The new value.")
            ]
        ),
        UnionMemberStruct(
            name="Define",
            doc="Defines a new local variable (SSA).",
            members=[
                StructMember("local", "LocalID"),
            ]
        ),
        UnionMemberStruct(
            name="SetReturn",
            doc="Sets the function's return value.",
            members=[
                StructMember("value", "LocalID",
                             doc="The return value.")
            ]
        ),
        UnionMemberStruct(
            name="EnterScope",
            doc="Marks the start of a lexical scope.",
            members=[
                StructMember("scope", "ScopeID",
                             doc="The id of the scope.")
            ]
        ),
        UnionMemberStruct(
            name="ExitScope",
            doc="Marks the end of a lexical scope.",
            members=[
                StructMember("scope", "ScopeID",
                             doc="The id of the scope.")
            ]
        ),
    ]
).format_function("define")
