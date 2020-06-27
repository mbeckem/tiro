#include "vm/load.hpp"

#include "common/overloaded.hpp"
#include "compiler/bytecode/module.hpp"
#include "vm/context.hpp"
#include "vm/heap/handles.hpp"
#include "vm/objects/class.hpp"
#include "vm/objects/function.hpp"
#include "vm/objects/module.hpp"
#include "vm/objects/string.hpp"

#include "vm/context.ipp"

namespace tiro::vm {

static constexpr u32 max_module_size = 1 << 20; // # of members

static_assert(std::is_same_v<BytecodeMemberId::UnderlyingType, u32>, "Type mismatch.");

namespace {

class ModuleLoader final {
public:
    explicit ModuleLoader(Context& ctx, const BytecodeModule& compiled_module);

    Module run();

    Value visit_integer(const BytecodeMember::Integer& i, u32 index);
    Value visit_float(const BytecodeMember::Float& f, u32 index);
    Value visit_string(const BytecodeMember::String& s, u32 index);
    Value visit_symbol(const BytecodeMember::Symbol& s, u32 index);
    Value visit_import(const BytecodeMember::Import& i, u32 index);
    Value visit_variable(const BytecodeMember::Variable& v, u32 index);
    Value visit_function(const BytecodeMember::Function& f, u32 index);

private:
    void create_export(u32 symbol_index, u32 value_index);

    // While loading members: module level indices must point to elements that have already been encountered.
    u32 seen(u32 current, BytecodeMemberId test);

    // Must be in range.
    u32 valid(BytecodeMemberId test);

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

ModuleLoader::ModuleLoader(Context& ctx, const BytecodeModule& compiled_module)
    : ctx_(ctx)
    , compiled_(compiled_module)
    , strings_(compiled_module.strings())
    , members_(ctx)
    , exported_(ctx)
    , module_(ctx) {

    Root module_name(ctx_, ctx_.get_interned_string(strings_.value(compiled_.name())));
    members_.set(Tuple::make(ctx_, compiled_.member_count()));
    exported_.set(HashTable::make(ctx_));
    module_.set(Module::make(ctx_, module_name, members_, exported_));
}

Module ModuleLoader::run() {
    for (const auto member_id : compiled_.member_ids()) {
        const u32 index = valid(member_id);
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
    // - Call *all* init functions after bootstrap is complete?
    {
        Root<Value> init(ctx_, module_->init());
        if (!init->is_null()) {
            ctx_.run(init, {});
        }
    }

    // Exports must run after init because the exports table contains copies of the exported values.
    // This is possible because init is eager and because exported values must be constant.
    // (This may be not be a great idea, the exports table could also contain integer indices into the
    // members tuple).
    for (auto [symbol_id, value_id] : compiled_.exports()) {
        create_export(valid(symbol_id), valid(value_id));
    }

    return module_;
}

Value ModuleLoader::visit_integer(const BytecodeMember::Integer& i, [[maybe_unused]] u32 index) {
    return ctx_.get_integer(i.value);
}

Value ModuleLoader::visit_float(const BytecodeMember::Float& f, [[maybe_unused]] u32 index) {
    return Float::make(ctx_, f.value);
}

Value ModuleLoader::visit_string(const BytecodeMember::String& s, u32 index) {
    if (!s.value) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format("Invalid string in module definition (at index {}).", index));
    }
    return ctx_.get_interned_string(strings_.value(s.value));
}

Value ModuleLoader::visit_symbol(const BytecodeMember::Symbol& s, u32 index) {
    const auto name_index = seen(index, s.name);

    Root name(ctx_, members_->get(name_index));
    if (!name->is<String>()) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format("Module member at index {} is not a string.", name_index));
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
    if (!ctx_.find_module(name.handle().cast<String>(), imported.mut_handle())) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format("Failed to import module {}: the module was not found.",
                name->as_strict<String>().view()));
    }
    return imported;
}

Value ModuleLoader::visit_variable(
    [[maybe_unused]] const BytecodeMember::Variable& v, [[maybe_unused]] u32 index) {
    // TODO: Support constant values here if variable
    // is expanded to support constant initializers
    return ctx_.get_undefined();
}

Value ModuleLoader::visit_function(const BytecodeMember::Function& f, u32 index) {
    if (!f.id) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format("Refers to an invalid function (at index {}).", index));
    }

    auto func = compiled_[f.id];

    Root<String> name(ctx_);
    if (func->name()) {
        const auto name_index = seen(index, func->name());

        auto name_value = members_->get(name_index);
        if (!name_value.is<String>()) {
            err(TIRO_SOURCE_LOCATION(),
                fmt::format("Module member at index {} is not a string.", name_index));
        }
        name.set(name_value.as_strict<String>());
    } else {
        name.set(ctx_.get_interned_string("<UNNAMED>"));
    }

    Root tmpl(ctx_,
        FunctionTemplate::make(ctx_, name, module_, func->params(), func->locals(), func->code()));

    switch (func->type()) {
    case BytecodeFunctionType::Normal:
        return Function::make(ctx_, tmpl, Handle<Environment>());
    case BytecodeFunctionType::Closure:
        return tmpl;
    }
    TIRO_UNREACHABLE("Invalid function type.");
}

void ModuleLoader::create_export(u32 symbol_index, u32 value_index) {
    Root symbol(ctx_, members_->get(symbol_index));
    if (!symbol->is<Symbol>()) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format(
                "Module member at index {} used as export name is not a symbol.", symbol_index));
    }

    if (exported_->contains(symbol)) {
        err(TIRO_SOURCE_LOCATION(), fmt::format("The name '{}' is exported more than once.",
                                        symbol->as<Symbol>().name().view()));
    }

    Root value(ctx_, members_->get(value_index));
    exported_->set(ctx_, symbol, value);
}

u32 ModuleLoader::seen(u32 current, BytecodeMemberId test) {
    const auto index = valid(test);
    if (index >= current) {
        err(TIRO_SOURCE_LOCATION(), fmt::format("Module member {} has not been visited yet (at "
                                                "index {}).",
                                        index, current));
    }
    return index;
}

// Must be in range.
u32 ModuleLoader::valid(BytecodeMemberId test) {
    if (!test) {
        err(TIRO_SOURCE_LOCATION(), fmt::format("references an invalid member."));
    }

    const auto index = test.value();
    if (index >= compiled_.member_count()) {
        err(TIRO_SOURCE_LOCATION(), fmt::format("Module member {} has is out of bounds.", test));
    }

    return index;
}

void ModuleLoader::err(const SourceLocation& src, std::string_view message) {
    const auto name = strings_.dump(compiled_.name());
    throw_internal_error(src, "Module {}: {}", name, message);
}

Module load_module(Context& ctx, const BytecodeModule& compiled_module) {
    TIRO_CHECK(compiled_module.name().valid(), "Module definition without a valid module name.");
    TIRO_CHECK(
        compiled_module.member_count() <= max_module_size, "Module definition is too large.");

    ModuleLoader loader(ctx, compiled_module);
    return loader.run();
}

} // namespace tiro::vm
