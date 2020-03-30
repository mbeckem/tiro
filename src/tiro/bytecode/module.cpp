#include "tiro/bytecode/module.hpp"

#include "tiro/bytecode/disassembler.hpp"
#include "tiro/compiler/utils.hpp"

namespace tiro {

std::string_view to_string(CompiledFunctionType type) {
    switch (type) {
    case CompiledFunctionType::Normal:
        return "Normal";
    case CompiledFunctionType::Closure:
        return "Closure";
    }

    TIRO_UNREACHABLE("Invalid function type.");
}

CompiledFunction::CompiledFunction() {}

CompiledFunction::~CompiledFunction() {}

void dump_function(const CompiledFunction& func, const StringTable& strings,
    FormatStream& stream) {
    stream.format(
        "Function\n"
        "  Name: {}\n"
        "  Type: {}\n"
        "  Params: {}\n"
        "  Locals: {}\n"
        "\n"
        "{}\n",
        strings.dump(func.name()), func.type(), func.params(), func.locals(),
        disassemble(func.code()));
}

/* [[[cog
    import unions
    import bytecode
    unions.implement_type(bytecode.CompiledModuleMemberType)
]]] */
std::string_view to_string(CompiledModuleMemberType type) {
    switch (type) {
    case CompiledModuleMemberType::Integer:
        return "Integer";
    case CompiledModuleMemberType::Float:
        return "Float";
    case CompiledModuleMemberType::String:
        return "String";
    case CompiledModuleMemberType::Symbol:
        return "Symbol";
    case CompiledModuleMemberType::Import:
        return "Import";
    case CompiledModuleMemberType::Variable:
        return "Variable";
    case CompiledModuleMemberType::Function:
        return "Function";
    }
    TIRO_UNREACHABLE("Invalid CompiledModuleMemberType.");
}
// [[[end]]]

/* [[[cog
    import unions
    import bytecode
    unions.implement_type(bytecode.CompiledModuleMember)
]]] */
CompiledModuleMember CompiledModuleMember::make_integer(const i64& value) {
    return Integer{value};
}

CompiledModuleMember CompiledModuleMember::make_float(const f64& value) {
    return Float{value};
}

CompiledModuleMember
CompiledModuleMember::make_string(const InternedString& value) {
    return String{value};
}

CompiledModuleMember
CompiledModuleMember::make_symbol(const CompiledModuleMemberID& name) {
    return Symbol{name};
}

CompiledModuleMember
CompiledModuleMember::make_import(const CompiledModuleMemberID& module_name) {
    return Import{module_name};
}

CompiledModuleMember
CompiledModuleMember::make_variable(const CompiledModuleMemberID& name,
    const CompiledModuleMemberID& initial_value) {
    return Variable{name, initial_value};
}

CompiledModuleMember
CompiledModuleMember::make_function(const CompiledFunctionID& id) {
    return Function{id};
}

CompiledModuleMember::CompiledModuleMember(const Integer& integer)
    : type_(CompiledModuleMemberType::Integer)
    , integer_(integer) {}

CompiledModuleMember::CompiledModuleMember(const Float& f)
    : type_(CompiledModuleMemberType::Float)
    , float_(f) {}

CompiledModuleMember::CompiledModuleMember(const String& string)
    : type_(CompiledModuleMemberType::String)
    , string_(string) {}

CompiledModuleMember::CompiledModuleMember(const Symbol& symbol)
    : type_(CompiledModuleMemberType::Symbol)
    , symbol_(symbol) {}

CompiledModuleMember::CompiledModuleMember(const Import& import)
    : type_(CompiledModuleMemberType::Import)
    , import_(import) {}

CompiledModuleMember::CompiledModuleMember(const Variable& variable)
    : type_(CompiledModuleMemberType::Variable)
    , variable_(variable) {}

CompiledModuleMember::CompiledModuleMember(const Function& function)
    : type_(CompiledModuleMemberType::Function)
    , function_(function) {}

const CompiledModuleMember::Integer& CompiledModuleMember::as_integer() const {
    TIRO_ASSERT(type_ == CompiledModuleMemberType::Integer,
        "Bad member access on CompiledModuleMember: not a Integer.");
    return integer_;
}

const CompiledModuleMember::Float& CompiledModuleMember::as_float() const {
    TIRO_ASSERT(type_ == CompiledModuleMemberType::Float,
        "Bad member access on CompiledModuleMember: not a Float.");
    return float_;
}

const CompiledModuleMember::String& CompiledModuleMember::as_string() const {
    TIRO_ASSERT(type_ == CompiledModuleMemberType::String,
        "Bad member access on CompiledModuleMember: not a String.");
    return string_;
}

const CompiledModuleMember::Symbol& CompiledModuleMember::as_symbol() const {
    TIRO_ASSERT(type_ == CompiledModuleMemberType::Symbol,
        "Bad member access on CompiledModuleMember: not a Symbol.");
    return symbol_;
}

const CompiledModuleMember::Import& CompiledModuleMember::as_import() const {
    TIRO_ASSERT(type_ == CompiledModuleMemberType::Import,
        "Bad member access on CompiledModuleMember: not a Import.");
    return import_;
}

const CompiledModuleMember::Variable&
CompiledModuleMember::as_variable() const {
    TIRO_ASSERT(type_ == CompiledModuleMemberType::Variable,
        "Bad member access on CompiledModuleMember: not a Variable.");
    return variable_;
}

const CompiledModuleMember::Function&
CompiledModuleMember::as_function() const {
    TIRO_ASSERT(type_ == CompiledModuleMemberType::Function,
        "Bad member access on CompiledModuleMember: not a Function.");
    return function_;
}

void CompiledModuleMember::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_integer([[maybe_unused]] const Integer& integer) {
            stream.format("Integer(value: {})", integer.value);
        }

        void visit_float([[maybe_unused]] const Float& f) {
            stream.format("Float(value: {})", f.value);
        }

        void visit_string([[maybe_unused]] const String& string) {
            stream.format("String(value: {})", string.value);
        }

        void visit_symbol([[maybe_unused]] const Symbol& symbol) {
            stream.format("Symbol(name: {})", symbol.name);
        }

        void visit_import([[maybe_unused]] const Import& import) {
            stream.format("Import(module_name: {})", import.module_name);
        }

        void visit_variable([[maybe_unused]] const Variable& variable) {
            stream.format("Variable(name: {}, initial_value: {})",
                variable.name, variable.initial_value);
        }

        void visit_function([[maybe_unused]] const Function& function) {
            stream.format("Function(id: {})", function.id);
        }
    };
    visit(FormatVisitor{stream});
}

void CompiledModuleMember::build_hash(Hasher& h) const {
    h.append(type());

    struct HashVisitor {
        Hasher& h;

        void visit_integer([[maybe_unused]] const Integer& integer) {
            h.append(integer.value);
        }

        void visit_float([[maybe_unused]] const Float& f) { h.append(f.value); }

        void visit_string([[maybe_unused]] const String& string) {
            h.append(string.value);
        }

        void visit_symbol([[maybe_unused]] const Symbol& symbol) {
            h.append(symbol.name);
        }

        void visit_import([[maybe_unused]] const Import& import) {
            h.append(import.module_name);
        }

        void visit_variable([[maybe_unused]] const Variable& variable) {
            h.append(variable.name).append(variable.initial_value);
        }

        void visit_function([[maybe_unused]] const Function& function) {
            h.append(function.id);
        }
    };
    return visit(HashVisitor{h});
}

bool operator==(
    const CompiledModuleMember& lhs, const CompiledModuleMember& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const CompiledModuleMember& rhs;

        bool visit_integer(
            [[maybe_unused]] const CompiledModuleMember::Integer& integer) {
            [[maybe_unused]] const auto& other = rhs.as_integer();
            return integer.value == other.value;
        }

        bool
        visit_float([[maybe_unused]] const CompiledModuleMember::Float& f) {
            [[maybe_unused]] const auto& other = rhs.as_float();
            return f.value == other.value;
        }

        bool visit_string(
            [[maybe_unused]] const CompiledModuleMember::String& string) {
            [[maybe_unused]] const auto& other = rhs.as_string();
            return string.value == other.value;
        }

        bool visit_symbol(
            [[maybe_unused]] const CompiledModuleMember::Symbol& symbol) {
            [[maybe_unused]] const auto& other = rhs.as_symbol();
            return symbol.name == other.name;
        }

        bool visit_import(
            [[maybe_unused]] const CompiledModuleMember::Import& import) {
            [[maybe_unused]] const auto& other = rhs.as_import();
            return import.module_name == other.module_name;
        }

        bool visit_variable(
            [[maybe_unused]] const CompiledModuleMember::Variable& variable) {
            [[maybe_unused]] const auto& other = rhs.as_variable();
            return variable.name == other.name
                   && variable.initial_value == other.initial_value;
        }

        bool visit_function(
            [[maybe_unused]] const CompiledModuleMember::Function& function) {
            [[maybe_unused]] const auto& other = rhs.as_function();
            return function.id == other.id;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(
    const CompiledModuleMember& lhs, const CompiledModuleMember& rhs) {
    return !(lhs == rhs);
}
// [[[end]]]

CompiledModule::CompiledModule(StringTable& strings)
    : strings_(strings) {}

CompiledModule::~CompiledModule() {}

CompiledModuleMemberID
CompiledModule::make(const CompiledModuleMember& member) {
    return members_.push_back(member);
}

CompiledFunctionID CompiledModule::make(CompiledFunction&& fn) {
    return functions_.push_back(std::move(fn));
}

NotNull<IndexMapPtr<CompiledModuleMember>> CompiledModule::
operator[](CompiledModuleMemberID id) {
    return TIRO_NN(members_.ptr_to(id));
}

NotNull<IndexMapPtr<const CompiledModuleMember>> CompiledModule::
operator[](CompiledModuleMemberID id) const {
    return TIRO_NN(members_.ptr_to(id));
}

NotNull<IndexMapPtr<CompiledFunction>> CompiledModule::
operator[](CompiledFunctionID id) {
    return TIRO_NN(functions_.ptr_to(id));
}

NotNull<IndexMapPtr<const CompiledFunction>> CompiledModule::
operator[](CompiledFunctionID id) const {
    return TIRO_NN(functions_.ptr_to(id));
}

void dump_module(const CompiledModule& module, FormatStream& stream) {
    struct MemberVisitor {
        const CompiledModule& module;
        FormatStream& stream;

        void visit_integer(const CompiledModuleMember::Integer& i) {
            stream.format("Integer({})\n", i.value);
        }

        void visit_float(const CompiledModuleMember::Float& f) {
            stream.format("Float({})\n", f.value);
        }

        void visit_string(const CompiledModuleMember::String& s) {
            std::string_view str = module.strings().value(s.value);
            stream.format("String(\"{}\")\n", escape_string(str));
        }

        void visit_symbol(const CompiledModuleMember::Symbol& s) {
            stream.format("Symbol(name: {})\n", s.name.value());
        }

        void visit_import(const CompiledModuleMember::Import& i) {
            stream.format("Import(module_name: {})\n", i.module_name.value());
        }

        void visit_variable(const CompiledModuleMember::Variable& v) {
            stream.format("Variable(name: {})\n", v.name.value());
        }

        void visit_function(const CompiledModuleMember::Function& f) {
            const auto& function = module[f.id];
            IndentStream indent(stream, 4, false);
            dump_function(*function, module.strings(), indent);
        }
    };

    stream.format(
        "Module\n"
        "  Name: {}\n"
        "  Members: {}\n"
        "  Functions: {}\n",
        module.strings().dump(module.name()), module.member_count(),
        module.function_count());

    stream.format("\nMembers:\n");
    const size_t member_count = module.member_count();
    const size_t max_index_length = fmt::formatted_size(
        "{}", member_count == 0 ? 0 : member_count - 1);

    size_t index = 0;
    for (const auto& member_id : module.member_ids()) {
        stream.format("  {index:>{width}}: ", fmt::arg("index", index),
            fmt::arg("width", max_index_length));

        module[member_id]->visit(MemberVisitor{module, stream});
        ++index;
    }
}

} // namespace tiro
