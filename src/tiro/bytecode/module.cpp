#include "tiro/bytecode/module.hpp"

namespace tiro::compiler::bytecode {

std::string_view to_string(FunctionType type) {
    switch (type) {
    case FunctionType::Normal:
        return "Normal";
    case FunctionType::Closure:
        return "Closure";
    }

    TIRO_UNREACHABLE("Invalid function type.");
}

Function::Function() {}

Function::~Function() {}

/* [[[cog
    import unions
    import bytecode
    unions.implement_type(bytecode.ModuleMemberType)
]]] */
std::string_view to_string(ModuleMemberType type) {
    switch (type) {
    case ModuleMemberType::Integer:
        return "Integer";
    case ModuleMemberType::Float:
        return "Float";
    case ModuleMemberType::String:
        return "String";
    case ModuleMemberType::Symbol:
        return "Symbol";
    case ModuleMemberType::Import:
        return "Import";
    case ModuleMemberType::Variable:
        return "Variable";
    case ModuleMemberType::Function:
        return "Function";
    }
    TIRO_UNREACHABLE("Invalid ModuleMemberType.");
}
// [[[end]]]

/* [[[cog
    import unions
    import bytecode
    unions.implement_type(bytecode.ModuleMember)
]]] */
ModuleMember ModuleMember::make_integer(const i64& value) {
    return Integer{value};
}

ModuleMember ModuleMember::make_float(const f64& value) {
    return Float{value};
}

ModuleMember ModuleMember::make_string(const InternedString& value) {
    return String{value};
}

ModuleMember ModuleMember::make_symbol(const ModuleMemberID& symbol_name) {
    return Symbol{symbol_name};
}

ModuleMember ModuleMember::make_import(const ModuleMemberID& module_name) {
    return Import{module_name};
}

ModuleMember ModuleMember::make_variable(
    const ModuleMemberID& name, const ModuleMemberID& initial_value) {
    return Variable{name, initial_value};
}

ModuleMember ModuleMember::make_function(const FunctionID& value) {
    return Function{value};
}

ModuleMember::ModuleMember(const Integer& integer)
    : type_(ModuleMemberType::Integer)
    , integer_(integer) {}

ModuleMember::ModuleMember(const Float& f)
    : type_(ModuleMemberType::Float)
    , float_(f) {}

ModuleMember::ModuleMember(const String& string)
    : type_(ModuleMemberType::String)
    , string_(string) {}

ModuleMember::ModuleMember(const Symbol& symbol)
    : type_(ModuleMemberType::Symbol)
    , symbol_(symbol) {}

ModuleMember::ModuleMember(const Import& import)
    : type_(ModuleMemberType::Import)
    , import_(import) {}

ModuleMember::ModuleMember(const Variable& variable)
    : type_(ModuleMemberType::Variable)
    , variable_(variable) {}

ModuleMember::ModuleMember(const Function& function)
    : type_(ModuleMemberType::Function)
    , function_(function) {}

const ModuleMember::Integer& ModuleMember::as_integer() const {
    TIRO_ASSERT(type_ == ModuleMemberType::Integer,
        "Bad member access on ModuleMember: not a Integer.");
    return integer_;
}

const ModuleMember::Float& ModuleMember::as_float() const {
    TIRO_ASSERT(type_ == ModuleMemberType::Float,
        "Bad member access on ModuleMember: not a Float.");
    return float_;
}

const ModuleMember::String& ModuleMember::as_string() const {
    TIRO_ASSERT(type_ == ModuleMemberType::String,
        "Bad member access on ModuleMember: not a String.");
    return string_;
}

const ModuleMember::Symbol& ModuleMember::as_symbol() const {
    TIRO_ASSERT(type_ == ModuleMemberType::Symbol,
        "Bad member access on ModuleMember: not a Symbol.");
    return symbol_;
}

const ModuleMember::Import& ModuleMember::as_import() const {
    TIRO_ASSERT(type_ == ModuleMemberType::Import,
        "Bad member access on ModuleMember: not a Import.");
    return import_;
}

const ModuleMember::Variable& ModuleMember::as_variable() const {
    TIRO_ASSERT(type_ == ModuleMemberType::Variable,
        "Bad member access on ModuleMember: not a Variable.");
    return variable_;
}

const ModuleMember::Function& ModuleMember::as_function() const {
    TIRO_ASSERT(type_ == ModuleMemberType::Function,
        "Bad member access on ModuleMember: not a Function.");
    return function_;
}

void ModuleMember::format(FormatStream& stream) const {
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
            stream.format("Symbol(symbol_name: {})", symbol.symbol_name);
        }

        void visit_import([[maybe_unused]] const Import& import) {
            stream.format("Import(module_name: {})", import.module_name);
        }

        void visit_variable([[maybe_unused]] const Variable& variable) {
            stream.format("Variable(name: {}, initial_value: {})",
                variable.name, variable.initial_value);
        }

        void visit_function([[maybe_unused]] const Function& function) {
            stream.format("Function(value: {})", function.value);
        }
    };
    visit(FormatVisitor{stream});
}

bool operator==(const ModuleMember& lhs, const ModuleMember& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const ModuleMember& rhs;

        bool
        visit_integer([[maybe_unused]] const ModuleMember::Integer& integer) {
            [[maybe_unused]] const auto& other = rhs.as_integer();
            return integer.value == other.value;
        }

        bool visit_float([[maybe_unused]] const ModuleMember::Float& f) {
            [[maybe_unused]] const auto& other = rhs.as_float();
            return f.value == other.value;
        }

        bool visit_string([[maybe_unused]] const ModuleMember::String& string) {
            [[maybe_unused]] const auto& other = rhs.as_string();
            return string.value == other.value;
        }

        bool visit_symbol([[maybe_unused]] const ModuleMember::Symbol& symbol) {
            [[maybe_unused]] const auto& other = rhs.as_symbol();
            return symbol.symbol_name == other.symbol_name;
        }

        bool visit_import([[maybe_unused]] const ModuleMember::Import& import) {
            [[maybe_unused]] const auto& other = rhs.as_import();
            return import.module_name == other.module_name;
        }

        bool visit_variable(
            [[maybe_unused]] const ModuleMember::Variable& variable) {
            [[maybe_unused]] const auto& other = rhs.as_variable();
            return variable.name == other.name
                   && variable.initial_value == other.initial_value;
        }

        bool visit_function(
            [[maybe_unused]] const ModuleMember::Function& function) {
            [[maybe_unused]] const auto& other = rhs.as_function();
            return function.value == other.value;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const ModuleMember& lhs, const ModuleMember& rhs) {
    return !(lhs == rhs);
}
// [[[end]]]

Module::Module(StringTable& strings)
    : strings_(strings) {}

Module::~Module() {}

ModuleMemberID Module::make(const ModuleMember& member) {
    return members_.push_back(member);
}

FunctionID Module::make(Function&& fn) {
    return functions_.push_back(std::move(fn));
}

NotNull<IndexMapPtr<ModuleMember>> Module::operator[](ModuleMemberID id) {
    return TIRO_NN(members_.ptr_to(id));
}

NotNull<IndexMapPtr<const ModuleMember>> Module::
operator[](ModuleMemberID id) const {
    return TIRO_NN(members_.ptr_to(id));
}

NotNull<IndexMapPtr<Function>> Module::operator[](FunctionID id) {
    return TIRO_NN(functions_.ptr_to(id));
}

NotNull<IndexMapPtr<const Function>> Module::operator[](FunctionID id) const {
    return TIRO_NN(functions_.ptr_to(id));
}

} // namespace tiro::compiler::bytecode
