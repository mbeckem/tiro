#!/usr/bin/env python3

from unions import Tag, TaggedUnion, UnionMemberStruct, UnionMemberAlias, StructMember

ComputedValueType = Tag("ComputedValueType", "u8")

ComputedValue = TaggedUnion(
    name="ComputedValue",
    tag=ComputedValueType,
    doc="Represents a reusable local variable for a certain operation.",
    members=[
        UnionMemberAlias(
            name="Constant",
            target="tiro::Constant",
            doc="A known constant."
        ),
        UnionMemberStruct(
            name="UnaryOp",
            doc="The known result of a unary operation.",
            members=[
                StructMember(
                    "op", "UnaryOpType", doc="The unary operator."
                ),
                StructMember(
                    "operand", "LocalID", doc="The operand value."
                ),
            ]
        ),
        UnionMemberStruct(
            name="BinaryOp",
            doc="The known result of a binary operation.",
            members=[
                StructMember(
                    "op", "BinaryOpType", doc="The binary operator."
                ),
                StructMember(
                    "left", "LocalID", doc="The left operand."
                ),
                StructMember(
                    "right", "LocalID", doc="The right operand."
                ),
            ]
        ),
    ]
).set_format_mode("define") \
 .set_equality_mode("define") \
 .set_hash_mode("define")

AssignTargetType = Tag("AssignTargetType", "u8")

AssignTarget = TaggedUnion(
    name="AssignTarget",
    tag=AssignTargetType,
    doc="Represents the left hand side of an assignment during compilation.",
    members=[
        UnionMemberAlias(
            name="LValue",
            target="tiro::LValue",
            doc="An ir lvalue"
        ),
        UnionMemberAlias(
            name="Symbol",
            target="NotNull<tiro::Symbol*>",
            doc="Represents a symbol."
        )
    ]
)
