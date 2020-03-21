#!/usr/bin/env python3
from unions import Tag, TaggedUnion, UnionMemberStruct, UnionMemberAlias, StructMember
from textwrap import dedent

LinkItemType = Tag("LinkItemType", "u8",
    doc="Represents the type of an external item referenced by the bytecode.")

LinkItem = TaggedUnion(
    "LinkItem",
    tag=LinkItemType,
    doc=dedent("""\
        Represents an external item referenced by the bytecode.
        These references must be patched when the module is being linked."""),
    members=[
        UnionMemberAlias("Use", "ModuleMemberID",
            doc="References a mir module member, possibly defined in another object."),
        UnionMemberStruct(
            name="Definition",
            doc="A definition made in the current object.",
            members=[
                StructMember(
                    "mir_id", "ModuleMemberID",
                    doc="ID of this definition in the MIR. May be invalid (for anonymous constants etc.)."),
                StructMember("value", "CompiledModuleMember")
            ]
        )
    ]
) \
    .set_format_mode("define") \
    .set_hash_mode("define") \
    .set_equality_mode("define")
