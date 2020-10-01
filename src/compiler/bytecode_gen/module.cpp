#include "compiler/bytecode_gen/module.hpp"

#include "bytecode/instruction.hpp"
#include "bytecode/module.hpp"
#include "common/binary.hpp"
#include "common/hash.hpp"
#include "compiler/bytecode_gen/func.hpp"
#include "compiler/ir/module.hpp"

#include "absl/container/flat_hash_map.h"
#include "absl/container/inlined_vector.h"

#include <algorithm>

namespace tiro {

namespace {

class ModuleCompiler final {
public:
    explicit ModuleCompiler(Module& module, BytecodeModule& result)
        : module_(module)
        , result_(result) {}

    StringTable& strings() const { return module_.strings(); }

    void run();

private:
    using DefinitionMap = absl::flat_hash_map<ModuleMemberId, BytecodeMemberId, UseHasher>;
    using RenameMap = absl::flat_hash_map<BytecodeMemberId, BytecodeMemberId, UseHasher>;
    using StringMap = absl::flat_hash_map<InternedString, InternedString, UseHasher>;

    // Improvement: could split members and parallelize, or split
    // them by source file and compile & link incrementally.
    // Would make merging of objects a requirement.
    void compile_object();

    void link_members();

    void define_exports();

    std::vector<BytecodeMemberId> reorder_members() const;

    void fix_references(std::vector<BytecodeMember>& members);

    void fix_func_references(BytecodeFunctionId func_id);

    void fix_strings(BytecodeMember& member);

    BytecodeMemberId renamed(BytecodeMemberId old) const {
        auto pos = renamed_.find(old);
        TIRO_CHECK(pos != renamed_.end(), "Module member was not assigned a new position.");
        return pos->second;
    }

    BytecodeMemberId resolved(ModuleMemberId ir_id) const {
        auto pos = defs_.find(ir_id);
        TIRO_CHECK(pos != defs_.end(), "Module member was never defined.");
        return pos->second;
    }

    InternedString result_str(InternedString ir_str) {
        if (!ir_str)
            return ir_str;

        if (auto pos = string_map_.find(ir_str); pos != string_map_.end())
            return pos->second;

        // Improvement: StringTable unsafe api for strings that are guaranteed to be unique?
        auto bc_str = result_.strings().insert(module_.strings().value(ir_str));
        string_map_.emplace(ir_str, bc_str);
        return bc_str;
    }

private:
    Module& module_;
    BytecodeModule& result_;

    LinkObject object_;

    // Definitions of ir module members in the compiled representation.
    // Refers to the final module index (not the index in the object).
    DefinitionMap defs_;

    // Old index (in object) to new object (in output).
    RenameMap renamed_;

    // Maps source strings (used in the compilation process) to output strings
    // (used in the bytecode module).
    StringMap string_map_;

    std::vector<BytecodeMember> final_members_;
};

} // namespace

static int module_type_order(BytecodeMemberType type) {
    switch (type) {
    case BytecodeMemberType::Integer:
        return 0;
    case BytecodeMemberType::Float:
        return 1;
    case BytecodeMemberType::String:
        return 2;
    case BytecodeMemberType::Symbol:
        return 3;
    case BytecodeMemberType::Import:
        return 4;
    case BytecodeMemberType::Variable:
        return 5;
    case BytecodeMemberType::RecordTemplate:
        return 6;
    case BytecodeMemberType::Function:
        return 7;
    }

    TIRO_UNREACHABLE("Invalid compiled module member type.");
}

static int function_type_order(BytecodeFunctionType type) {
    switch (type) {
    case BytecodeFunctionType::Normal:
        return 0;
    case BytecodeFunctionType::Closure:
        return 1;
    }

    TIRO_UNREACHABLE("Invalid compiled function type.");
}

static bool module_order_less(BytecodeMemberId lhs, BytecodeMemberId rhs, const LinkObject& object,
    const StringTable& strings) {
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
        const BytecodeMember& rhs;

        bool visit_integer(const BytecodeMember::Integer& lhs) const {
            return lhs.value < rhs.as_integer().value;
        }

        bool visit_float(const BytecodeMember::Float& lhs) const {
            return lhs.value < rhs.as_float().value;
        }

        bool visit_string(const BytecodeMember::String& lhs) const {
            return strings.value(lhs.value) < strings.value(rhs.as_string().value);
        }

        bool visit_symbol(const BytecodeMember::Symbol& lhs) const {
            return module_order_less(lhs.name, rhs.as_symbol().name, object, strings);
        }

        bool visit_import(const BytecodeMember::Import& lhs) const {
            return module_order_less(lhs.module_name, rhs.as_import().module_name, object, strings);
        }

        bool visit_variable(const BytecodeMember::Variable& lhs) const {
            return module_order_less(lhs.name, rhs.as_variable().name, object, strings);
        }

        bool visit_function(const BytecodeMember::Function& lhs) const {
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
                return module_order_less(lname, rname, object, strings);
            if (lname)
                return true;
            return false;
        }

        bool visit_record_template(const BytecodeMember::RecordTemplate& lhs) const {
            const auto& lkeys = object[lhs.id]->keys();
            const auto& rkeys = object[rhs.as_record_template().id]->keys();
            return std::lexicographical_compare(lkeys.begin(), lkeys.end(), rkeys.begin(),
                rkeys.end(),
                [&](auto l, auto r) { return module_order_less(l, r, object, strings); });
        }
    };

    return ld.visit(Comparator{object, strings, rd});
}

void ModuleCompiler::run() {
    compile_object();
    link_members();
    define_exports();

    // TODO handle indices better.
    result_.name(result_str(module_.name()));
    if (auto ir_init = module_.init())
        result_.init(resolved(ir_init));

    for (u32 i = 0, e = final_members_.size(); i < e; ++i) {
        auto member = std::move(final_members_[i]);
        fix_strings(member);

        auto new_id = result_.make(std::move(member));
        TIRO_CHECK(new_id.value() == i, "Implementation requirement: same index is assigned.");
    }

    for (auto func_id : object_.function_ids()) {
        auto func = std::move(object_[func_id]->func);

        auto new_func_id = result_.make(std::move(func));
        TIRO_CHECK(func_id == new_func_id, "Implementation requirement: same index is assigned.");
    }

    for (auto record_id : object_.record_ids()) {
        auto rec = std::move(object_[record_id]);

        auto new_record_id = result_.make(std::move(*rec));
        TIRO_CHECK(
            record_id == new_record_id, "Implementation requirement: same index is assigned.");
    }
}

void ModuleCompiler::compile_object() {
    // Improvement: create multiple objects here and merge them later.
    // This would be a first step towards incremental compilation (if needed).
    std::vector<ModuleMemberId> members;
    for (const auto id : module_.member_ids()) {
        members.push_back(id);
    }
    object_ = tiro::compile_object(module_, members);
}

void ModuleCompiler::link_members() {
    auto order = reorder_members();

    std::vector<BytecodeMember> final_members;
    final_members.reserve(order.size());
    for (u32 i = 0, e = order.size(); i < e; ++i) {
        auto old_id = order[i];
        auto new_id = BytecodeMemberId(i);

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

void ModuleCompiler::define_exports() {
    absl::InlinedVector<std::pair<BytecodeMemberId, BytecodeMemberId>, 16> exports;
    for (const auto& [symbol_id, value_id] : object_.exports()) {
        auto new_symbol_id = renamed(symbol_id);
        auto new_value_id = renamed(value_id);

        TIRO_DEBUG_ASSERT(
            final_members_.at(new_symbol_id.value()).type() == BytecodeMemberType::Symbol,
            "The exported name must be a symbol value.");
        exports.emplace_back(new_symbol_id, new_value_id);
    }

    std::sort(exports.begin(), exports.end(),
        [&](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
    for (const auto& [new_symbol_id, new_value_id] : exports)
        result_.add_export(new_symbol_id, new_value_id);
}

// Every definition is assigned a new index.
// Skips "use" instances since they will be resolved and will not
// be present in the compiled output.
std::vector<BytecodeMemberId> ModuleCompiler::reorder_members() const {
    std::vector<BytecodeMemberId> member_order;
    {
        for (const auto& id : object_.item_ids()) {
            auto item = object_[id];
            if (item->type() == LinkItemType::Definition)
                member_order.push_back(id);
        }
        std::sort(member_order.begin(), member_order.end(), [&](const auto& lhs, const auto& rhs) {
            return module_order_less(lhs, rhs, object_, strings());
        });
    }
    return member_order;
}

void ModuleCompiler::fix_references(std::vector<BytecodeMember>& members) {
    struct Visitor {
        ModuleCompiler& self;

        void visit_integer(const BytecodeMember::Integer&) {}

        void visit_float(const BytecodeMember::Float&) {}

        void visit_string(const BytecodeMember::String&) {}

        void visit_symbol(BytecodeMember::Symbol& sym) { sym.name = self.renamed(sym.name); }

        void visit_import(BytecodeMember::Import& imp) {
            imp.module_name = self.renamed(imp.module_name);
        }

        void visit_variable(BytecodeMember::Variable& var) { var.name = self.renamed(var.name); }

        void visit_function(const BytecodeMember::Function& func) {
            self.fix_func_references(func.id);
        }

        void visit_record_template(const BytecodeMember::RecordTemplate& rec) {
            auto record = self.object_[rec.id];
            auto& keys = record->keys();

            for (auto& key : keys)
                key = self.renamed(key);

            std::sort(keys.begin(), keys.end());
        }
    };

    for (auto& member : members) {
        member.visit(Visitor{*this});
    }
}

void ModuleCompiler::fix_func_references(BytecodeFunctionId func_id) {
    auto func_item = object_[func_id];

    if (auto name = func_item->func.name())
        func_item->func.name(renamed(name));

    BinaryWriter writer(func_item->func.code());
    for (const auto& [offset, old_id] : func_item->refs_) {
        auto item = object_[old_id];

        BytecodeMemberId new_id = [&, old_id = old_id]() {
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
        static_assert(std::is_same_v<u32, BytecodeMemberId::UnderlyingType>);
        writer.overwrite_u32(offset, new_id.value());
    }
}

void ModuleCompiler::fix_strings(BytecodeMember& member) {
    struct Visitor {
        ModuleCompiler& self;

        void visit_string(BytecodeMember::String& s) { s.value = self.result_str(s.value); }

        void visit_integer(BytecodeMember::Integer&) {}
        void visit_float(BytecodeMember::Float&) {}
        void visit_symbol(BytecodeMember::Symbol&) {}
        void visit_import(BytecodeMember::Import&) {}
        void visit_variable(BytecodeMember::Variable&) {}
        void visit_function(BytecodeMember::Function&) {}
        void visit_record_template(BytecodeMember::RecordTemplate&) {}
    };
    member.visit(Visitor{*this});
}

BytecodeModule compile_module(Module& module) {
    BytecodeModule result;
    ModuleCompiler compiler(module, result);
    compiler.run();
    return result;
}

} // namespace tiro
