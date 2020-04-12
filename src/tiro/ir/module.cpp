#include "tiro/ir/module.hpp"

#include "tiro/ir/function.hpp"

namespace tiro {

template<typename ID, typename Vec>
static bool check_id(const ID& id, const Vec& vec) {
    return id && id.value() < vec.size();
}

Module::Module(InternedString name, StringTable& strings)
    : strings_(strings)
    , name_(name) {}

Module::~Module() {}

ModuleMemberID Module::make(const ModuleMember& member) {
    return members_.push_back(member);
}

FunctionID Module::make(Function&& function) {
    return functions_.push_back(std::move(function));
}

NotNull<VecPtr<ModuleMember>> Module::operator[](ModuleMemberID id) {
    TIRO_DEBUG_ASSERT(check_id(id, members_), "Invalid member id.");
    return TIRO_NN(members_.ptr_to(id));
}

NotNull<VecPtr<Function>> Module::operator[](FunctionID id) {
    TIRO_DEBUG_ASSERT(check_id(id, functions_), "Invalid function id.");
    return TIRO_NN(functions_.ptr_to(id));
}

NotNull<VecPtr<const ModuleMember>>
    Module::operator[](ModuleMemberID id) const {
    TIRO_DEBUG_ASSERT(check_id(id, members_), "Invalid member id.");
    return TIRO_NN(members_.ptr_to(id));
}

NotNull<VecPtr<const Function>> Module::operator[](FunctionID id) const {
    TIRO_DEBUG_ASSERT(check_id(id, functions_), "Invalid function id.");
    return TIRO_NN(functions_.ptr_to(id));
}

void dump_module(const Module& module, FormatStream& stream) {
    stream.format(
        "Module\n"
        "  Name: {}\n"
        "  Members: {}\n"
        "  Functions: {}\n",
        module.strings().dump(module.name()), module.member_count(),
        module.function_count());

    // Dump all members
    {
        stream.format("\nMembers:\n");

        const size_t member_count = module.member_count();
        const size_t max_index_length = fmt::formatted_size(
            "{}", member_count == 0 ? 0 : member_count - 1);

        size_t index = 0;
        for (const auto& member : module.members()) {
            stream.format("  {index:>{width}}: {value}\n",
                fmt::arg("index", index), fmt::arg("width", max_index_length),
                fmt::arg("value", member));

            if (member.type() == ModuleMemberType::Function) {
                auto function_id = member.as_function().id;
                if (function_id) {
                    const auto& function = module[function_id];
                    IndentStream indent(stream, 4);
                    dump_function(*function, indent);
                }
            }

            stream.format("\n");
            ++index;
        }
    }
}

/* [[[cog
    import unions
    import ir
    unions.implement_type(ir.ModuleMemberType)
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
    import unions
    import ir
    unions.implement_type(ir.ModuleMember)
]]] */
ModuleMember ModuleMember::make_import(const InternedString& name) {
    return Import{name};
}

ModuleMember ModuleMember::make_variable(const InternedString& name) {
    return Variable{name};
}

ModuleMember ModuleMember::make_function(const FunctionID& id) {
    return Function{id};
}

ModuleMember::ModuleMember(const Import& import)
    : type_(ModuleMemberType::Import)
    , import_(import) {}

ModuleMember::ModuleMember(const Variable& variable)
    : type_(ModuleMemberType::Variable)
    , variable_(variable) {}

ModuleMember::ModuleMember(const Function& function)
    : type_(ModuleMemberType::Function)
    , function_(function) {}

const ModuleMember::Import& ModuleMember::as_import() const {
    TIRO_DEBUG_ASSERT(type_ == ModuleMemberType::Import,
        "Bad member access on ModuleMember: not a Import.");
    return import_;
}

const ModuleMember::Variable& ModuleMember::as_variable() const {
    TIRO_DEBUG_ASSERT(type_ == ModuleMemberType::Variable,
        "Bad member access on ModuleMember: not a Variable.");
    return variable_;
}

const ModuleMember::Function& ModuleMember::as_function() const {
    TIRO_DEBUG_ASSERT(type_ == ModuleMemberType::Function,
        "Bad member access on ModuleMember: not a Function.");
    return function_;
}

void ModuleMember::format(FormatStream& stream) const {
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

} // namespace tiro
