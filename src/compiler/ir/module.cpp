#include "compiler/ir/module.hpp"

#include "common/string_utils.hpp"
#include "compiler/ir/function.hpp"

namespace tiro {

template<typename ID, typename Vec>
static bool check_id(const ID& id, const Vec& vec) {
    return id && id.value() < vec.size();
}

Module::Module(InternedString name, StringTable& strings)
    : strings_(strings)
    , name_(name) {}

Module::~Module() {}

ModuleMemberId Module::make(const ModuleMember& member) {
    return members_.push_back(member);
}

FunctionId Module::make(Function&& function) {
    return functions_.push_back(std::move(function));
}

NotNull<VecPtr<ModuleMember>> Module::operator[](ModuleMemberId id) {
    TIRO_DEBUG_ASSERT(check_id(id, members_), "Invalid member id.");
    return TIRO_NN(members_.ptr_to(id));
}

NotNull<VecPtr<Function>> Module::operator[](FunctionId id) {
    TIRO_DEBUG_ASSERT(check_id(id, functions_), "Invalid function id.");
    return TIRO_NN(functions_.ptr_to(id));
}

NotNull<VecPtr<const ModuleMember>> Module::operator[](ModuleMemberId id) const {
    TIRO_DEBUG_ASSERT(check_id(id, members_), "Invalid member id.");
    return TIRO_NN(members_.ptr_to(id));
}

NotNull<VecPtr<const Function>> Module::operator[](FunctionId id) const {
    TIRO_DEBUG_ASSERT(check_id(id, functions_), "Invalid function id.");
    return TIRO_NN(functions_.ptr_to(id));
}

void dump_module(const Module& module, FormatStream& stream) {
    stream.format(
        "Module\n"
        "  Name: {}\n"
        "  Members: {}\n",
        module.strings().dump(module.name()), module.member_count());

    // Dump all members
    {
        stream.format("\nMembers:\n");

        const size_t member_count = module.member_count();
        const size_t max_index_length = fmt::formatted_size(
            "{}", member_count == 0 ? 0 : member_count - 1);

        size_t index = 0;
        for (const auto& member : module.members()) {
            stream.format("  {:>{}}: {}\n", index, max_index_length,
                dump_helpers::DumpModuleMember{module, member});

            if (member.type() == ModuleMemberType::Function && index + 1 != member_count) {
                stream.format("\n");
            }
            ++index;
        }
    }
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import ModuleMemberData
    implement(ModuleMemberData.tag)
]]] */
std::string_view to_string(ModuleMemberType type) {
    switch (type) {
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
    from codegen.unions import implement
    from codegen.ir import ModuleMemberData
    implement(ModuleMemberData)
]]] */
ModuleMemberData ModuleMemberData::make_import(const InternedString& name) {
    return {Import{name}};
}

ModuleMemberData ModuleMemberData::make_variable(const InternedString& name) {
    return {Variable{name}};
}

ModuleMemberData ModuleMemberData::make_function(const FunctionId& id) {
    return {Function{id}};
}

ModuleMemberData::ModuleMemberData(Import import)
    : type_(ModuleMemberType::Import)
    , import_(std::move(import)) {}

ModuleMemberData::ModuleMemberData(Variable variable)
    : type_(ModuleMemberType::Variable)
    , variable_(std::move(variable)) {}

ModuleMemberData::ModuleMemberData(Function function)
    : type_(ModuleMemberType::Function)
    , function_(std::move(function)) {}

const ModuleMemberData::Import& ModuleMemberData::as_import() const {
    TIRO_DEBUG_ASSERT(
        type_ == ModuleMemberType::Import, "Bad member access on ModuleMemberData: not a Import.");
    return import_;
}

const ModuleMemberData::Variable& ModuleMemberData::as_variable() const {
    TIRO_DEBUG_ASSERT(type_ == ModuleMemberType::Variable,
        "Bad member access on ModuleMemberData: not a Variable.");
    return variable_;
}

const ModuleMemberData::Function& ModuleMemberData::as_function() const {
    TIRO_DEBUG_ASSERT(type_ == ModuleMemberType::Function,
        "Bad member access on ModuleMemberData: not a Function.");
    return function_;
}

void ModuleMemberData::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_import([[maybe_unused]] const Import& import) {
            stream.format("Import(name: {})", import.name);
        }

        void visit_variable([[maybe_unused]] const Variable& variable) {
            stream.format("Variable(name: {})", variable.name);
        }

        void visit_function([[maybe_unused]] const Function& function) {
            stream.format("Function(id: {})", function.id);
        }
    };
    visit(FormatVisitor{stream});
}

// [[[end]]]

ModuleMember::ModuleMember(const ModuleMemberData& data)
    : data_(data) {}

namespace dump_helpers {

void format(const DumpModuleMember& d, FormatStream& stream) {
    struct Visitor {
        const Module& module;
        FormatStream& stream;

        void visit_import(const ModuleMemberData::Import& i) {
            if (!i.name) {
                stream.format("Import(<unnamed>)");
                return;
            }

            std::string_view str = module.strings().value(i.name);
            stream.format("Import(\"{}\")", escape_string(str));
        }

        void visit_variable(const ModuleMemberData::Variable& v) {
            if (!v.name) {
                stream.format("Variable(<unnamed>)");
                return;
            }

            std::string_view str = module.strings().value(v.name);
            stream.format("Variable(\"{}\")", escape_string(str));
        }

        void visit_function(const ModuleMemberData::Function& f) {
            auto function_id = f.id;
            if (function_id) {
                const auto& function = module[function_id];
                IndentStream indent(stream, 4, false);
                dump_function(*function, indent);
            } else {
                stream.format("Invalid function");
            }
        }
    };

    if (d.member.exported()) {
        stream.format("[exported] ");
    }
    d.member.data().visit(Visitor{d.parent, stream});
}

} // namespace dump_helpers

} // namespace tiro
