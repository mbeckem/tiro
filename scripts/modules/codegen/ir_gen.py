# Contains helper types used in the ir construction phase.

from .unions import Tag, Union, Struct, Alias, Field

ComputedValueType = Tag("ComputedValueType", "u8")

ComputedValue = (
    Union(
        name="ComputedValue",
        tag=ComputedValueType,
        doc="Represents a reusable local variable for a certain operation.",
        members=[
            Alias(name="Constant", target="tiro::Constant", doc="A known constant."),
            Struct(
                name="UnaryOp",
                doc="The known result of a unary operation.",
                members=[
                    Field("op", "UnaryOpType", doc="The unary operator."),
                    Field("operand", "LocalId", doc="The operand value."),
                ],
            ),
            Struct(
                name="BinaryOp",
                doc="The known result of a binary operation.",
                members=[
                    Field("op", "BinaryOpType", doc="The binary operator."),
                    Field("left", "LocalId", doc="The left operand."),
                    Field("right", "LocalId", doc="The right operand."),
                ],
            ),
        ],
    )
    .set_format_mode("define")
    .set_equality_mode("define")
    .set_hash_mode("define")
)

AssignTargetType = Tag("AssignTargetType", "u8")

AssignTarget = Union(
    name="AssignTarget",
    tag=AssignTargetType,
    doc="Represents the left hand side of an assignment during compilation.",
    members=[
        Alias(name="LValue", target="tiro::LValue", doc="An ir lvalue"),
        Alias(
            name="Symbol", target="NotNull<tiro::Symbol*>", doc="Represents a symbol."
        ),
    ],
)
