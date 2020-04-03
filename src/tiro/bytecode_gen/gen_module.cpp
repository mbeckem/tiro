#include "tiro/bytecode_gen/gen_module.hpp"

#include "tiro/bytecode/instruction.hpp"
#include "tiro/bytecode/module.hpp"
#include "tiro/bytecode_gen/gen_func.hpp"
#include "tiro/compiler/binary.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/ir/types.hpp"

#include <algorithm>
#include <unordered_map>

namespace tiro {

namespace {

class ModuleCompiler final {
public:
    explicit ModuleCompiler(Module& module, CompiledModule& result)
        : module_(module)
        , result_(result) {}

    StringTable& strings() const { return module_.strings(); }

    void run();

private:
    using DefinitionMap =
        std::unordered_map<ModuleMemberID, CompiledModuleMemberID, UseHasher>;

    using RenameMap = std::unordered_map<CompiledModuleMemberID,
        CompiledModuleMemberID, UseHasher>;

    // Improvement: could split members and parallelize, or split
    // them by source file and compile & link incrementally.
    // Would make merging of objects a requirement.
    void compile_object();

    void link_members();

    std::vector<CompiledModuleMemberID> reorder_members() const;

    void fix_references(std::vector<CompiledModuleMember>& members);

    void fix_func_references(CompiledFunctionID func_id);

    CompiledModuleMemberID renamed(CompiledModuleMemberID old) const {
        auto pos = renamed_.find(old);
        TIRO_CHECK(pos != renamed_.end(),
            "Module member was not assigned a new position.");
        return pos->second;
    }

    CompiledModuleMemberID resolved(ModuleMemberID ir_id) const {
        auto pos = defs_.find(ir_id);
        TIRO_CHECK(pos != defs_.end(), "Module member was never defined.");
        return pos->second;
    }

private:
    Module& module_;
    CompiledModule& result_;

    LinkObject object_;

    // Definitions of ir module members in the compiled representation.
    // Refers to the final module index (not the index in the object).
    DefinitionMap defs_;

    // Old index (in object) to new object (in output).
    RenameMap renamed_;

    std::vector<CompiledModuleMember> final_members_;
};

} // namespace

static int module_type_order(CompiledModuleMemberType type) {
    switch (type) {
    case CompiledModuleMemberType::Integer:
        return 0;
    case CompiledModuleMemberType::Float:
        return 1;
    case CompiledModuleMemberType::String:
        return 2;
    case CompiledModuleMemberType::Symbol:
        return 3;
    case CompiledModuleMemberType::Import:
        return 4;
    case CompiledModuleMemberType::Variable:
        return 5;
    case CompiledModuleMemberType::Function:
        return 6;
    }

    TIRO_UNREACHABLE("Invalid compiled module member type.");
}

static int function_type_order(CompiledFunctionType type) {
    switch (type) {
    case CompiledFunctionType::Normal:
        return 0;
    case CompiledFunctionType::Closure:
        return 1;
    }

    TIRO_UNREACHABLE("Invalid compiled function type.");
}

static bool
module_order_less(CompiledModuleMemberID lhs, CompiledModuleMemberID rhs,
    const LinkObject& object, const StringTable& strings) {
    const auto& ld = object[lhs]->as_definition().value;
    const auto& rd = object[rhs]->as_definition().value;

    const auto lt = ld.type();
    const auto rt = rd.type();
    if (lt != rt) {
        return module_type_order(lt) < module_type_order(rt);
    }

    struct Comparator {
        const LinkObject& object;
        const StringTable& strings;
        const CompiledModuleMember& rhs;

        bool visit_integer(const CompiledModuleMember::Integer& lhs) const {
            return lhs.value < rhs.as_integer().value;
        }

        bool visit_float(const CompiledModuleMember::Float& lhs) const {
            return lhs.value < rhs.as_float().value;
        }

        bool visit_string(const CompiledModuleMember::String& lhs) const {
            return strings.value(lhs.value)
                   < strings.value(rhs.as_string().value);
        }

        bool visit_symbol(const CompiledModuleMember::Symbol& lhs) const {
            return module_order_less(
                lhs.name, rhs.as_symbol().name, object, strings);
        }

        bool visit_import(const CompiledModuleMember::Import& lhs) const {
            return module_order_less(
                lhs.module_name, rhs.as_import().module_name, object, strings);
        }

        bool visit_variable(const CompiledModuleMember::Variable& lhs) const {
            return module_order_less(
                lhs.name, rhs.as_variable().name, object, strings);
        }

        bool visit_function(const CompiledModuleMember::Function& lhs) const {
            auto lfunc = object[lhs.id];
            auto rfunc = object[rhs.as_function().id];

            // Sort by type (normal functions first).
            auto ltype = lfunc->func.type();
            auto rtype = lfunc->func.type();
            if (ltype != rtype)
                return function_type_order(ltype) < function_type_order(rtype);

            // Sort by name (unnamed functions last).
            auto lname = lfunc->func.name();
            auto rname = rfunc->func.name();
            if (lname && rname)
                return strings.value(lname) < strings.value(rname);

            return !rname;
        }
    };

    return ld.visit(Comparator{object, strings, rd});
}

void ModuleCompiler::run() {
    compile_object();
    link_members();

    // TODO handle indices better.
    result_.name(module_.name());
    if (auto ir_init = module_.init())
        result_.init(resolved(ir_init));

    for (u32 i = 0, e = final_members_.size(); i < e; ++i) {
        auto new_id = result_.make(std::move(final_members_[i]));
        TIRO_CHECK(new_id.value() == i,
            "Implementation requirement: same index as before is assigned.");
    }

    for (auto func_id : object_.function_ids()) {
        auto func_item = object_[func_id];
        auto new_func_id = result_.make(std::move(func_item->func));
        TIRO_CHECK(func_id == new_func_id,
            "Implementation requirement: same index as before is assigned.");
    }
}

void ModuleCompiler::link_members() {
    auto order = reorder_members();

    std::vector<CompiledModuleMember> final_members;
    final_members.reserve(order.size());
    for (u32 i = 0, e = order.size(); i < e; ++i) {
        auto old_id = order[i];
        auto new_id = CompiledModuleMemberID(i);

        auto& old_def = object_[old_id]->as_definition();
        if (old_def.ir_id) {
            defs_[old_def.ir_id] = new_id;
        }
        renamed_[old_id] = new_id;
        final_members.push_back(std::move(old_def.value));
    }

    fix_references(final_members);
    final_members_ = std::move(final_members);
}

// Every definition is assigned a new index.
// Skips "use" instances since they will be resolved and will not
// be present in the compiled output.
std::vector<CompiledModuleMemberID> ModuleCompiler::reorder_members() const {
    std::vector<CompiledModuleMemberID> member_order;
    {
        for (const auto& id : object_.item_ids()) {
            auto item = object_[id];
            if (item->type() == LinkItemType::Definition)
                member_order.push_back(id);
        }
        std::sort(member_order.begin(), member_order.end(),
            [&](const auto& lhs, const auto& rhs) {
                return module_order_less(lhs, rhs, object_, strings());
            });
    }
    return member_order;
}

void ModuleCompiler::fix_references(
    std::vector<CompiledModuleMember>& members) {
    struct Visitor {
        ModuleCompiler& self;

        void visit_integer(const CompiledModuleMember::Integer&) {}

        void visit_float(const CompiledModuleMember::Float&) {}

        void visit_string(const CompiledModuleMember::String&) {}

        void visit_symbol(CompiledModuleMember::Symbol& sym) {
            sym.name = self.renamed(sym.name);
        }

        void visit_import(CompiledModuleMember::Import& imp) {
            imp.module_name = self.renamed(imp.module_name);
        }

        void visit_variable(CompiledModuleMember::Variable& var) {
            var.name = self.renamed(var.name);
        }

        void visit_function(const CompiledModuleMember::Function& func) {
            self.fix_func_references(func.id);
        }
    };

    for (auto& member : members) {
        member.visit(Visitor{*this});
    }
}

void ModuleCompiler::fix_func_references(CompiledFunctionID func_id) {
    auto func_item = object_[func_id];
    BinaryWriter writer(func_item->func.code());

    for (const auto& [offset, old_id] : func_item->refs_) {
        auto item = object_[old_id];

        CompiledModuleMemberID new_id = [&, old_id = old_id]() {
            switch (item->type()) {
            // The module index was renamed.
            case LinkItemType::Definition:
                return renamed(old_id);

            // Resolve the reference.
            case LinkItemType::Use:
                return resolved(item->as_use());
            }

            TIRO_UNREACHABLE("Invalid link item type.");
        }();

        // TODO quick and dirty patching
        static_assert(
            std::is_same_v<u32, CompiledModuleMemberID::UnderlyingType>);
        writer.overwrite_u32(offset, new_id.value());
    }
}

void ModuleCompiler::compile_object() {
    std::vector<ModuleMemberID> members;
    for (const auto id : module_.member_ids()) {
        members.push_back(id);
    }
    object_ = tiro::compile_object(module_, members);
}

CompiledModule compile_module([[maybe_unused]] Module& module) {
    CompiledModule result(module.strings());
    ModuleCompiler compiler(module, result);
    compiler.run();
    return result;
}

} // namespace tiro
