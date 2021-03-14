from textwrap import dedent
from .unions import Tag, Union, Struct, Alias, Field

ParserEvent = (
    Union(
        name="ParserEvent",
        tag=Tag("ParserEventType", "u8", doc="Represents the type of a ParserEvent."),
        doc=dedent(
            """\
            ParserEvents are emitted by the parser in order to start and finish nodes
            or to add tokens to the current node.
            
            Events are emitted as a simple stream of values that form an implicit tree structure.
            This design is inspired by the Kotlin compiler and the rust-analyzer project."""
        ),
        members=[
            Struct(
                "Tombstone",
                doc=dedent(
                    """\
                    This event does nothing. The following events are added to the current node instead.
                    Tombstones are used before the type of a node is known of when syntax nodes become abandoned."""
                ),
            ),
            Struct(
                "Start",
                doc=dedent(
                    """\
                    Marks the start of a syntax node. Every start event is followed by a matching finish event.

                    Some syntax rules will emit a parent node *after* the child has been observed.
                    This is the case, for example, in function call expressions like `EXPR(ARGS...)` where
                    a new function call node becomes the parent of the fully parsed EXPR node.
                    
                    To enable this pattern, every start event may have a `forward_parent` pointing to a later parent node's
                    start event using its index.
                    Nodes that do not need a forward parent leave its value at `0`, which is never a valid index for a forward parent."""
                ),
                members=[
                    Field(
                        name="type", type="SyntaxType", doc="The node's syntax type."
                    ),
                    Field(
                        name="forward_parent",
                        type="size_t",
                        doc="The forward parent node's index, or 0 if there is none.",
                    ),
                ],
            ),
            Struct("Finish", doc="The finish event ends the current node."),
            Alias(
                "Token",
                "tiro::Token",
                doc="Tokens emitted between the start and finish events of a node belong to that node.",
            ),
            Struct(
                "Error",
                doc="Represents an error encountered while parsing the current node",
                members=[
                    Field(
                        name="message",
                        type="std::string",
                        doc="The error message. TODO: message class / message ids.",
                        pass_as="move",
                    ),
                ],
            ),
        ],
    )
    .set_format_mode("define")
    .set_accessors("all")
    .set_storage_mode("movable")
)

SyntaxChild = (
    Union(
        name="SyntaxChild",
        tag=Tag("SyntaxChildType", "u8"),
        doc="Represents the child of a syntax tree node.",
        members=[
            Alias(
                name="Token", target="tiro::Token", doc="A token from the source code.",
            ),
            Alias(name="NodeId", target="tiro::SyntaxNodeId", doc="A node child."),
        ],
    )
    .set_format_mode("define")
    .set_equality_mode("define")
)
