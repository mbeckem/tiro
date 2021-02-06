# Defines helper classes used in the bytecode_gen package.
from textwrap import dedent
from .unions import Tag, Union, Struct, Alias, Field

LinkItemType = Tag(
    "LinkItemType",
    "u8",
    doc="Represents the type of an external item referenced by the bytecode.",
)

LinkItem = (
    Union(
        "LinkItem",
        tag=LinkItemType,
        doc=dedent(
            """\
            Represents an external item referenced by the bytecode.
            These references must be patched when the module is being linked."""
        ),
        members=[
            Alias(
                "Use",
                "ir::ModuleMemberId",
                doc="References a ir module member, possibly defined in another object.",
            ),
            Struct(
                name="Definition",
                doc="A definition made in the current object.",
                members=[
                    Field(
                        "ir_id",
                        "ir::ModuleMemberId",
                        doc="Id of this definition in the IR. May be invalid (for anonymous constants etc.).",
                    ),
                    Field("value", "BytecodeMember"),
                ],
            ),
        ],
    )
    .set_format_mode("define")
    .set_hash_mode("define")
    .set_equality_mode("define")
)

