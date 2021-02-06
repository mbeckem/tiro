# Contains helper types used in the ir construction phase.

from .unions import Tag, Union, Struct, Alias, Field

from textwrap import dedent

ComputedValue = (
    Union(
        name="ComputedValue",
        tag=Tag("ComputedValueType", "u8"),
        doc="Represents a reusable value defined by an instruction.",
        members=[
            Alias(
                name="Constant", target="tiro::ir::Constant", doc="A known constant."
            ),
            Alias(
                name="ModuleMemberId",
                target="tiro::ir::ModuleMemberId",
                doc=dedent(
                    """\
                    A cached read targeting a module member.
                    Only makes sense for constant values."""
                ),
            ),
            Struct(
                name="UnaryOp",
                doc="The known result of a unary operation.",
                members=[
                    Field("op", "UnaryOpType", doc="The unary operator."),
                    Field("operand", "InstId", doc="The operand value."),
                ],
            ),
            Struct(
                name="BinaryOp",
                doc="The known result of a binary operation.",
                members=[
                    Field("op", "BinaryOpType", doc="The binary operator."),
                    Field("left", "InstId", doc="The left operand."),
                    Field("right", "InstId", doc="The right operand."),
                ],
            ),
            Struct(
                name="AggregateMemberRead",
                doc="A cached read access to an aggregate's member.",
                members=[
                    Field("aggregate", "InstId", doc="The aggregate instance."),
                    Field("member", "AggregateMember", doc="The accessed member."),
                ],
            ),
        ],
    )
    .set_format_mode("define")
    .set_equality_mode("define")
    .set_hash_mode("define")
)

AssignTarget = Union(
    name="AssignTarget",
    tag=Tag("AssignTargetType", "u8"),
    doc="Represents the left hand side of an assignment during compilation.",
    members=[
        Alias(name="LValue", target="tiro::ir::LValue", doc="An ir lvalue"),
        Alias(name="Symbol", target="tiro::SymbolId", doc="Represents a symbol."),
    ],
)

Region = (
    Union(
        name="Region",
        tag=Tag("RegionType", "u8"),
        doc=dedent(
            """\
            Represents the data associated with a nested region.
            
            NOTE: Make sure to only use (de-facto) noexcept movable types in this class."""
        ),
        members=[
            Struct(
                name="Loop",
                doc="Represents an active loop.",
                members=[
                    Field(
                        "jump_break",
                        "BlockId",
                        doc="Target block for the `break` expression.",
                    ),
                    Field(
                        "jump_continue",
                        "BlockId",
                        doc="Target block for the `continue` expression.",
                    ),
                ],
            ),
            Struct(
                name="Scope",
                doc="Represents a block scope.",
                members=[
                    Field(
                        "original_handler",
                        "BlockId",
                        doc="The original exception handler when this scope was entered.",
                    ),
                    Field(
                        "processed",
                        "u32",
                        doc=dedent(
                            """\
                            Signals already completed deferred executions to recursive scope exit invocations.
                            This is important when nested control flow instructions are encountered while
                            evaluating deferred statements."""
                        ),
                    ),
                    Field(
                        "deferred",
                        "absl::InlinedVector<std::pair<NotNull<AstExpr*>, BlockId>, 3>",
                        pass_as="move",
                        doc=dedent(
                            """\
                            Deferred expressions that must be evaluated on normal (non-exceptional)
                            scope exit, e.g. return or break."""
                        ),
                    ),
                ],
            ).set_force_noexcept(True),
        ],
    )
    .set_storage_mode("movable")
    .set_accessors("all")
)
