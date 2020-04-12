#include "tiro/vm/load.hpp"

#include "tiro/bytecode/module.hpp"
#include "tiro/core/overloaded.hpp"
#include "tiro/heap/handles.hpp"
#include "tiro/objects/classes.hpp"
#include "tiro/objects/functions.hpp"
#include "tiro/objects/modules.hpp"
#include "tiro/objects/strings.hpp"
#include "tiro/vm/context.hpp"

#include "tiro/vm/context.ipp"

namespace tiro::vm {

static constexpr u32 max_module_size = 1 << 20; // # of members

static_assert(
    std::is_same_v<BytecodeMemberID::UnderlyingType, u32>, "Type mismatch.");

namespace {

class ModuleLoader final {
public:
    explicit ModuleLoader(Context& ctx, const BytecodeModule& compiled_module,
        const StringTable& strings);

    Module run();

    Value visit_integer(const BytecodeMember::Integer& i, u32 index);
    Value visit_float(const BytecodeMember::Float& f, u32 index);
    Value visit_string(const BytecodeMember::String& s, u32 index);
    Value visit_symbol(const BytecodeMember::Symbol& s, u32 index);
    Value visit_import(const BytecodeMember::Import& i, u32 index);
    Value visit_variable(const BytecodeMember::Variable& v, u32 index);
    Value visit_function(const BytecodeMember::Function& f, u32 index);

private:
    // While loading members: module level indices must point to elements that have already been encountered.
    u32 seen(u32 current, BytecodeMemberID test);

    // Must be in range.
    u32 valid(BytecodeMemberID test);

    [[noreturn]] void err(const SourceLocation& src, std::string_view message);

private:
    Context& ctx_;
    const BytecodeModule& compiled_;
    const StringTable& strings_;

    Root<Tuple> members_;
    Root<HashTable> exported_; // TODO: Not implemented
    Root<Module> module_;
};

} // namespace

ModuleLoader::ModuleLoader(Context& ctx, const BytecodeModule& compiled_module,
    const StringTable& strings)
    : ctx_(ctx)
    , compiled_(compiled_module)
    , strings_(strings)
    , members_(ctx)
    , exported_(ctx)
    , module_(ctx) {

    // TODO exported!

    Root module_name(
        ctx_, ctx_.get_interned_string(strings.value(compiled_.name())));
    members_.set(Tuple::make(ctx_, compiled_.member_count()));
    module_.set(Module::make(ctx_, module_name, members_, exported_));
}

Module ModuleLoader::run() {
    for (const auto member_id : compiled_.member_ids()) {
        const u32 index = member_id.value();
        const auto member = compiled_[member_id];

        Root value(ctx_, member->visit(*this, index));
        members_->set(index, value);
    }

    const auto init_id = compiled_.init();
    if (init_id) {
        const auto init_index = valid(init_id);
        Root init(ctx_, members_->get(init_index));
        module_->init(init);
    }

    // TODO: Smarter loading algorithm - should not eagerly init modules.
    // - Call the init functions when the module is being imported for the first time?
    // - Call *all* init functions after bootstrap is complete? <-- Prefer this eager version
    {
        Root<Value> init(ctx_, module_->init());
        if (!init->is_null()) {
            ctx_.run(init, {});
        }
    }

    return module_;
}

Value ModuleLoader::visit_integer(
    const BytecodeMember::Integer& i, [[maybe_unused]] u32 index) {
    return ctx_.get_integer(i.value);
}

Value ModuleLoader::visit_float(
    const BytecodeMember::Float& f, [[maybe_unused]] u32 index) {
    return Float::make(ctx_, f.value);
}

Value ModuleLoader::visit_string(const BytecodeMember::String& s, u32 index) {
    if (!s.value) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format(
                "Invalid string in module definition (at index {}).", index));
    }
    return ctx_.get_interned_string(strings_.value(s.value));
}

Value ModuleLoader::visit_symbol(const BytecodeMember::Symbol& s, u32 index) {
    const auto name_index = seen(index, s.name);

    Root name(ctx_, members_->get(name_index));
    if (!name->is<String>()) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format("Module member at index {} is not a string.", index));
    }
    return ctx_.get_symbol(name.handle().cast<String>());
}

Value ModuleLoader::visit_import(const BytecodeMember::Import& i, u32 index) {
    const auto name_index = seen(index, i.module_name);

    Root name(ctx_, members_->get(name_index));
    if (!name->is<String>()) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format("Module member at index {} is not a string.", index));
    }

    Root<Module> imported(ctx_);
    if (!ctx_.find_module(
            name.handle().cast<String>(), imported.mut_handle())) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format("Failed to import module {}: the module was not found.",
                name->as_strict<String>().view()));
    }
    return imported;
}

Value ModuleLoader::visit_variable(
    [[maybe_unused]] const BytecodeMember::Variable& v,
    [[maybe_unused]] u32 index) {
    // TODO: Support constant values here if variable
    // is expanded to support constant initializers
    return ctx_.get_undefined();
}

Value ModuleLoader::visit_function(
    const BytecodeMember::Function& f, [[maybe_unused]] u32 index) {
    if (!f.id) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format("Refers to an invalid function (at index {}).", index));
    }

    auto func = compiled_[f.id];

    Root function_name(ctx_,
        ctx_.get_interned_string(strings_.value_or(func->name(), "<UNNAMED>")));
    Root tmpl(ctx_, FunctionTemplate::make(ctx_, function_name, module_,
                        func->params(), func->locals(), func->code()));

    switch (func->type()) {
    case BytecodeFunctionType::Normal:
        return Function::make(ctx_, tmpl, Handle<Environment>());
    case BytecodeFunctionType::Closure:
        return tmpl;
    }
    TIRO_UNREACHABLE("Invalid function type.");
}

u32 ModuleLoader::seen(u32 current, BytecodeMemberID test) {
    const auto index = valid(test);
    if (index >= current) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format("Module member {} has not been visited yet (at "
                        "index {}).",
                index, current));
    }
    return index;
}

// Must be in range.
u32 ModuleLoader::valid(BytecodeMemberID test) {
    if (!test) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format("references an invalid member."));
    }

    const auto index = test.value();
    if (index >= compiled_.member_count()) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format("Module member {} has is out of bounds.", test));
    }

    return index;
}

void ModuleLoader::err(const SourceLocation& src, std::string_view message) {
    const auto name = strings_.dump(compiled_.name());
    throw_internal_error(src, "Module {}: {}", name, message);
}

Module load_module(Context& ctx, const BytecodeModule& compiled_module,
    const StringTable& strings) {
    TIRO_CHECK(compiled_module.name().valid(),
        "Module definition without a valid module name.");
    TIRO_CHECK(compiled_module.member_count() <= max_module_size,
        "Module definition is too large.");

    ModuleLoader loader(ctx, compiled_module, strings);
    return loader.run();
}

} // namespace tiro::vm
