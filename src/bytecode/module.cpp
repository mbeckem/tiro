#include "bytecode/module.hpp"

#include "common/assert.hpp"

namespace tiro {

BytecodeRecordTemplate::BytecodeRecordTemplate() {}

BytecodeRecordTemplate::~BytecodeRecordTemplate() {}

BytecodeRecordTemplate::BytecodeRecordTemplate(BytecodeRecordTemplate&&) noexcept = default;

BytecodeRecordTemplate&
BytecodeRecordTemplate::operator=(BytecodeRecordTemplate&&) noexcept = default;

/* [[[cog
    from codegen.unions import implement
    from codegen.bytecode import BytecodeMemberType
    implement(BytecodeMemberType)
]]] */
std::string_view to_string(BytecodeMemberType type) {
    switch (type) {
    case BytecodeMemberType::Integer:
        return "Integer";
    case BytecodeMemberType::Float:
        return "Float";
    case BytecodeMemberType::String:
        return "String";
    case BytecodeMemberType::Symbol:
        return "Symbol";
    case BytecodeMemberType::Import:
        return "Import";
    case BytecodeMemberType::Variable:
        return "Variable";
    case BytecodeMemberType::Function:
        return "Function";
    case BytecodeMemberType::RecordTemplate:
        return "RecordTemplate";
    }
    TIRO_UNREACHABLE("Invalid BytecodeMemberType.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.bytecode import BytecodeMember
    implement(BytecodeMember)
]]] */
BytecodeMember BytecodeMember::make_integer(const i64& value) {
    return {Integer{value}};
}

BytecodeMember BytecodeMember::make_float(const f64& value) {
    return {Float{value}};
}

BytecodeMember BytecodeMember::make_string(const InternedString& value) {
    return {String{value}};
}

BytecodeMember BytecodeMember::make_symbol(const BytecodeMemberId& name) {
    return {Symbol{name}};
}

BytecodeMember BytecodeMember::make_import(const BytecodeMemberId& module_name) {
    return {Import{module_name}};
}

BytecodeMember
BytecodeMember::make_variable(const BytecodeMemberId& name, const BytecodeMemberId& initial_value) {
    return {Variable{name, initial_value}};
}

BytecodeMember BytecodeMember::make_function(const BytecodeFunctionId& id) {
    return {Function{id}};
}

BytecodeMember BytecodeMember::make_record_template(const BytecodeRecordTemplateId& id) {
    return {RecordTemplate{id}};
}

BytecodeMember::BytecodeMember(Integer integer)
    : type_(BytecodeMemberType::Integer)
    , integer_(std::move(integer)) {}

BytecodeMember::BytecodeMember(Float f)
    : type_(BytecodeMemberType::Float)
    , float_(std::move(f)) {}

BytecodeMember::BytecodeMember(String string)
    : type_(BytecodeMemberType::String)
    , string_(std::move(string)) {}

BytecodeMember::BytecodeMember(Symbol symbol)
    : type_(BytecodeMemberType::Symbol)
    , symbol_(std::move(symbol)) {}

BytecodeMember::BytecodeMember(Import import)
    : type_(BytecodeMemberType::Import)
    , import_(std::move(import)) {}

BytecodeMember::BytecodeMember(Variable variable)
    : type_(BytecodeMemberType::Variable)
    , variable_(std::move(variable)) {}

BytecodeMember::BytecodeMember(Function function)
    : type_(BytecodeMemberType::Function)
    , function_(std::move(function)) {}

BytecodeMember::BytecodeMember(RecordTemplate record_template)
    : type_(BytecodeMemberType::RecordTemplate)
    , record_template_(std::move(record_template)) {}

const BytecodeMember::Integer& BytecodeMember::as_integer() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeMemberType::Integer,
        "Bad member access on BytecodeMember: not a Integer.");
    return integer_;
}

const BytecodeMember::Float& BytecodeMember::as_float() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeMemberType::Float, "Bad member access on BytecodeMember: not a Float.");
    return float_;
}

const BytecodeMember::String& BytecodeMember::as_string() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeMemberType::String, "Bad member access on BytecodeMember: not a String.");
    return string_;
}

const BytecodeMember::Symbol& BytecodeMember::as_symbol() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeMemberType::Symbol, "Bad member access on BytecodeMember: not a Symbol.");
    return symbol_;
}

const BytecodeMember::Import& BytecodeMember::as_import() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeMemberType::Import, "Bad member access on BytecodeMember: not a Import.");
    return import_;
}

const BytecodeMember::Variable& BytecodeMember::as_variable() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeMemberType::Variable,
        "Bad member access on BytecodeMember: not a Variable.");
    return variable_;
}

const BytecodeMember::Function& BytecodeMember::as_function() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeMemberType::Function,
        "Bad member access on BytecodeMember: not a Function.");
    return function_;
}

const BytecodeMember::RecordTemplate& BytecodeMember::as_record_template() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeMemberType::RecordTemplate,
        "Bad member access on BytecodeMember: not a RecordTemplate.");
    return record_template_;
}

void BytecodeMember::format(FormatStream& stream) const {
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
            stream.format(
                "Variable(name: {}, initial_value: {})", variable.name, variable.initial_value);
        }

        void visit_function([[maybe_unused]] const Function& function) {
            stream.format("Function(id: {})", function.id);
        }

        void visit_record_template([[maybe_unused]] const RecordTemplate& record_template) {
            stream.format("RecordTemplate(id: {})", record_template.id);
        }
    };
    visit(FormatVisitor{stream});
}

void BytecodeMember::hash(Hasher& h) const {
    h.append(type());

    struct HashVisitor {
        Hasher& h;

        void visit_integer([[maybe_unused]] const Integer& integer) { h.append(integer.value); }

        void visit_float([[maybe_unused]] const Float& f) { h.append(f.value); }

        void visit_string([[maybe_unused]] const String& string) { h.append(string.value); }

        void visit_symbol([[maybe_unused]] const Symbol& symbol) { h.append(symbol.name); }

        void visit_import([[maybe_unused]] const Import& import) { h.append(import.module_name); }

        void visit_variable([[maybe_unused]] const Variable& variable) {
            h.append(variable.name).append(variable.initial_value);
        }

        void visit_function([[maybe_unused]] const Function& function) { h.append(function.id); }

        void visit_record_template([[maybe_unused]] const RecordTemplate& record_template) {
            h.append(record_template.id);
        }
    };
    return visit(HashVisitor{h});
}

bool operator==(const BytecodeMember& lhs, const BytecodeMember& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const BytecodeMember& rhs;

        bool visit_integer([[maybe_unused]] const BytecodeMember::Integer& integer) {
            [[maybe_unused]] const auto& other = rhs.as_integer();
            return integer.value == other.value;
        }

        bool visit_float([[maybe_unused]] const BytecodeMember::Float& f) {
            [[maybe_unused]] const auto& other = rhs.as_float();
            return f.value == other.value;
        }

        bool visit_string([[maybe_unused]] const BytecodeMember::String& string) {
            [[maybe_unused]] const auto& other = rhs.as_string();
            return string.value == other.value;
        }

        bool visit_symbol([[maybe_unused]] const BytecodeMember::Symbol& symbol) {
            [[maybe_unused]] const auto& other = rhs.as_symbol();
            return symbol.name == other.name;
        }

        bool visit_import([[maybe_unused]] const BytecodeMember::Import& import) {
            [[maybe_unused]] const auto& other = rhs.as_import();
            return import.module_name == other.module_name;
        }

        bool visit_variable([[maybe_unused]] const BytecodeMember::Variable& variable) {
            [[maybe_unused]] const auto& other = rhs.as_variable();
            return variable.name == other.name && variable.initial_value == other.initial_value;
        }

        bool visit_function([[maybe_unused]] const BytecodeMember::Function& function) {
            [[maybe_unused]] const auto& other = rhs.as_function();
            return function.id == other.id;
        }

        bool visit_record_template(
            [[maybe_unused]] const BytecodeMember::RecordTemplate& record_template) {
            [[maybe_unused]] const auto& other = rhs.as_record_template();
            return record_template.id == other.id;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const BytecodeMember& lhs, const BytecodeMember& rhs) {
    return !(lhs == rhs);
}
// [[[end]]]

BytecodeModule::BytecodeModule() {}

BytecodeModule::~BytecodeModule() {}

void BytecodeModule::add_export(BytecodeMemberId symbol_id, BytecodeMemberId value_id) {
    TIRO_DEBUG_ASSERT(symbol_id, "The symbol id must be valid.");
    TIRO_DEBUG_ASSERT(value_id, "The value id must be valid.");
    exports_.emplace_back(symbol_id, value_id);
}

BytecodeMemberId BytecodeModule::make(const BytecodeMember& member) {
    return members_.push_back(member);
}

BytecodeFunctionId BytecodeModule::make(BytecodeFunction&& fn) {
    return functions_.push_back(std::move(fn));
}

BytecodeRecordTemplateId BytecodeModule::make(BytecodeRecordTemplate&& tmpl) {
    return records_.push_back(std::move(tmpl));
}

NotNull<IndexMapPtr<BytecodeMember>> BytecodeModule::operator[](BytecodeMemberId id) {
    return TIRO_NN(members_.ptr_to(id));
}

NotNull<IndexMapPtr<const BytecodeMember>> BytecodeModule::operator[](BytecodeMemberId id) const {
    return TIRO_NN(members_.ptr_to(id));
}

NotNull<IndexMapPtr<BytecodeFunction>> BytecodeModule::operator[](BytecodeFunctionId id) {
    return TIRO_NN(functions_.ptr_to(id));
}

NotNull<IndexMapPtr<const BytecodeFunction>>
BytecodeModule::operator[](BytecodeFunctionId id) const {
    return TIRO_NN(functions_.ptr_to(id));
}

NotNull<IndexMapPtr<BytecodeRecordTemplate>>
BytecodeModule::operator[](BytecodeRecordTemplateId id) {
    return TIRO_NN(records_.ptr_to(id));
}

NotNull<IndexMapPtr<const BytecodeRecordTemplate>>
BytecodeModule::operator[](BytecodeRecordTemplateId id) const {
    return TIRO_NN(records_.ptr_to(id));
}

} // namespace tiro
