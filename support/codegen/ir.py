# Contains the definition of the IR.

from textwrap import dedent
from .unions import Tag, Union, Struct, Alias, Field

ModuleMemberData = Union(
    name="ModuleMemberData",
    tag=Tag("ModuleMemberType", "u8"),
    members=[
        Struct(
            name="Import",
            doc="Represents an import of another module.",
            members=[
                Field("name", "InternedString", doc="The name of the imported module.")
            ],
        ),
        Struct(
            name="Variable",
            doc="Represents a variable at module scope.",
            members=[Field("name", "InternedString", doc="The name of the variable.")],
        ),
        Struct(
            name="Function",
            doc="Represents a function of this module, in IR form.",
            members=[
                Field(
                    "id", "FunctionId", doc="The Id of the function within this module."
                )
            ],
        ),
    ],
).set_format_mode("define")

Terminator = (
    Union(
        name="Terminator",
        tag=Tag("TerminatorType", "u8"),
        doc="Represents edges connecting different basic blocks.",
        members=[
            Struct(
                name="None",
                doc=dedent(
                    """\
                    The block has no outgoing edge. This is the initial value after a new block has been created.
                    must be changed to one of the valid edge types when construction is complete."""
                ),
                members=[],
            ),
            Struct(
                name="Entry",
                doc="Contains the actual entry points into the function.",
                members=[
                    Field(
                        "body",
                        "BlockId",
                        doc="The start of the function under normal control flow.",
                    ),
                    Field(
                        "handlers",
                        "std::vector<BlockId>",
                        doc="Exception handler blocks.",
                        pass_as="move",
                    ),
                ],
            ),
            Struct(
                name="Exit", doc="Marks the exit block of the function.", members=[]
            ),
            Struct(
                name="Jump",
                doc="A single successor block, reached through an unconditional jump.",
                members=[Field("target", "BlockId", doc="The jump target."),],
            ),
            Struct(
                name="Branch",
                doc="A conditional jump with two successor blocks.",
                members=[
                    Field("type", "BranchType", doc="The kind of conditional jump."),
                    Field("value", "InstId", doc="The value that is being tested."),
                    Field(
                        "target", "BlockId", doc="The jump target for successful tests."
                    ),
                    Field(
                        "fallthrough",
                        "BlockId",
                        doc="The jump target for failed tests.",
                    ),
                ],
            ),
            Struct(
                name="Return",
                doc="Returns a value from the function.",
                members=[
                    Field("value", "InstId", doc="The value that is being returned."),
                    Field(
                        "target",
                        "BlockId",
                        doc=dedent(
                            """\
                            The successor block. This must be the exit block.
                            These edges are needed to make all code paths converge at the exit block."""
                        ),
                    ),
                ],
            ),
            Struct(
                name="Rethrow",
                doc="Throws the currently active exception. Only legal when used in exceptional control flow.",
                members=[
                    Field(
                        "target",
                        "BlockId",
                        doc=dedent(
                            """\
                            The successor block. This must be the exit block.
                            These edges are needed to make all code paths converge at the exit block."""
                        ),
                    )
                ],
            ),
            Struct(
                name="AssertFail",
                doc="An assertion failure is an unconditional hard exit.",
                members=[
                    Field(
                        "expr",
                        "InstId",
                        doc="The string representation of the failed assertion.",
                    ),
                    Field(
                        "message",
                        "InstId",
                        doc="The message that will be printed when the assertion fails.",
                    ),
                    Field(
                        "target",
                        "BlockId",
                        doc=dedent(
                            """\
                            The successor block. This must be the exit block.
                            These edges are needed to make all code paths converge at the exit block."""
                        ),
                    ),
                ],
            ),
            Struct(
                name="Never",
                doc="The block never returns (e.g. contains a statement that never terminates).",
                members=[
                    Field(
                        "target",
                        "BlockId",
                        doc=dedent(
                            """\
                            The successor block. This must be the exit block.
                            These edges are needed to make all code paths converge at the exit block."""
                        ),
                    )
                ],
            ),
        ],
    )
    .set_format_mode("declare")
    .set_storage_mode("movable")
    .set_accessors("all")
)

LValue = Union(
    name="LValue",
    tag=Tag("LValueType", "u8"),
    doc=dedent(
        """\
        LValues can appear as the left hand side of an assignment.
        They are associated with a mutable storage location.
        LValues do not use SSA form since they may reference memory shared
        with other parts of the program."""
    ),
    members=[
        Struct(
            name="Param",
            doc="Reference to a function argument.",
            members=[
                Field("target", "ParamId", doc="Argument index in parameter list.")
            ],
        ),
        Struct(
            name="Closure",
            doc="Reference to a variable captured from an outer scope.",
            members=[
                Field(
                    "env",
                    "InstId",
                    doc="The environment to search. Either a local variable or the function's outer environment.",
                ),
                Field(
                    "levels",
                    "u32",
                    doc='Levels to "go up" the environment hierarchy. 0 is the closure environment itself.',
                ),
                Field("index", "u32", doc="Index into the environment."),
            ],
        ),
        Struct(
            name="Module",
            doc="Reference to a variable at module scope.",
            members=[
                Field(
                    "member", "ModuleMemberId", doc="Id of the module level variable.",
                )
            ],
        ),
        Struct(
            name="Field",
            doc="Reference to the field of an object (i.e. `object.foo`).",
            members=[
                Field("object", "InstId", doc="Dereferenced object."),
                Field("name", "InternedString", doc="Field name to access."),
            ],
        ),
        Struct(
            name="TupleField",
            doc="Reference to a tuple field of a tuple (i.e. `tuple.3`).",
            members=[
                Field("object", "InstId", doc="Dereferenced tuple object."),
                Field("index", "u32", doc="Index of the tuple member."),
            ],
        ),
        Struct(
            name="Index",
            doc="Reference to an index of an array (or a map), i.e. `thing[foo]`.",
            members=[
                Field("object", "InstId", doc="Dereferenced arraylike object."),
                Field("index", "InstId", doc="Index into the array."),
            ],
        ),
    ],
).set_format_mode("define")

Constant = (
    Union(
        name="Constant",
        tag=Tag("ConstantType", "u8"),
        doc="Represents a compile time constant.",
        members=[
            Struct("Integer", members=[Field("value", "i64")]),
            Alias("Float", "FloatConstant"),
            Struct("String", members=[Field("value", "InternedString")]),
            Struct("Symbol", members=[Field("value", "InternedString")]),
            Struct("Null"),
            Struct("True"),
            Struct("False"),
        ],
    )
    .set_format_mode("define")
    .set_equality_mode("define")
    .set_hash_mode("define")
)

Aggregate = (
    Union(
        name="Aggregate",
        tag=Tag("AggregateType", "u8"),
        doc=dedent(
            """\
            Represents the compile time type of an aggregate value.
            Aggregate values are an aggregate of other values, which (at this time)
            only exist as virtual entities at IR level.
            The main use case right now is to group member instances and method pointers
            or efficient method calls."""
        ),
        members=[
            Struct(
                "Method",
                doc="Represents a method invocation (returns instance, method pointer).",
                members=[
                    Field("instance", "InstId"),
                    Field("function", "InternedString"),
                ],
            ),
            Struct(
                "IteratorNext",
                doc="Represents the result of advancing an iterator (returns valid, value).",
                members=[Field("iterator", "InstId")],
            ),
        ],
    )
    .set_format_mode("define")
    .set_equality_mode("define")
    .set_hash_mode("define")
)

Value = (
    Union(
        name="Value",
        tag=Tag("ValueType", "u8"),
        doc=dedent(
            """\
            Represents the value of an instruction.

            Values at this compilation stage do not allow inner control flow. Nested
            language-level expressions that contain loops or conditionals are split up
            so that only "simple" expressions remain."""
        ),
        members=[
            Struct(
                name="Read",
                doc="Read from an lvalue to produce a value.",
                members=[Field("target", "LValue", doc="Dereferenced lvalue.")],
            ),
            Struct(
                name="Write",
                doc=dedent(
                    """\
                    Write to an lvalue. Write operations are side effects only.
                    The result of a write should not be an operand for other instructions."""
                ),
                members=[
                    Field("target", "LValue", doc="The write target."),
                    Field("value", "InstId", doc="The new value."),
                ],
            ),
            Struct(
                name="Alias",
                doc="References the value of another instruction.",
                members=[Field("target", "InstId", doc="Used instruction.")],
            ),
            Alias(
                "Phi",
                "tiro::ir::Phi",
                doc="Phi nodes can have one of multiple instructions as their value, depending on the code path that led to them.",
            ),
            Struct(
                name="ObserveAssign",
                doc=dedent(
                    """\
                    Similar to phi nodes, but used for exception handling.
                    All operands must be PublishAssign values, and all such values must belong to the same symbol."""
                ),
                members=[
                    Field(
                        "symbol",
                        "SymbolId",
                        doc="The symbol (variable) observed by this node.",
                    ),
                    Field(
                        "operands",
                        "LocalListId",
                        doc="The list of assignments observed by this node.",
                    ),
                ],
            ),
            Struct(
                name="PublishAssign",
                doc=dedent(
                    """\
                    Marks the assignment of an ssa variable to a new value.
                    This is needed for exception handlers, which must be able to observe side effects.
                    Optimized out when not actually used."""
                ),
                members=[
                    Field("symbol", "SymbolId", doc="The reassigned symbol."),
                    Field("value", "InstId", doc="The new SSA value."),
                ],
            ),
            Alias("Constant", "tiro::ir::Constant", doc="A constant."),
            Struct(
                name="OuterEnvironment",
                doc="Dereferences the function's outer closure environment",
            ),
            Struct(
                name="BinaryOp",
                doc="Simple binary operation.",
                members=[
                    Field("op", "BinaryOpType"),
                    Field("left", "InstId", doc="Left operand."),
                    Field("right", "InstId", doc="Right operand."),
                ],
            ),
            Struct(
                name="UnaryOp",
                doc="Simple unary operation.",
                members=[Field("op", "UnaryOpType"), Field("operand", "InstId"),],
            ),
            Struct(
                name="Call",
                doc="Function call expression, i.e. `f(a, b, c)`.",
                members=[
                    Field("func", "InstId", doc="Function to call."),
                    Field("args", "LocalListId", doc="The list of function arguments."),
                ],
            ),
            Alias(
                name="Aggregate",
                target="tiro::ir::Aggregate",
                doc=dedent("""Represents an aggregate value."""),
            ),
            Struct(
                name="GetAggregateMember",
                doc="Fetches a member value from an aggregate.",
                members=[
                    Field(
                        "aggregate",
                        "InstId",
                        doc="Must be an aggregate value of the correct type.",
                    ),
                    Field(
                        "member",
                        "AggregateMember",
                        doc="The aggregate member returned from the aggregate.",
                    ),
                ],
            ),
            Struct(
                name="MethodCall",
                doc="Method call expression, i.e `a.b(c, d)`.",
                members=[
                    Field(
                        "method",
                        "InstId",
                        doc="Method to be called. Must be a method value.",
                    ),
                    Field("args", "LocalListId", doc="List of method arguments."),
                ],
            ),
            Struct(
                name="MakeEnvironment",
                doc="Creates a new closure environment.",
                members=[
                    Field("parent", "InstId", doc="The parent environment."),
                    Field(
                        "size",
                        "u32",
                        doc="The number of variable slots in the new environment.",
                    ),
                ],
            ),
            Struct(
                name="MakeClosure",
                doc="Creates a new closure function.",
                members=[
                    Field("env", "InstId", doc="The closure environment."),
                    Field(
                        "func",
                        "InstId",
                        doc="The closure function's template location.",
                    ),
                ],
            ),
            Struct(
                name="MakeIterator",
                doc="Creates a new iterator for a given container instance.",
                members=[
                    Field("container", "InstId", doc="The container being iterated."),
                ],
            ),
            Struct(
                name="Record",
                doc="Creates a new record.",
                members=[
                    Field("value", "RecordId", doc="Points to the record's content.")
                ],
            ),
            Struct(
                name="Container",
                doc=dedent(
                    """\
                Construct a container from the argument list,
                such as an array, a tuple or a map."""
                ),
                members=[
                    Field(
                        "container",
                        "ContainerType",
                        doc="Container type we're going to construct.",
                    ),
                    Field(
                        "args",
                        "LocalListId",
                        doc=dedent(
                            """\
                            Arguments for the container constructor (list of elements,
                            or list of key/value-pairs in the case of Map and Record)."""
                        ),
                    ),
                ],
            ),
            Struct(
                name="Format",
                doc=dedent(
                    """\
                    Takes a list of values and formats them as a string.
                    This is used to implement format string literals."""
                ),
                members=[Field("args", "LocalListId", doc="The list of values.")],
            ),
            Struct(
                name="Error",
                doc=dedent(
                    """\
                    Represents an error value that was generated to continue with the translation (for analysis).
                    Never present in a valid program."""
                ),
            ),
            Struct(name="Nop", doc="A value that has been optimized out."),
        ],
    )
    .set_format_mode("define")
    .set_storage_mode("movable")
    .set_accessors("all")
)
