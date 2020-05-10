#!/usr/bin/env python3

from textwrap import dedent
from .unions import Tag, Union, Struct, Alias, Field


class Node:
    def __init__(self, name, doc="", members=None):
        self.name = name
        self.doc = doc
        self.members = members if members is not None else []


class NodeList:
    def __init__(self, typename):
        self.typename = typename

    @property
    def cpp_type(self):
        return f"std::vector<AstPtr<{self.typename}>>"


class NodePtr:
    def __init__(self, typename):
        self.typename = typename

    @property
    def cpp_type(self):
        return f"AstPtr<{self.typename}>"


class PlainData:
    def __init__(self, typename):
        self.typename = typename

    @property
    def cpp_type(self):
        return self.typename


class PlainDataList:
    def __init__(self, typename):
        self.typename = typename

    @property
    def cpp_type(self):
        return f"std::vector<{self.typename}>"


class Member:
    def __init__(self, name, node_type, doc=""):
        self.name = name
        self.node_type = node_type
        self.doc = doc


def _is_pass_by_move(member):
    return (
        isinstance(member, NodePtr)
        or isinstance(member, NodeList)
        or isinstance(member, PlainDataList)
    )


def _map_members(nodes):
    members = []
    for node in nodes:
        struct_members = []
        for member in node.members:
            struct_member = Field(
                member.name, member.node_type.cpp_type, pass_as="move", doc=member.doc
            )
            struct_members.append(struct_member)

        member = Struct(node.name, members=struct_members, doc=node.doc)
        members.append(member)

    return members


ItemPtrType = NodePtr("AstItem")
StmtPtrType = NodePtr("AstStmt")
ExprPtrType = NodePtr("AstExpr")
StmtListType = NodeList("AstStmt")
ExprListType = NodeList("AstExpr")
PropertyType = PlainData("AstProperty")
InternedStringType = PlainData("InternedString")
AccessType = PlainData("AccessType")
FuncDeclType = PlainData("AstFuncDecl")

ITEMS = [
    Node(
        "Import",
        members=[
            Member("name", InternedStringType),
            Member("path", PlainDataList("InternedString")),
        ],
    ),
    Node("Func", members=[Member("decl", FuncDeclType)]),
    Node("Var", members=[Member("bindings", PlainDataList("AstBinding"))]),
]

BINDINGS = [
    Node(
        "Var",
        members=[
            Member("name", InternedStringType),
            Member("is_const", PlainData("bool")),
            Member("init", ExprPtrType),
        ],
    ),
    Node(
        "Tuple",
        members=[
            Member("names", PlainDataList("InternedString")),
            Member("is_const", PlainData("bool")),
            Member("init", ExprPtrType),
        ],
    ),
]

EXPRESSIONS = [
    Node("Block", members=[Member("stmts", StmtListType)]),
    Node(
        "Unary",
        members=[
            Member("operation", PlainData("UnaryOperator")),
            Member("inner", ExprPtrType),
        ],
    ),
    Node(
        "Binary",
        members=[
            Member("operation", PlainData("BinaryOperator")),
            Member("left", ExprPtrType),
            Member("right", ExprPtrType),
        ],
    ),
    Node("Var", members=[Member("name", InternedStringType)]),
    Node(
        "PropertyAccess",
        members=[
            Member("access_type", AccessType),
            Member("instance", ExprPtrType),
            Member("property", PropertyType),
        ],
    ),
    Node(
        "ElementAccess",
        members=[
            Member("access_type", AccessType),
            Member("instance", ExprPtrType),
            Member("element", ExprPtrType),
        ],
    ),
    Node(
        "Call",
        members=[
            Member("access_type", AccessType),
            Member("func", ExprPtrType),
            Member("args", ExprListType),
        ],
    ),
    Node(
        "If",
        members=[
            Member("cond", ExprPtrType),
            Member("then_branch", ExprPtrType),
            Member("else_branch", ExprPtrType),
        ],
    ),
    Node("Return", members=[Member("value", ExprPtrType)]),
    Node("Break"),
    Node("Continue"),
    Node("StringSequence", members=[Member("strings", ExprListType)]),
    Node("InterpolatedString", members=[Member("strings", ExprListType)]),
    Node("Null"),
    Node("Boolean", members=[Member("value", PlainData("bool"))]),
    Node("Integer", members=[Member("value", PlainData("i64"))]),
    Node("Float", members=[Member("value", PlainData("f64"))]),
    Node("String", members=[Member("value", InternedStringType)]),
    Node("Symbol", members=[Member("value", InternedStringType)]),
    Node("Array", members=[Member("items", ExprListType)]),
    Node("Tuple", members=[Member("items", ExprListType)]),
    Node("Set", members=[Member("items", ExprListType)]),
    Node("Map", members=[Member("keys", ExprListType), Member("values", ExprListType)]),
    Node("Func", members=[Member("decl", FuncDeclType)]),
]

STATEMENTS = [
    Node("Empty"),
    Node("Item", members=[Member("item", ItemPtrType)]),
    Node(
        "Assert", members=[Member("cond", ExprPtrType), Member("message", ExprPtrType),]
    ),
    Node("While", members=[Member("cond", ExprPtrType), Member("body", ExprPtrType),]),
    Node(
        "For",
        members=[
            Member("decl", StmtPtrType),
            Member("cond", ExprPtrType),
            Member("step", ExprPtrType),
            Member("body", ExprPtrType),
        ],
    ),
    Node("Expr", members=[Member("expr", ExprPtrType)]),
]

PropertyData = Union(
    name="AstPropertyData",
    tag=Tag("AstPropertyType", "u8"),
    doc="Represents the name of a property.",
    members=[
        Struct(
            "Field",
            doc="Represents an object field.",
            members=[Field("name", "InternedString")],
        ),
        Struct(
            "TupleField",
            doc="Represents a numeric field within a tuple.",
            members=[Field("index", "u32")],
        ),
    ],
).set_final(False)

ItemData = (
    Union(
        name="AstItemData",
        tag=Tag("AstItemType", "u8"),
        doc="Represents the contents of a toplevel item.",
        members=_map_members(ITEMS),
    )
    .set_storage_mode("movable")
    .set_final(False)
)

BindingData = (
    Union(
        name="AstBindingData",
        tag=Tag("AstBindingType", "u8"),
        doc="Represents a binding of values to names.",
        members=_map_members(BINDINGS),
    )
    .set_storage_mode("movable")
    .set_final(False)
)

ExprData = (
    Union(
        name="AstExprData",
        tag=Tag("AstExprType", "u8"),
        doc="Represents the contents of an expression in the abstract syntax tree.",
        members=_map_members(EXPRESSIONS),
    )
    .set_storage_mode("movable")
    .set_final(False)
)

StmtData = (
    Union(
        name="AstStmtData",
        tag=Tag("AstStmtType", "u8"),
        doc="Represents the contents of a statement in the abstract syntax tree.",
        members=_map_members(STATEMENTS),
    )
    .set_storage_mode("movable")
    .set_final(False)
)

TokenData = Union(
    name="TokenData",
    tag=Tag("TokenDataType", "u8"),
    doc="Represents data associated with a token.",
    members=[
        Struct(
            name="None",
            members=[],
            doc="No additional value at all (the most common case).",
        ),
        Alias("Integer", "i64", "An integer value."),
        Alias("Float", "f64", "A floating pointer value."),
        Alias("String", "InternedString", "A string value (e.g. an identifier)."),
    ],
)
