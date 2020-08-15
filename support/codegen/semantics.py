# Contains helper types used in the semantic analysis phase.

from .unions import Tag, Union, Struct, Alias, Field

SymbolData = (
    Union(
        name="SymbolData",
        tag=Tag("SymbolType", "u8", doc="Represents the type of a symbol."),
        doc="Stores the data associated with a symbol.",
        members=[
            Struct(
                name="Import",
                doc="Represents an imported item.",
                members=[
                    Field(
                        name="path",
                        type="InternedString",
                        doc="The imported item path.",
                    ),
                ],
            ),
            Struct(name="TypeSymbol", doc="Represents a type."),
            Struct(name="Function", doc="Represents a function item."),
            Struct(name="Parameter", doc="Represents a parameter value."),
            Struct(name="Variable", doc="Represents a variable value."),
        ],
    )
    .set_format_mode("define")
    .set_equality_mode("define")
    .set_hash_mode("define")
)
