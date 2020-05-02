#!/usr/bin/env python3
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
                "ModuleMemberID",
                doc="References a ir module member, possibly defined in another object.",
            ),
            Struct(
                name="Definition",
                doc="A definition made in the current object.",
                members=[
                    Field(
                        "ir_id",
                        "ModuleMemberID",
                        doc="ID of this definition in the IR. May be invalid (for anonymous constants etc.).",
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

BytecodeLocationType = Tag(
    "BytecodeLocationType", "u8", doc="Represents the type of a compiled location."
)

# TODO: Make this a short array of registers.
BytecodeLocation = Union(
    "BytecodeLocation",
    tag=BytecodeLocationType,
    doc=dedent(
        """\
        Represents a location that has been assigned to a ir value. Usually locations
        are only concerned with single local (at bytecode level). Some special cases
        exist where a virtual ir value is mapped to multiple physical locals."""
    ),
    members=[
        Alias(
            "Value",
            target="BytecodeRegister",
            doc="Represents a single value. This is the usual case.",
        ),
        Struct(
            name="Method",
            doc=dedent(
                """\
                Represents a method value. Two locals are needed to represent a method:
                One for the object instance and one for the actual method function value."""
            ),
            members=[
                Field(
                    "instance",
                    "BytecodeRegister",
                    doc="The 'this' argument of the method call.",
                ),
                Field("function", "BytecodeRegister", doc="The function value."),
            ],
        ),
    ],
).set_equality_mode("define")
