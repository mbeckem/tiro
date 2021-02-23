# Contains helper types used in the ast construction phase.

from .unions import Tag, Union, Struct, Alias, Field

from textwrap import dedent

SyntaxChild = (
    Union(
        name="SyntaxChild",
        tag=Tag("SyntaxChildType", "u8"),
        doc="Represents the child of a syntax tree node.",
        members=[
            Alias(
                name="Token",
                target="tiro::next::Token",
                doc="A token from the source code.",
            ),
            Alias(
                name="NodeId", target="tiro::next::SyntaxNodeId", doc="A node child."
            ),
        ],
    )
    .set_format_mode("define")
    .set_equality_mode("define")
)
