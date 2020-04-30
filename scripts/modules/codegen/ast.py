#!/usr/bin/env python3

from textwrap import dedent
from .unions import Tag, TaggedUnion, UnionMemberStruct, UnionMemberAlias, StructMember

ExprType = Tag("ASTExprType", "u8")

Expr = TaggedUnion(
    name="ASTExprData",
    tag=ExprType,
    doc="Represents the contents of an expression in the abstract syntax tree.",
    members=[
        UnionMemberStruct(
            name="Test",
            doc="TODO.",
            members=[
                StructMember(
                    "todo", "int", doc="TODO."
                )
            ],
        ),
    ],
) \
    .set_format_mode("define") \
    .set_storage_mode("movable")
