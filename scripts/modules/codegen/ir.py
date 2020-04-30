#!/usr/bin/env python3

from textwrap import dedent
from .unions import Tag, TaggedUnion, UnionMemberStruct, UnionMemberAlias, StructMember

ModuleMemberType = Tag("ModuleMemberType", "u8")

ModuleMember = TaggedUnion(
    name="ModuleMember",
    tag=ModuleMemberType,
    doc="Represents a member of a module.",
    members=[
        UnionMemberStruct(
            name="Import",
            doc="Represents an import of another module.",
            members=[
                StructMember(
                    "name", "InternedString", doc="The name of the imported module."
                )
            ],
        ),
        UnionMemberStruct(
            name="Variable",
            doc="Represents a variable at module scope.",
            members=[
                StructMember("name", "InternedString",
                             doc="The name of the variable.")
            ],
        ),
        UnionMemberStruct(
            name="Function",
            doc="Represents a function of this module, in IR form.",
            members=[
                StructMember(
                    "id", "FunctionID", doc="The ID of the function within this module."
                )
            ],
        ),
    ],
).set_format_mode("define")

TerminatorType = Tag("TerminatorType", "u8")

Terminator = TaggedUnion(
    name="Terminator",
    tag=TerminatorType,
    doc="Represents edges connecting different basic blocks.",
    members=[
        UnionMemberStruct(
            name="None",
            doc=dedent(
                """\
               The block has no outgoing edge. This is the initial value after a new block has been created.
               must be changed to one of the valid edge types when construction is complete."""
            ),
            members=[],
        ),
        UnionMemberStruct(
            name="Jump",
            doc="A single successor block, reached through an unconditional jump.",
            members=[StructMember("target", "BlockID",
                                  doc="The jump target."), ],
        ),
        UnionMemberStruct(
            name="Branch",
            doc="A conditional jump with two successor blocks.",
            members=[
                StructMember("type", "BranchType",
                             doc="The kind of conditional jump."),
                StructMember("value", "LocalID",
                             doc="The value that is being tested."),
                StructMember(
                    "target", "BlockID", doc="The jump target for successful tests."
                ),
                StructMember(
                    "fallthrough", "BlockID", doc="The jump target for failed tests."
                ),
            ],
        ),
        UnionMemberStruct(
            name="Return",
            doc="Returns a value from the function.",
            members=[
                StructMember(
                    "value", "LocalID", doc="The value that is being returned."
                ),
                StructMember(
                    "target",
                    "BlockID",
                    doc=dedent(
                        """\
                        The successor block. This must be the exit block.
                        These edges are needed to make all code paths converge at the exit block."""
                    ),
                ),
            ],
        ),
        UnionMemberStruct(
            name="Exit", doc="Marks the exit block of the function.", members=[]
        ),
        UnionMemberStruct(
            name="AssertFail",
            doc="An assertion failure is an unconditional hard exit.",
            members=[
                StructMember(
                    "expr",
                    "LocalID",
                    doc="The string representation of the failed assertion.",
                ),
                StructMember(
                    "message",
                    "LocalID",
                    doc="The message that will be printed when the assertion fails.",
                ),
                StructMember(
                    "target",
                    "BlockID",
                    doc=dedent(
                        """\
                        The successor block. This must be the exit block.
                        These edges are needed to make all code paths converge at the exit block."""
                    ),
                ),
            ],
        ),
        UnionMemberStruct(
            name="Never",
            doc="The block never returns (e.g. contains a statement that never terminates).",
            members=[
                StructMember(
                    "target",
                    "BlockID",
                    doc=dedent(
                        """\
                        The successor block. This must be the exit block.
                        These edges are needed to make all code paths converge at the exit block."""
                    ),
                )
            ],
        ),
    ],
).set_format_mode("define")

LValueType = Tag("LValueType", "u8")

LValue = TaggedUnion(
    name="LValue",
    tag=LValueType,
    doc=dedent(
        """\
        LValues can appear as the left hand side of an assignment.
        They are associated with a mutable storage location.
        LValues do not use SSA form since they may reference memory shared
        with other parts of the program."""
    ),
    members=[
        UnionMemberStruct(
            name="Param",
            doc="Reference to a function argument.",
            members=[
                StructMember(
                    "target", "ParamID", doc="Argument index in parameter list."
                )
            ],
        ),
        UnionMemberStruct(
            name="Closure",
            doc="Reference to a variable captured from an outer scope.",
            members=[
                StructMember(
                    "env",
                    "LocalID",
                    doc="The environment to search. Either a local variable or the function's outer environment.",
                ),
                StructMember(
                    "levels",
                    "u32",
                    doc='Levels to "go up" the environment hierarchy. 0 is the closure environment itself.',
                ),
                StructMember("index", "u32",
                             doc="Index into the environment."),
            ],
        ),
        UnionMemberStruct(
            name="Module",
            doc="Reference to a variable at module scope.",
            members=[
                StructMember(
                    "member", "ModuleMemberID", doc="ID of the module level variable."
                )
            ],
        ),
        UnionMemberStruct(
            name="Field",
            doc="Reference to the field of an object (i.e. `object.foo`).",
            members=[
                StructMember("object", "LocalID", doc="Dereferenced object."),
                StructMember("name", "InternedString",
                             doc="Field name to access."),
            ],
        ),
        UnionMemberStruct(
            name="TupleField",
            doc="Referencce to a tuple field of a tuple (i.e. `tuple.3`).",
            members=[
                StructMember("object", "LocalID",
                             doc="Dereferenced tuple object."),
                StructMember("index", "u32", doc="Index of the tuple member."),
            ],
        ),
        UnionMemberStruct(
            name="Index",
            doc="Reference to an index of an array (or a map), i.e. `thing[foo]`.",
            members=[
                StructMember("object", "LocalID",
                             doc="Dereferenced arraylike object."),
                StructMember("index", "LocalID", doc="Index into the array."),
            ],
        ),
    ],
).set_format_mode("define")

ConstantType = Tag("ConstantType", "u8")

Constant = (
    TaggedUnion(
        name="Constant",
        tag=ConstantType,
        doc="Represents a compile time constant.",
        members=[
            UnionMemberStruct("Integer", members=[
                              StructMember("value", "i64")]),
            UnionMemberAlias("Float", "FloatConstant"),
            UnionMemberStruct(
                "String", members=[StructMember("value", "InternedString")]
            ),
            UnionMemberStruct(
                "Symbol", members=[StructMember("value", "InternedString")]
            ),
            UnionMemberStruct("Null"),
            UnionMemberStruct("True"),
            UnionMemberStruct("False"),
        ],
    )
    .set_format_mode("define")
    .set_equality_mode("define")
    .set_hash_mode("define")
)

RValueType = Tag("RValueType", "u8")

RValue = TaggedUnion(
    name="RValue",
    tag=RValueType,
    doc=dedent(
        """\
        Represents an rvalue.
        RValues can be used as the right hand side of an assignment or definition.

        RValues at this compilation stage do not allow inner control flow. Nested
        language-level expressions that contain loops or conditionals are split up
        so that only "simple" expressions remain."""
    ),
    members=[
        UnionMemberStruct(
            name="UseLValue",
            doc="References an lvalue to produce a value.",
            members=[StructMember("target", "LValue",
                                  doc="Dereferenced lvalue.")],
        ),
        UnionMemberStruct(
            name="UseLocal",
            doc="References a local variable.",
            members=[StructMember("target", "LocalID",
                                  doc="Dereferenced local.")],
        ),
        UnionMemberStruct(
            name="Phi",
            doc="Phi nodes can have one of multiple locals as their value, depending on the code path that led to them.",
            members=[StructMember(
                "value", "PhiID", doc="The possible alternatives.")],
        ),
        UnionMemberStruct(
            name="Phi0",
            doc="Marker to store the fact that this local has been visited during variable resolution (used to stop recursion).",
            members=[],
        ),
        UnionMemberAlias("Constant", "tiro::Constant", doc="A constant."),
        UnionMemberStruct(
            name="OuterEnvironment",
            doc="Deferences the function's outer closure environment",
        ),
        UnionMemberStruct(
            name="BinaryOp",
            doc="Simple binary operation.",
            members=[
                StructMember("op", "BinaryOpType"),
                StructMember("left", "LocalID", doc="Left operand."),
                StructMember("right", "LocalID", doc="Right operand."),
            ],
        ),
        UnionMemberStruct(
            name="UnaryOp",
            doc="Simple unary operation.",
            members=[
                StructMember("op", "UnaryOpType"),
                StructMember("operand", "LocalID"),
            ],
        ),
        UnionMemberStruct(
            name="Call",
            doc="Function call expression, i.e. `f(a, b, c)`.",
            members=[
                StructMember("func", "LocalID", doc="Function to call."),
                StructMember(
                    "args", "LocalListID", doc="The list of function arguments."
                ),
            ],
        ),
        UnionMemberStruct(
            name="MethodHandle",
            doc=dedent(
                """\
                Represents an evaluated method access on an object, i.e. `object.method()`.
                This is a separate value in order to support left-to-right evaluation order."""
            ),
            members=[
                StructMember("instance", "LocalID",
                             doc="The object instance."),
                StructMember("method", "InternedString",
                             doc="The name of the method."),
            ],
        ),
        UnionMemberStruct(
            name="MethodCall",
            doc="Method call expression, i.e `a.b(c, d)`.",
            members=[
                StructMember(
                    "method",
                    "LocalID",
                    doc="Method to be called. Must be a method handle.",
                ),
                StructMember("args", "LocalListID",
                             doc="List of method arguments."),
            ],
        ),
        UnionMemberStruct(
            name="MakeEnvironment",
            doc="Creates a new closure environment.",
            members=[
                StructMember("parent", "LocalID",
                             doc="The parent environment."),
                StructMember(
                    "size",
                    "u32",
                    doc="The number of variable slots in the new environment.",
                ),
            ],
        ),
        UnionMemberStruct(
            name="MakeClosure",
            doc="Creates a new closure function.",
            members=[
                StructMember("env", "LocalID", doc="The closure environment."),
                StructMember(
                    "func", "LocalID", doc="The closure function's template location."
                ),
            ],
        ),
        UnionMemberStruct(
            name="Container",
            doc=dedent(
                """\
                Construct a container from the argument list,
                such as an array, a tuple or a map."""
            ),
            members=[
                StructMember(
                    "container",
                    "ContainerType",
                    doc="Container type we're going to construct.",
                ),
                StructMember(
                    "args",
                    "LocalListID",
                    doc=dedent(
                        """\
                    Arguments for the container constructor (list of elements,
                    or list of key/value-pairs in the case of Map)."""
                    ),
                ),
            ],
        ),
        UnionMemberStruct(
            name="Format",
            doc=dedent(
                """\
               Takes a list of values and formats them as a string.
               This is used to implement format string literals."""
            ),
            members=[StructMember("args", "LocalListID",
                                  doc="The list of values.")],
        ),
    ],
).set_format_mode("define")

StmtType = Tag("StmtType", "u8")

Stmt = TaggedUnion(
    name="Stmt",
    tag=StmtType,
    doc=dedent(
        """\
        Represents a statement, i.e. a single instruction inside a basic block."""
    ),
    members=[
        UnionMemberStruct(
            name="Assign",
            doc="Assigns a value to a memory location (non-SSA operations).",
            members=[
                StructMember("target", "LValue", doc="The assignment target."),
                StructMember("value", "LocalID", doc="The new value."),
            ],
        ),
        UnionMemberStruct(
            name="Define",
            doc="Defines a new local variable (SSA).",
            members=[StructMember("local", "LocalID"), ],
        ),
    ],
).set_format_mode("define")
