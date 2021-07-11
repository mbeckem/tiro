#include "vm/modules/verify.hpp"

#include "bytecode/function.hpp"
#include "bytecode/module.hpp"

#include <absl/container/flat_hash_set.h>

namespace tiro::vm {

namespace {

class ModuleVerifier final {
public:
    explicit ModuleVerifier(const BytecodeModule& module)
        : module_(module) {}

    void verify();

    // Performs bounds checking on the given member id
    // and returns a reference to the member on success.
    const BytecodeMember& check_reference(BytecodeMemberId id, BytecodeMemberId parent);
    [[noreturn]] TIRO_COLD void fail(std::string_view message);

    // module member visitor
    void visit_integer(const BytecodeMember::Integer& i, BytecodeMemberId id);
    void visit_float(const BytecodeMember::Float& f, BytecodeMemberId id);
    void visit_string(const BytecodeMember::String& s, BytecodeMemberId id);
    void visit_symbol(const BytecodeMember::Symbol& s, BytecodeMemberId id);
    void visit_import(const BytecodeMember::Import& i, BytecodeMemberId id);
    void visit_variable(const BytecodeMember::Variable& v, BytecodeMemberId id);
    void visit_function(const BytecodeMember::Function& f, BytecodeMemberId id);
    void visit_record_template(const BytecodeMember::RecordTemplate& r, BytecodeMemberId id);

private:
    const BytecodeModule& module_;
    u32 seen_member_ids_ = 0;
};

class FunctionVerifier final {
public:
    explicit FunctionVerifier(
        BytecodeMemberId id, const BytecodeFunction& function, ModuleVerifier& parent)
        : id_(id)
        , function_(function)
        , parent_(parent) {
        TIRO_DEBUG_ASSERT(id_, "invalid function member id");
    }

    void verify();

    const BytecodeMember& check_reference(BytecodeMemberId id) {
        return parent_.check_reference(id, id_);
    }

    [[noreturn]] TIRO_COLD void fail(std::string_view message) {
        parent_.fail(fmt::format("{} (in function member {})", message, id_.value()));
    }

private:
    BytecodeMemberId id_;
    const BytecodeFunction& function_;
    ModuleVerifier& parent_;
};

} // namespace

void ModuleVerifier::verify() {
    if (!module_.name())
        fail("module does not have a valid name");

    // Validate data
    for (const auto member_id : module_.member_ids()) {
        const auto& member = module_[member_id];
        member.visit(*this, member_id);
        ++seen_member_ids_;
    }

    // Validate functions
    for (const auto& member_id : module_.member_ids()) {
        const auto& member = module_[member_id];
        if (member.type() != BytecodeMemberType::Function)
            continue;

        FunctionVerifier verifier(member_id, module_[member.as_function().id], *this);
        verifier.verify();
    }

    // Validate initializer function (if present)
    if (auto init_id = module_.init()) {
        const auto& init = check_reference(init_id, {});
        if (init.type() != BytecodeMemberType::Function) {
            fail(fmt::format(
                "member {} is not a function (required by module init)", init_id.value()));
        }

        auto& func = module_[init.as_function().id];
        if (func.type() != BytecodeFunctionType::Normal) {
            fail(fmt::format(
                "member {} is not a normal function (required by module init)", init_id.value()));
        }
    }

    // Validate exports
    for (const auto [symbol_id, value_id] : module_.exports()) {
        const auto& symbol = check_reference(symbol_id, {});
        if (symbol.type() != BytecodeMemberType::Symbol) {
            fail(fmt::format(
                "member {} is not a symbol (required by usage as export name)", symbol_id.value()));
        }

        const auto& value = check_reference(value_id, {});
        switch (value.type()) {
        case BytecodeMemberType::Import:
        case BytecodeMemberType::RecordTemplate:
            fail("forbidden export of internal type");

        default:
            break;
        }
    }
}

void ModuleVerifier::visit_integer(
    [[maybe_unused]] const BytecodeMember::Integer& i, [[maybe_unused]] BytecodeMemberId id) {}

void ModuleVerifier::visit_float(
    [[maybe_unused]] const BytecodeMember::Float& f, [[maybe_unused]] BytecodeMemberId id) {}

void ModuleVerifier::visit_string(const BytecodeMember::String& s, BytecodeMemberId id) {
    if (!s.value) {
        fail(fmt::format("invalid string (in member {})", id.value()));
    }
}

void ModuleVerifier::visit_symbol(const BytecodeMember::Symbol& s, BytecodeMemberId id) {
    const auto& name = check_reference(s.name, id);
    if (name.type() != BytecodeMemberType::String) {
        fail(fmt::format(
            "member {} is not a string (required by symbol at {})", s.name.value(), id.value()));
    }
}

void ModuleVerifier::visit_import(const BytecodeMember::Import& i, BytecodeMemberId id) {
    const auto& module_name = check_reference(i.module_name, id);
    if (module_name.type() != BytecodeMemberType::String) {
        fail(fmt::format("member {} is not a string (required by import at {})",
            i.module_name.value(), id.value()));
    }
}

void ModuleVerifier::visit_variable(
    [[maybe_unused]] const BytecodeMember::Variable& v, [[maybe_unused]] BytecodeMemberId id) {}

void ModuleVerifier::visit_function(const BytecodeMember::Function& f, BytecodeMemberId id) {
    if (!f.id) {
        fail(fmt::format("invalid function reference (in member {})", id.value()));
    }

    const auto& func = module_[f.id];
    if (auto name_id = func.name()) {
        const auto& name = check_reference(func.name(), id);
        if (name.type() != BytecodeMemberType::String) {
            fail(fmt::format("member {} is not a string (required by function at {})",
                name_id.value(), id.value()));
        }
    }

    // Code and handlers are verified when all members have been seen.
}

void ModuleVerifier::visit_record_template(
    const BytecodeMember::RecordTemplate& r, BytecodeMemberId id) {
    if (!r.id) {
        fail(fmt::format("invalid record template reference (in member {})", id.value()));
    }

    const auto& tmpl = module_[r.id];
    for (auto key_id : tmpl.keys()) {
        const auto& key = check_reference(key_id, id);
        if (key.type() != BytecodeMemberType::Symbol) {
            fail(fmt::format("member {} is not a symbol (required by record template at {})",
                key_id.value(), id.value()));
        }
    }
}

const BytecodeMember&
ModuleVerifier::check_reference(BytecodeMemberId id, BytecodeMemberId parent) {
    auto context = [&]() -> std::string {
        if (parent) {
            return fmt::format("(referenced by member {})", parent.value());
        }
        return "(referenced by module)";
    };

    if (!id)
        fail(fmt::format("invalid module member id {}", context()));

    if (id.value() >= seen_member_ids_)
        fail(fmt::format("member id {} has not been visited yet {}", id.value(), context()));

    if (!module_.members().in_bounds(id))
        fail(fmt::format("member id {} is out of bounds {}", id.value(), context()));

    return module_[id];
}

void ModuleVerifier::fail(std::string_view message) {
    // TODO: Mark exception as bytecode verification failure?
    std::string_view name = module_.strings().dump(module_.name());
    TIRO_ERROR("module '{}' verification error: {}", name, message);
}

void FunctionVerifier::verify() {}

void verify_module(const BytecodeModule& module) {
    ModuleVerifier verifier(module);
    verifier.verify();
}

} // namespace tiro::vm
