#!/usr/bin/env python3

import cog

from textwrap import dedent
from .codegen import camel_to_snake, avoid_keyword, ENV
from .unions import Tag, Union, Struct, Alias, Field


class NodeRegistry:
    def __init__(self, types):
        self.types = {}
        self.__add_all(*types)
        self.__finish()

    def get(self, name):
        if name not in self.types:
            raise RuntimeError(f"Type {repr(name)} was not defined.")
        return self.types[name]

    def __add_all(self, *definitions):
        for definition in definitions:
            name = definition.name
            if name in self.types:
                raise RuntimeError(f"Type {repr(name)} was already defined.")

            self.types[name] = definition

    def __finish(self):
        self.__resolve_references()
        self.__bind_base_classes()

    def __resolve_references(self):
        def resolve(name_or_type):
            if name_or_type is None:
                return None
            if isinstance(name_or_type, Type):
                return name_or_type
            if isinstance(name_or_type, str):
                return self.get(name_or_type)
            raise RuntimeError(f"Cannot resolve invalid type {repr(name_or_type)}.")

        for definition in self.types.values():
            definition.base = resolve(definition.base)

    def __bind_base_classes(self):
        for definition in self.types.values():
            if definition.base is not None:
                base = definition.base
                if base.final:
                    raise RuntimeError(
                        f"Cannot derive from final type {repr(base.name)}."
                    )
                base.derived.append(definition)

        for definition in self.types.values():
            definition.derived = sorted(
                definition.derived, key=lambda definition: definition.name
            )


# Initialized below, declared here as a forward declaration.
NODE_TYPES = None


class Type:
    def __init__(self, name, cpp_name=None, base=None, final=False, doc=None):
        self.name = name
        self.cpp_name = cpp_name if cpp_name is not None else name
        self.base = base
        self.final = final
        self.derived = []
        self.doc = doc


class Foreign(Type):
    def __init__(self, name, cpp_name=None, final=True):
        super().__init__(name, cpp_name=cpp_name, final=final)


class Node(Type):
    def __init__(
        self,
        name,
        base=None,
        final=True,
        doc=None,
        members=None,
        walk_order="base_first",
    ):
        super().__init__(
            name, cpp_name="Ast" + name, base=base, final=final, doc=doc,
        )
        self.members = members if members is not None else []

        if walk_order not in ["base_first", "derived_first"]:
            raise RuntimeError(f"Invalid walk order: {repr(walk_order)}")
        self.walk_order = walk_order


class Member:
    def __init__(self, name, *, required=True, doc=None, kind):
        self.kind = kind
        self.name = name
        self.required = required
        self.doc = doc

    @property
    def field_name(self):
        return self.name + "_"


class DataMember(Member):
    def __init__(self, name, data_type, required=True, doc=None):
        super().__init__(name, required=required, doc=doc, kind="data")
        self.data_type = data_type
        self.cpp_type = data_type


class DataListMember(Member):
    def __init__(self, name, element_type, required=True, doc=None):
        super().__init__(name, required=required, doc=doc, kind="data_list")
        self.element_type = element_type
        self.cpp_type = f"std::vector<{element_type}>"


class NodeMember(Member):
    def __init__(self, name, node_type, required=True, doc=None):
        super().__init__(name, required=required, doc=doc, kind="node")
        self.node_type = node_type

    @property
    def cpp_type(self):
        node_type = NODE_TYPES.get(self.node_type).cpp_name
        return f"AstPtr<{node_type}>"


class NodeListMember(Member):
    def __init__(self, name, element_type, required=True, doc=None):
        super().__init__(name, required=required, doc=doc, kind="node_list")
        self.element_type = element_type

    @property
    def cpp_type(self):
        node_type = NODE_TYPES.get(self.element_type).cpp_name
        return f"AstNodeList<{node_type}>"


NODE_TYPES = NodeRegistry(
    [
        # Root type.
        Foreign(name="Node", cpp_name="AstNode", final=False),
        # ---------------------------
        #           Bindings
        #
        Node(
            name="Binding",
            base="Node",
            final=False,
            doc="Represents a binding of one or more variables to a value",
            members=[DataMember("is_const", "bool")],
        ),
        Node(
            name="VarBinding",
            base="Binding",
            walk_order="derived_first",
            doc="Represents a variable name bound to an (optional) value.",
            members=[DataMember("name", "InternedString")],
        ),
        Node(
            name="TupleBinding",
            base="Binding",
            walk_order="derived_first",
            doc="Represents a tuple that is being unpacked into a number of variables.",
            members=[DataMember("names", "InternedStringList")],
        ),
        # ---------------------------
        #           Items
        #
        Node(
            name="Item",
            base="Node",
            final=False,
            doc="Represents the contents of a toplevel item.",
        ),
        Node(
            name="ImportItem",
            base="Item",
            doc="Represents a module import.",
            members=[
                DataMember("name", "InternedString"),
                DataListMember("path", "InternedString"),
            ],
        ),
        Node(
            name="FuncItem",
            base="Item",
            doc="Represents a function item.",
            members=[NodeMember("decl", "FuncDecl")],
        ),
        Node(
            name="VarItem",
            base="Item",
            doc="Represents a variable item.",
            members=[NodeListMember("bindings", "Binding")],
        ),
        # ---------------------------
        #           Expressions
        #
        Node("Expr", base="Node", final=False, doc="Represents a single expression."),
        Node(
            "BlockExpr",
            base="Expr",
            doc="Represents a block expression containing multiple statements.",
            members=[NodeListMember("stmts", "Stmt"),],
        ),
        Node(
            "UnaryExpr",
            base="Expr",
            doc="Represents a unary expression.",
            members=[
                DataMember("operation", "UnaryOperator"),
                NodeMember("inner", "Expr"),
            ],
        ),
        Node(
            "BinaryExpr",
            base="Expr",
            doc="Represents a binary expression.",
            members=[
                DataMember("operation", "BinaryOperator"),
                NodeMember("left", "Expr"),
                NodeMember("right", "Expr"),
            ],
        ),
        Node(
            "StringExpr",
            base="Expr",
            doc="Represents a string expression consisting of literal strings and formatted sub expressions.",
            members=[NodeListMember("items", "Expr")],
        ),
        Node(
            "FuncExpr",
            base="Expr",
            doc="Represents a function expression.",
            members=[NodeMember("decl", "FuncDecl")],
        ),
        Node(
            "VarExpr",
            base="Expr",
            doc="Represents a reference to a variable.",
            members=[DataMember("name", "InternedString")],
        ),
        Node(
            "PropertyExpr",
            base="Expr",
            doc="Represents an access to an object property.",
            members=[
                DataMember("access_type", "AccessType"),
                NodeMember("instance", "Expr"),
                DataMember("property", "AstProperty"),
            ],
        ),
        Node(
            "ElementExpr",
            base="Expr",
            doc="Represents an access to a container element.",
            members=[
                DataMember("access_type", "AccessType"),
                NodeMember("instance", "Expr"),
                NodeMember("element", "Expr"),
            ],
        ),
        Node(
            "CallExpr",
            base="Expr",
            doc="Represents a function call expression.",
            members=[
                DataMember("access_type", "AccessType"),
                NodeMember("func", "Expr"),
                NodeListMember("args", "Expr"),
            ],
        ),
        Node(
            "IfExpr",
            base="Expr",
            doc="Represents an if expression.",
            members=[
                NodeMember("cond", "Expr"),
                NodeMember("then_branch", "Expr"),
                NodeMember("else_branch", "Expr", required=False),
            ],
        ),
        Node(
            "ReturnExpr",
            base="Expr",
            doc="Represents a return expression with an optional return value.",
            members=[NodeMember("value", "Expr", required=False)],
        ),
        Node(
            "BreakExpr", base="Expr", doc="Represents a break expression within a loop."
        ),
        Node(
            "ContinueExpr",
            base="Expr",
            doc="Represents a continue expression within a loop.",
        ),
        # ---------------------------
        #           Literals
        #
        Node("Literal", base="Expr", final=False, doc="Represents a literal value."),
        Node("NullLiteral", base="Literal", doc="Represents a null literal."),
        Node(
            "BooleanLiteral",
            base="Literal",
            doc="Represents a boolean literal.",
            members=[DataMember("value", "bool")],
        ),
        Node(
            "IntegerLiteral",
            base="Literal",
            doc="Represents an integer literal.",
            members=[DataMember("value", "i64")],
        ),
        Node(
            "FloatLiteral",
            base="Literal",
            doc="Represents a floating point literal.",
            members=[DataMember("value", "f64")],
        ),
        Node(
            "StringLiteral",
            base="Literal",
            doc="Represents a string literal.",
            members=[DataMember("value", "InternedString")],
        ),
        Node(
            "SymbolLiteral",
            base="Literal",
            doc="Represents a symbol.",
            members=[DataMember("value", "InternedString")],
        ),
        Node(
            "ArrayLiteral",
            base="Literal",
            doc="Represents an array expression.",
            members=[NodeListMember("items", "Expr")],
        ),
        Node(
            "TupleLiteral",
            base="Literal",
            doc="Represents a tuple expression.",
            members=[NodeListMember("items", "Expr")],
        ),
        Node(
            "SetLiteral",
            base="Literal",
            doc="Represents a set expression.",
            members=[NodeListMember("items", "Expr")],
        ),
        Node(
            "MapLiteral",
            base="Literal",
            doc="Represents a map expression.",
            members=[NodeListMember("items", "MapItem")],
        ),
        # ---------------------------
        #           Statements
        #
        Node("Stmt", base="Node", final=False, doc="Represents a statement."),
        Node("EmptyStmt", base="Stmt", doc="Represents an empty statement."),
        Node(
            "ItemStmt",
            base="Stmt",
            doc="Represents an item in a statement context.",
            members=[NodeMember("item", "Item")],
        ),
        Node(
            "AssertStmt",
            base="Stmt",
            doc="Represents an assert statement with an optional message.",
            members=[
                NodeMember("cond", "Expr"),
                NodeMember("message", "Expr", required=False),
            ],
        ),
        Node(
            "WhileStmt",
            base="Stmt",
            doc="Represents a while loop.",
            members=[NodeMember("cond", "Expr"), NodeMember("body", "Expr"),],
        ),
        Node(
            "ForStmt",
            base="Stmt",
            doc="Represents a for loop.",
            members=[
                NodeMember("decl", "VarItem", required=False),
                NodeMember("cond", "Expr", required=False),
                NodeMember("step", "Expr", required=False),
                NodeMember("body", "Expr"),
            ],
        ),
        # ---------------------------
        #           Misc Nodes
        #
        Node(
            "MapItem",
            base="Node",
            doc="Represents a key-value pair in a map expression.",
            members=[NodeMember("key", "Expr"), NodeMember("value", "Expr"),],
        ),
        Node(
            "FuncDecl",
            base="Node",
            doc="Represents a function declaration.",
            members=[
                DataMember("name", "InternedString"),
                NodeListMember("params", "ParamDecl"),
                NodeMember("body", "Expr"),
            ],
        ),
        Node(
            "ParamDecl",
            base="Node",
            doc="Represents a function parameter declaration.",
            members=[DataMember("name", "InternedString")],
        ),
    ]
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

Property = Union(
    name="AstProperty",
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
)


def _visit_member(member, visitor):
    visit = getattr(visitor, f"visit_{member.kind}")
    return visit(member)


def walk_types(base=None):
    if base is None:
        base = NODE_TYPES.get("Node")

    yield base
    if not base.final:
        for child in base.derived:
            yield from walk_types(child)


def walk_concrete_types(base=None):
    for node_type in walk_types(base):
        if node_type.final:
            yield node_type


def type_tags():
    tags = []
    next_value = 1

    def visit(node):
        nonlocal next_value, tags
        if node.final:
            value = next_value
            next_value += 1
            tags.append((node.name, value))
            return (value, value)

        first = None
        last = None
        for child in node.derived:
            (low, high) = visit(child)
            first = low if first is None else min(first, low)
            last = high if last is None else max(last, high)

        if first is None or last is None:
            raise RuntimeError(f"Base class without any children: {node.name}.")

        tags.append((f"First{node.name}", first))
        tags.append((f"Last{node.name}", last))
        return (first, last)

    visit(NODE_TYPES.get("Node"))
    return tags


def declare(*node_types):
    for node_type in node_types:
        cog.outl(node_type.cpp_name)


def define(*node_types):
    templ = ENV.get_template("ast.jinja2")
    for index, node_type in enumerate(node_types):
        if index > 0:
            cog.outl()

        cog.outl(templ.module.node_def(node_type))
