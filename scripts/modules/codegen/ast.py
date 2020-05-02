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
        return f"std::vector<ASTPtr<{self.typename}>>"


class NodePtr:
    def __init__(self, typename):
        self.typename = typename

    @property
    def cpp_type(self):
        return f"ASTPtr<{self.typename}>"


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


def map_members(nodes):
    members = []
    for node in nodes:
        struct_members = []
        for member in node.members:
            pass_as = "move" if _is_pass_by_move(member.node_type) else "copy"
            struct_member = Field(
                member.name, member.node_type.cpp_type, pass_as=pass_as, doc=member.doc
            )
            struct_members.append(struct_member)

        member = Struct(node.name, members=struct_members, doc=node.doc)
        members.append(member)

    return members


StmtPtrType = NodePtr("ASTStmt")
ExprPtrType = NodePtr("ASTExpr")
DeclPtrType = NodePtr("ASTDecl")
StmtListType = NodeList("ASTStmt")
ExprListType = NodeList("ASTExpr")
DeclListType = NodeList("ASTDecl")
InternedStringType = PlainData("InternedString")
AccessType = PlainData("AccessType")
PropertyType = PlainData("ASTProperty")

NODES = {}

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
            Member("element", PlainData("u32")),
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
    Node("Func", members=[Member("decl", DeclPtrType)]),
]

DECLARATIONS = [
    Node(
        "Func",
        members=[
            Member("name", InternedStringType),
            Member("params", DeclListType),
            Member("body", ExprPtrType),
            Member("body_is_value", PlainData("bool")),
        ],
    ),
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
    Node("Import", members=[Member("path", PlainDataList("InternedString")),]),
]

STATEMENTS = [
    Node("Empty"),
    Node(
        "Assert", members=[Member("cond", ExprPtrType), Member("message", ExprPtrType),]
    ),
    Node("Decl", members=[Member("decls", DeclListType)]),
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

for kind, items in [
    ("expr", EXPRESSIONS),
    ("stmt", STATEMENTS),
    ("decl", DECLARATIONS),
]:
    for item in items:
        NODES[(kind, item.name)] = item

PropertyType = Tag("ASTPropertyType", "u8")
Property = Union(
    name="ASTProperty",
    tag=PropertyType,
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
).set_format_mode("define")

ExprType = Tag("ASTExprType", "u8")
ExprData = (
    Union(
        name="ASTExprData",
        tag=ExprType,
        doc="Represents the contents of an expression in the abstract syntax tree.",
        members=map_members(EXPRESSIONS),
    )
    .set_format_mode("define")
    .set_storage_mode("movable")
)

StmtType = Tag("ASTStmtType", "u8")
StmtData = (
    Union(
        name="ASTStmtData",
        tag=StmtType,
        doc="Represents the contents of a statement in the abstract syntax tree.",
        members=map_members(STATEMENTS),
    )
    .set_format_mode("define")
    .set_storage_mode("movable")
)

DeclType = Tag("ASTDeclType", "u8")
DeclData = (
    Union(
        name="ASTDeclData",
        tag=DeclType,
        doc="Represents the contents of a declaration in the abstract syntax tree.",
        members=map_members(DECLARATIONS),
    )
    .set_format_mode("define")
    .set_storage_mode("movable")
)
