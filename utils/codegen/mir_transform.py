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
            target="mir::Constant",
            doc="A known constant."
        ),
        UnionMemberStruct(
            name="UnaryOp",
            doc="The known result of a unary operation.",
            members=[
                StructMember(
                    "op", "mir::UnaryOpType", doc="The unary operator."
                ),
                StructMember(
                    "operand", "mir::LocalID", doc="The operand value."
                ),
            ]
        ),
        UnionMemberStruct(
            name="BinaryOp",
            doc="The known result of a binary operation.",
            members=[
                StructMember(
                    "op", "mir::BinaryOpType", doc="The binary operator."
                ),
                StructMember(
                    "left", "mir::LocalID", doc="The left operand."
                ),
                StructMember(
                    "right", "mir::LocalID", doc="The right operand."
                ),
            ]
        ),
    ]
).set_format_mode("define") \
 .set_equality_mode("define") \
 .set_hash_mode("define")
