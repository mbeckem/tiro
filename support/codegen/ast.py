# This file defines the structure of the compiler's abstract syntax tree.
# All node types listed here serve as prototypes for generated C++ classes.

import cog

from textwrap import dedent
from .codegen import camel_to_snake, avoid_keyword, ENV
from .unions import Tag, Union, Struct, Alias, Field


class NodeRegistry:
    "A collection of node types that can reference each other."

    def __init__(self, types):
        self.types = {}
        self.__add_all(*types)
        self.__finish()

    def get(self, name):
        "Returns the node type with the given name."

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

            if isinstance(definition, Node):
                for member in definition.members:
                    if isinstance(member, NodeMember):
                        member.node_type = resolve(member.node_type)
                    if isinstance(member, NodeListMember):
                        member.element_type = resolve(member.element_type)

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
    "Represents a type registered with a node registry instance."

    def __init__(self, name, cpp_name=None, base=None, final=False, doc=None):
        self.name = name
        self.cpp_name = cpp_name if cpp_name is not None else name
        self.base = base
        self.final = final
        self.derived = []
        self.doc = doc


class RootNode(Type):
    def __init__(self, name, cpp_name=None):
        super().__init__(name, cpp_name=cpp_name, final=False)
        self.members = []
        self.walk_order = "base_first"
        self.required_members = []
        self.visitor_name = "visit_node"


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

    @property
    def required_members(self):
        return [member for member in self.members if member.required]

    @property
    def visitor_name(self):
        return f"visit_{camel_to_snake(self.name)}"


class Member:
    "Represents a member (i.e. field) of a node type."

    def __init__(self, name, *, required=True, doc=None, kind):
        self.kind = kind
        self.name = name
        self.required = required
        self.doc = doc

    @property
    def field_name(self):
        return self.name + "_"


class DataMember(Member):
    def __init__(self, name, data_type, required=True, simple=False, doc=None):
        super().__init__(name, required=required, doc=doc, kind="data")
        self.data_type = data_type
        self.cpp_type = data_type
        self.simple = simple


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
    def visitor_name(self):
        return self.node_type.visitor_name

    @property
    def cpp_type(self):
        return f"AstPtr<{self.node_type.cpp_name}>"


class NodeListMember(Member):
    def __init__(self, name, element_type, required=True, doc=None):
        super().__init__(name, required=required, doc=doc, kind="node_list")
        self.element_type = element_type

    @property
    def visitor_name(self):
        return f"{self.element_type.visitor_name}_list"

    @property
    def cpp_type(self):
        return f"AstNodeList<{self.element_type.cpp_name}>"


NODE_TYPES = NodeRegistry(
    [
        # Root type.
        RootNode(name="Node", cpp_name="AstNode"),
        # ---------------------------
        #           Statements
        #
        Node("Stmt", base="Node", final=False, doc="Represents a statement."),
        Node("EmptyStmt", base="Stmt", doc="Represents an empty statement."),
        Node(
            "AssertStmt",
            base="Stmt",
            doc="Represents an assert statement with an optional message.",
            members=[
                NodeMember("cond", "Expr", required=False),
                NodeMember("message", "Expr", required=False),
            ],
        ),
        Node(
            "WhileStmt",
            base="Stmt",
            doc="Represents a while loop.",
            members=[
                NodeMember("cond", "Expr", required=False),
                NodeMember("body", "Expr", required=False),
            ],
        ),
        Node(
            "ForStmt",
            base="Stmt",
            doc="Represents a for loop.",
            members=[
                NodeMember("decl", "VarDecl", required=False),
                NodeMember("cond", "Expr", required=False),
                NodeMember("step", "Expr", required=False),
                NodeMember("body", "Expr", required=False),
            ],
        ),
        Node(
            "ForEachStmt",
            base="Stmt",
            doc="Represents a for each loop.",
            members=[
                NodeMember("spec", "BindingSpec", required=False),
                NodeMember("expr", "Expr", required=False),
                NodeMember("body", "Expr", required=False),
            ],
        ),
        Node(
            name="DeclStmt",
            base="Stmt",
            doc="Represents a declaration in a statement context.",
            members=[NodeMember("decl", "Decl", required=False)],
        ),
        Node(
            name="DeferStmt",
            base="Stmt",
            doc="Represents an expression that will be evaluated on scope exit.",
            members=[NodeMember("expr", "Expr", required=False)],
        ),
        Node(
            "ExprStmt",
            base="Stmt",
            doc="Represents an expression in a statement context.",
            members=[NodeMember("expr", "Expr", required=False)],
        ),
        # ---------------------------
        #           Declarations
        #
        Node(
            "Decl",
            base="Node",
            final=False,
            doc="Represents a declaration.",
            members=[
                NodeListMember(
                    "modifiers",
                    "Modifier",
                    required=False,
                    doc="A set of modifiers for this declaration.",
                )
            ],
        ),
        Node(
            "ImportDecl",
            base="Decl",
            doc="Represents a module import.",
            members=[
                DataMember("name", "InternedString", simple=True, required=False),
                DataListMember("path", "InternedString", required=False),
            ],
        ),
        Node(
            name="VarDecl",
            base="Decl",
            doc="Represents the declaration of a number of variables.",
            members=[NodeListMember("bindings", "Binding", required=False)],
        ),
        Node(
            "FuncDecl",
            base="Decl",
            doc="Represents a function declaration.",
            members=[
                DataMember("name", "InternedString", simple=True, required=False),
                DataMember("body_is_value", "bool", simple=True, required=False),
                NodeListMember("params", "ParamDecl", required=False),
                NodeMember("body", "Expr", required=False),
            ],
        ),
        Node(
            "ParamDecl",
            base="Decl",
            doc="Represents a function parameter declaration.",
            members=[DataMember("name", "InternedString", simple=True, required=False)],
        ),
        Node("Modifier", base="Node", final=False, doc="Represents a item modifier."),
        Node("ExportModifier", base="Modifier", doc="Represents an export modifier."),
        # ---------------------------
        #           Bindings
        #
        Node(
            name="Binding",
            base="Node",
            final=True,
            doc="Represents a binding of one or more variables to a value",
            members=[
                DataMember("is_const", "bool", simple=True),
                NodeMember("spec", "BindingSpec", required=False),
                NodeMember("init", "Expr", required=False),
            ],
        ),
        Node(
            "BindingSpec",
            "Node",
            final=False,
            doc="Represents the variable specifiers in the left hand side of a binding.",
        ),
        Node(
            name="VarBindingSpec",
            base="BindingSpec",
            doc="Represents a variable name bound to an (optional) value.",
            members=[NodeMember("name", "StringIdentifier", required=False)],
        ),
        Node(
            name="TupleBindingSpec",
            base="BindingSpec",
            doc="Represents a tuple that is being unpacked into a number of variables.",
            members=[NodeListMember("names", "StringIdentifier", required=False)],
        ),
        # ---------------------------
        #           Expressions
        #
        Node("Expr", base="Node", final=False, doc="Represents a single expression."),
        Node(
            "BlockExpr",
            base="Expr",
            doc="Represents a block expression containing multiple statements.",
            members=[NodeListMember("stmts", "Stmt", required=False)],
        ),
        Node(
            "UnaryExpr",
            base="Expr",
            doc="Represents a unary expression.",
            members=[
                DataMember("operation", "UnaryOperator", simple=True),
                NodeMember("inner", "Expr", required=False),
            ],
        ),
        Node(
            "BinaryExpr",
            base="Expr",
            doc="Represents a binary expression.",
            members=[
                DataMember("operation", "BinaryOperator", simple=True),
                NodeMember("left", "Expr", required=False),
                NodeMember("right", "Expr", required=False),
            ],
        ),
        Node(
            "StringGroupExpr",
            base="Expr",
            doc="Represents a sequence of adjacent string expressions.",
            members=[NodeListMember("strings", "StringExpr", required=False)],
        ),
        Node(
            "StringExpr",
            base="Expr",
            doc="Represents a string expression consisting of literal strings and formatted sub expressions.",
            members=[NodeListMember("items", "Expr", required=False)],
        ),
        Node(
            "FuncExpr",
            base="Expr",
            doc="Represents a function expression.",
            members=[NodeMember("decl", "FuncDecl", required=False)],
        ),
        Node(
            "VarExpr",
            base="Expr",
            doc="Represents a reference to a variable.",
            members=[DataMember("name", "InternedString", simple=True)],
        ),
        Node(
            "PropertyExpr",
            base="Expr",
            doc="Represents an access to an object property.",
            members=[
                DataMember("access_type", "AccessType", simple=True),
                NodeMember("instance", "Expr", required=False),
                NodeMember("property", "Identifier", required=False),
            ],
        ),
        Node(
            "ElementExpr",
            base="Expr",
            doc="Represents an access to a container element.",
            members=[
                DataMember("access_type", "AccessType", simple=True),
                NodeMember("instance", "Expr", required=False),
                NodeMember("element", "Expr", required=False),
            ],
        ),
        Node(
            "CallExpr",
            base="Expr",
            doc="Represents a function call expression.",
            members=[
                DataMember("access_type", "AccessType", simple=True),
                NodeMember("func", "Expr", required=False),
                NodeListMember("args", "Expr", required=False),
            ],
        ),
        Node(
            "IfExpr",
            base="Expr",
            doc="Represents an if expression.",
            members=[
                NodeMember("cond", "Expr", required=False),
                NodeMember("then_branch", "Expr", required=False),
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
            members=[DataMember("value", "bool", simple=True)],
        ),
        Node(
            "IntegerLiteral",
            base="Literal",
            doc="Represents an integer literal.",
            members=[DataMember("value", "i64", simple=True)],
        ),
        Node(
            "FloatLiteral",
            base="Literal",
            doc="Represents a floating point literal.",
            members=[DataMember("value", "f64", simple=True)],
        ),
        Node(
            "StringLiteral",
            base="Literal",
            doc="Represents a string literal.",
            members=[DataMember("value", "InternedString", simple=True)],
        ),
        Node(
            "SymbolLiteral",
            base="Literal",
            doc="Represents a symbol.",
            members=[DataMember("value", "InternedString", simple=True)],
        ),
        Node(
            "ArrayLiteral",
            base="Literal",
            doc="Represents an array expression.",
            members=[NodeListMember("items", "Expr", required=False)],
        ),
        Node(
            "TupleLiteral",
            base="Literal",
            doc="Represents a tuple expression.",
            members=[NodeListMember("items", "Expr", required=False)],
        ),
        Node(
            "SetLiteral",
            base="Literal",
            doc="Represents a set expression.",
            members=[NodeListMember("items", "Expr", required=False)],
        ),
        Node(
            "MapLiteral",
            base="Literal",
            doc="Represents a map expression.",
            members=[NodeListMember("items", "MapItem", required=False)],
        ),
        # ---------------------------
        #           Misc Nodes
        #
        Node(
            "File",
            base="Node",
            doc="Represents the contents of a file.",
            members=[
                NodeListMember(
                    "items",
                    "Stmt",
                    required=False,
                    doc="The top level statements in this file.",
                )
            ],
        ),
        Node(
            "MapItem",
            base="Node",
            doc="Represents a key-value pair in a map expression.",
            members=[
                NodeMember("key", "Expr", required=False),
                NodeMember("value", "Expr", required=False),
            ],
        ),
        Node(
            "Identifier",
            base="Node",
            final=False,
            doc="Represents an identifier in a property access expression.",
        ),
        Node(
            "StringIdentifier",
            base="Identifier",
            doc="Represents the name of a variable or a field.",
            members=[DataMember("value", "InternedString", simple=True)],
        ),
        Node(
            "NumericIdentifier",
            base="Identifier",
            doc="Represents an integer literal in an identifier context (such as a tuple member expression).",
            members=[DataMember("value", "u32", simple=True)],
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
        Alias("Integer", "i64", doc="An integer value."),
        Alias("Float", "f64", doc="A floating pointer value."),
        Alias("String", "InternedString", doc="A string value (e.g. an identifier)."),
    ],
)


def __walk_types(base):
    if base is None:
        return

    yield base
    if not base.final:
        for child in base.derived:
            yield from __walk_types(child)


def walk_types(*bases):
    """Visit the type hierarchies rooted at the given base types.
    Defaults to the root node type if base is not specified."""

    if not bases:
        bases = [NODE_TYPES.get("Node")]

    for base in bases:
        yield from __walk_types(base)


def walk_concrete_types(*bases):
    "Visits concrete types of the type hierarchies rooted at the given base types."

    for node_type in walk_types(*bases):
        if node_type.final:
            yield node_type


def type_tags():
    """Returns a list of type tags (i.e. (Name, EnumValue) pairs) 
    that can be used to identify node types on the C++ side."""

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


def slot_types():
    """Returns member types that are being used as children. These must be supported by the AST visitor."""

    member_seen = set()
    member_types = list()
    for node_type in walk_types():
        for member in node_type.members:
            member_key = member.cpp_type
            is_child = isinstance(member, NodeMember) or isinstance(
                member, NodeListMember
            )
            if (member_key not in member_seen) and is_child:
                member_seen.add(member_key)
                member_types.append(member)

    return sorted(member_types, key=lambda member: member.cpp_type)


def declare(*node_types):
    for node_type in node_types:
        cog.outl(node_type.cpp_name)


def define(*node_types):
    templ = ENV.get_template("ast.jinja2")
    for index, node_type in enumerate(node_types):
        if index > 0:
            cog.outl()

        cog.outl(templ.module.node_def(node_type))


def implement(*node_types):
    templ = ENV.get_template("ast.jinja2")
    for index, node_type in enumerate(node_types):
        if index > 0:
            cog.outl()

        cog.outl(templ.module.node_impl(node_type))
