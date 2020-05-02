#!/usr/bin/env python3
from textwrap import dedent
from .unions import Tag, Union, Struct, Alias, Field


def visit_instr_param(instr, visitor):
    typename = type(instr).__name__
    visit = getattr(visitor, f"visit_{typename}")
    return visit(instr)


class InstrParam:
    """Represents a parameter accapted by an instruction."""

    def __init__(self, name, kind, doc):
        self.name = name
        self.kind = kind
        self.doc = doc

    @property
    def cpp_type(self):
        class Visitor:
            def visit_Local(self, local):
                return "BytecodeRegister"

            def visit_Param(self, param):
                return "BytecodeParam"

            def visit_Module(self, module):
                return "BytecodeMemberID"

            def visit_Offset(self, offset):
                return "BytecodeOffset"

            def visit_Integer(self, i):
                return i.int_type

            def visit_Float(self, f):
                return f.float_type

        return visit_instr_param(self, Visitor())

    @property
    def raw_type(self):
        class Visitor:
            def visit_Local(self, local):
                return "u32"

            def visit_Param(self, param):
                return "u32"

            def visit_Module(self, module):
                return "u32"

            def visit_Offset(self, offset):
                return "u32"

            def visit_Integer(self, i):
                return i.int_type

            def visit_Float(self, f):
                return f.float_type

        return visit_instr_param(self, Visitor())

    @property
    def description(self):
        class Visitor:
            def visit_Local(self, local):
                return "local"

            def visit_Param(self, param):
                return "param"

            def visit_Module(self, module):
                return "module"

            def visit_Offset(self, module):
                return "offset"

            def visit_Integer(self, i):
                return "constant"

            def visit_Float(self, f):
                return "constant"

        return visit_instr_param(self, Visitor())


class Local(InstrParam):
    """Refers to a local (by index)."""

    def __init__(self, name, doc=""):
        super().__init__(name, "local", doc)


class Param(InstrParam):
    """Refers to a function parameter (by index)."""

    def __init__(self, name, doc=""):
        super().__init__(name, "param", doc)


class Module(InstrParam):
    """Refers to a module variable (by index)."""

    def __init__(self, name, doc=""):
        super().__init__(name, "module", doc)


class Offset(InstrParam):
    """Refers to an absolute byte offset in the function."""

    def __init__(self, name, doc=""):
        super().__init__(name, "offset", doc)


class Integer(InstrParam):
    """Refers to an integer argument (signed or unsigned)."""

    def __init__(self, name, int_type, doc=""):
        super().__init__(name, "integer", doc)
        self.int_type = int_type


class Float(InstrParam):
    """Refers to a floating point argument."""

    def __init__(self, name, doc=""):
        super().__init__(name, "float", doc)
        self.float_type = "f64"


class Instr:
    """Represents a bytecode instruction."""

    def __init__(self, name, params=None, doc=""):
        self.name = name
        self.params = [] if params is None else params
        self.doc = doc


InstructionList = [
    Instr("LoadNull", [Local("target")], doc="Load null into the target."),
    Instr("LoadFalse", [Local("target")], doc="Load false into the target."),
    Instr("LoadTrue", [Local("target")], doc="Load true into the target."),
    Instr(
        "LoadInt",
        [Integer("constant", "i64"), Local("target")],
        doc="Load the given integer constant into the target.",
    ),
    Instr(
        "LoadFloat",
        [Float("constant"), Local("target")],
        doc="Load the given floating point constant into the target.",
    ),
    Instr(
        "LoadParam",
        [Param("source"), Local("target")],
        doc="Load the given parameter into the target.",
    ),
    Instr(
        "StoreParam",
        [Local("source"), Param("target")],
        doc="Store the given local into the the parameter.",
    ),
    Instr(
        "LoadModule",
        [Module("source"), Local("target")],
        doc="Load the module variable source into target.",
    ),
    Instr(
        "StoreModule",
        [Local("source"), Module("target")],
        doc="Store the source local into the target module variable.",
    ),
    Instr(
        "LoadMember",
        [Local("object"), Module("name"), Local("target")],
        doc="Load `object.name` into target.",
    ),
    Instr(
        "StoreMember",
        [Local("source"), Local("object"), Module("name")],
        doc="Store source into `object.name`.",
    ),
    Instr(
        "LoadTupleMember",
        [Local("tuple"), Integer("index", "u32"), Local("target")],
        doc="Load `tuple.index` into target.",
    ),
    Instr(
        "StoreTupleMember",
        [Local("source"), Local("tuple"), Integer("index", "u32")],
        doc="Store source into `tuple.index`.",
    ),
    Instr(
        "LoadIndex",
        [Local("array"), Local("index"), Local("target")],
        doc="Load `array[index]` into target.",
    ),
    Instr(
        "StoreIndex",
        [Local("source"), Local("array"), Local("index")],
        doc="Store source into `array[index]`.",
    ),
    Instr(
        "LoadClosure",
        [Local("target")],
        "Load the function's closure environment into the target.",
    ),
    Instr(
        "LoadEnv",
        [
            Local("env"),
            Integer("level", "u32"),
            Integer("index", "u32"),
            Local("target"),
        ],
        doc=dedent(
            """\
            Load a value from a closure environment. `level` is the number parent links to follow
            to reach the desired target environment (0 is `env` itself). `index` is the index of the value
            in the target environment."""
        ),
    ),
    Instr(
        "StoreEnv",
        [
            Local("source"),
            Local("env"),
            Integer("level", "u32"),
            Integer("index", "u32"),
        ],
        doc="Store a value into a closure environment. Analog to LoadEnv.",
    ),
    Instr(
        "Add",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs + rhs into target.",
    ),
    Instr(
        "Sub",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs - rhs into target.",
    ),
    Instr(
        "Mul",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs * rhs into target.",
    ),
    Instr(
        "Div",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs / rhs into target.",
    ),
    Instr(
        "Mod",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs % rhs into target.",
    ),
    Instr(
        "Pow",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store pow(lhs, rhs) into target.",
    ),
    Instr("UAdd", [Local("value"), Local("target")], doc="Store +value into target."),
    Instr("UNeg", [Local("value"), Local("target")], doc="Store -value into target."),
    Instr(
        "LSh",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs << rhs into target.",
    ),
    Instr(
        "RSh",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs >> rhs into target.",
    ),
    Instr(
        "BAnd",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs & rhs into target.",
    ),
    Instr(
        "BOr",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs | rhs into target.",
    ),
    Instr(
        "BXor",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs ^ rhs into target.",
    ),
    Instr("BNot", [Local("value"), Local("target")], doc="Store ~value into target."),
    Instr(
        "Gt",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs > rhs into target.",
    ),
    Instr(
        "Gte",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs >= rhs into target.",
    ),
    Instr(
        "Lt",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs < rhs into target.",
    ),
    Instr(
        "Lte",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs <= rhs into target.",
    ),
    Instr(
        "Eq",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs == rhs into target.",
    ),
    Instr(
        "NEq",
        [Local("lhs"), Local("rhs"), Local("target")],
        doc="Store lhs != rhs into target.",
    ),
    Instr("LNot", [Local("value"), Local("target")], doc="Store !value into target."),
    Instr(
        "Array",
        [Integer("count", "u32"), Local("target")],
        doc=dedent(
            """\
            Construct an array with the count topmost values
            from the stack and store it into target."""
        ),
    ),
    Instr(
        "Tuple",
        [Integer("count", "u32"), Local("target")],
        doc=dedent(
            """\
            Construct a tuple with the count topmost values
            from the stack and store it into target."""
        ),
    ),
    Instr(
        "Set",
        [Integer("count", "u32"), Local("target")],
        doc=dedent(
            """\
            Construct a set with the count topmost values
            from the stack and store it into target."""
        ),
    ),
    Instr(
        "Map",
        [Integer("count", "u32"), Local("target")],
        doc=dedent(
            """\
            Construct a map with the count topmost keys and values
            from the stack and store it into target.
            The count must be even.
            Arguments at even indices become keys, arguments at odd indices become
            values of the new map."""
        ),
    ),
    Instr(
        "Env",
        [Local("parent"), Integer("size", "u32"), Local("target")],
        doc=dedent(
            """\
            Construct an environment with the given parent and size and
            store it into target."""
        ),
    ),
    Instr(
        "Closure",
        [Local("template"), Local("env"), Local("target")],
        doc=dedent(
            """\
                Construct a closure with the given function template and environment and
                store it into target."""
        ),
    ),
    Instr(
        "Formatter",
        [Local("target")],
        doc="Construct a new string formatter and store it into target.",
    ),
    Instr(
        "AppendFormat",
        [Local("value"), Local("formatter")],
        doc="Format a value and append it to the formatter.",
    ),
    Instr(
        "FormatResult",
        [Local("formatter"), Local("target")],
        doc="Store the formatted string into target.",
    ),
    Instr("Copy", [Local("source"), Local("target")], doc="Copy source to target."),
    Instr("Swap", [Local("a"), Local("b")], doc="Swap the values of the two locals."),
    Instr("Push", [Local("value")], doc="Push value on the stack."),
    Instr("Pop", doc="Pop the top (written by most recent push) from the stack."),
    Instr(
        "PopTo",
        [Local("target")],
        doc="Pop the top (written by most recent push) from the stack and store it into target.",
    ),
    Instr("Jmp", [Offset("offset")], doc="Unconditional jump to the given offset."),
    Instr(
        "JmpTrue",
        [Local("condition"), Offset("offset")],
        doc=dedent(
            """\
            Jump to the given offset if the condition evaluates to true,
            otherwise continue with the next instruction."""
        ),
    ),
    Instr(
        "JmpFalse",
        [Local("condition"), Offset("offset")],
        doc=dedent(
            """\
            Jump to the given offset if the condition evaluates to false,
            otherwise continue with the next instruction."""
        ),
    ),
    Instr(
        "Call",
        [Local("function"), Integer("count", "u32")],
        doc=dedent(
            """\
            Call the given function the topmost count arguments on the stack.
            After the call, a single return value will be left on the stack."""
        ),
    ),
    Instr(
        "LoadMethod",
        [Local("object"), Module("name"), Local("this"), Local("method")],
        doc=dedent(
            """\
            Load the method called name from the given object.

            The appropriate this pointer (possibly null) will be stored into `this`.
            The method handle will be stored into `method`. The this pointer will be null
            for functions that do not accept a this parameter (e.g. bound methods, function
            attributes).

            This instruction is designed to be used in combination with CallMethod."""
        ),
    ),
    Instr(
        "CallMethod",
        [Local("method"), Integer("count", "u32")],
        doc=dedent(
            """\
            Call the given method on an object with `count` additional arguments on the stack.
            The caller must push the `this` value received by LoadMethod followed by `count` arguments (for 
            a total of `count + 1` push instructions).
            
            The arguments `this` and `method` must be the results
            of a previously executed LoadMethod instruction.

            After the call, a single return value will be left on the stack."""
        ),
    ),
    Instr("Return", [Local("value")], doc="Returns the value to the calling function."),
    Instr(
        "AssertFail",
        [Local("expr"), Local("message")],
        doc=dedent(
            """\
            Signals an assertion error and aborts the pogram.
            `expr` should contain the string representation of the failed assertion.
            `message` can hold a user defined error message string or null."""
        ),
    ),
]

InstructionMap = {instr.name: instr for instr in InstructionList}


def _map_instructions():
    def map_instruction(instr):
        members = []
        for param in instr.params:
            members.append(Field(param.name, param.cpp_type, doc=param.doc))

        doc = instr.doc
        if instr.params:
            doc += "\n\n"
            doc += "Arguments:\n"
            for (index, param) in enumerate(instr.params):
                if index > 0:
                    doc += "\n"
                doc += f"  - {param.name} ({param.description}, {param.raw_type})"

        return Struct(instr.name, members, doc)

    return [map_instruction(instr) for instr in InstructionList]


BytecodeOp = Tag(
    "BytecodeOp", "u8", start_value=1, doc="""Represents the type of an instruction.""",
)

Instruction = (
    Union(
        "BytecodeInstr",
        BytecodeOp,
        _map_instructions(),
        doc="Represents a bytecode instruction.",
    )
    .set_format_mode("define")
    .set_doc_mode("tag")
)

BytecodeMemberType = Tag(
    "BytecodeMemberType", "u8", doc="Represents the type of a module member."
)

BytecodeMember = (
    Union(
        "BytecodeMember",
        BytecodeMemberType,
        doc="Represents a member of a compiled module.",
        members=[
            Struct(
                "Integer",
                doc="Represents an integer constant.",
                members=[Field("value", "i64")],
            ),
            Struct(
                "Float",
                doc="Represents a floating point constant.",
                members=[Field("value", "f64"),],
            ),
            Struct(
                "String",
                doc="Represents a string constant.",
                members=[Field("value", "InternedString"),],
            ),
            Struct(
                "Symbol",
                doc="Represents a symbol constant.",
                members=[
                    Field(
                        "name", "BytecodeMemberID", doc="References a string constant.",
                    ),
                ],
            ),
            Struct(
                "Import",
                doc="Represents an import.",
                members=[
                    Field(
                        "module_name",
                        "BytecodeMemberID",
                        doc="References a string constant.",
                    ),
                ],
            ),
            Struct(
                "Variable",
                doc="Represents a variable.",
                members=[
                    Field(
                        "name", "BytecodeMemberID", doc="References a string constant.",
                    ),
                    Field(
                        "initial_value",
                        "BytecodeMemberID",
                        doc="References a constant. Can be invalid (meaning: initially null).",
                    ),
                ],
            ),
            Struct(
                "Function",
                doc="Represents a function.",
                members=[
                    Field(
                        "id",
                        "BytecodeFunctionID",
                        doc="References the compiled function.",
                    ),
                ],
            ),
        ],
    )
    .set_format_mode("define")
    .set_hash_mode("define")
    .set_equality_mode("define")
)
