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

CompiledLocationType = Tag("CompiledLocationType", "u8",
                           doc="Represents the type of a compiled location.")

CompiledLocation = TaggedUnion(
    "CompiledLocation",
    tag=CompiledLocationType,
    doc=dedent("""\
        Represents a location that has been assigned to a mir value. Usually locations
        are only concerned with single local (at bytecode level). Some special cases
        exist where a virtual mir value is mapped to multiple physical locals."""),
    members=[
        UnionMemberAlias("Value",
                         target="CompiledLocalID",
                         doc="Represents a single value. This is the usual case."),
        UnionMemberStruct(
            name="Method",
            doc=dedent("""\
                Represents a method value. Two locals are needed to represent a method:
                One for the object instance and one for the actual method function value."""),
            members=[
                StructMember("instance", "CompiledLocalID",
                             doc="The 'this' argument of the method call."),
                StructMember("function", "CompiledLocalID",
                             doc="The function value."),
            ]
        )
    ]
)
