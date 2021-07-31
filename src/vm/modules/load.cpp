#include "vm/modules/load.hpp"

#include "bytecode/module.hpp"
#include "vm/context.hpp"
#include "vm/handles/scope.hpp"
#include "vm/math.hpp"
#include "vm/modules/verify.hpp"
#include "vm/objects/all.hpp"

namespace tiro::vm {
namespace {

class ModuleLoader final {
public:
    static_assert(std::is_same_v<BytecodeMemberId::UnderlyingType, u32>, "Type mismatch.");

    static constexpr u32 max_module_size = 1 << 20; // # of members

    explicit ModuleLoader(Context& ctx, const BytecodeModule& compiled_module);

    Module run();

    Value visit_integer(const BytecodeMember::Integer& i);
    Value visit_float(const BytecodeMember::Float& f);
    Value visit_string(const BytecodeMember::String& s);
    Value visit_symbol(const BytecodeMember::Symbol& s);
    Value visit_import(const BytecodeMember::Import& i);
    Value visit_variable(const BytecodeMember::Variable& v);
    Value visit_function(const BytecodeMember::Function& f);
    Value visit_record_template(const BytecodeMember::RecordTemplate& r);

private:
    void create_export(u32 symbol_index, u32 value_index);

private:
    Context& ctx_;
    const BytecodeModule& compiled_;
    const StringTable& strings_;

    Scope sc_;
    Local<Module> module_;
    Local<Tuple> members_;
    Local<HashTable> exported_;
};

} // namespace

Module load_module(Context& ctx, const BytecodeModule& compiled_module) {
    if (TIRO_UNLIKELY(compiled_module.member_count() > ModuleLoader::max_module_size))
        TIRO_ERROR("module has too many members");

    verify_module(compiled_module);
    ModuleLoader loader(ctx, compiled_module);
    return loader.run();
}

static Module create_module(Context& ctx, const BytecodeModule& compiled_module) {
    const StringTable& strings = compiled_module.strings();

    Scope sc(ctx);
    Local name = sc.local(ctx.get_interned_string(strings.value(compiled_module.name())));
    Local members = sc.local(Tuple::make(ctx, compiled_module.member_count()));
    Local exported = sc.local(HashTable::make(ctx));
    return Module::make(ctx, name, members, exported);
}

ModuleLoader::ModuleLoader(Context& ctx, const BytecodeModule& compiled_module)
    : ctx_(ctx)
    , compiled_(compiled_module)
    , strings_(compiled_module.strings())
    , sc_(ctx_)
    , module_(sc_.local(create_module(ctx, compiled_)))
    , members_(sc_.local(module_->members()))
    , exported_(sc_.local(module_->exported())) {}

Module ModuleLoader::run() {
    Scope sc(ctx_);
    Local value = sc.local();
    Local init = sc.local();

    for (const auto member_id : compiled_.member_ids()) {
        const auto& member = compiled_[member_id];
        value = member.visit(*this);
        members_->unchecked_set(member_id.value(), *value);
    }

    for (auto [symbol_id, value_id] : compiled_.exports()) {
        create_export(symbol_id.value(), value_id.value());
    }

    const auto init_id = compiled_.init();
    if (init_id) {
        const auto init_index = init_id.value();
        init = members_->unchecked_get(init_index);
        module_->initializer(*init);
    }

    return *module_;
}

Value ModuleLoader::visit_integer(const BytecodeMember::Integer& i) {
    return ctx_.get_integer(i.value);
}

Value ModuleLoader::visit_float(const BytecodeMember::Float& f) {
    return Float::make(ctx_, f.value);
}

Value ModuleLoader::visit_string(const BytecodeMember::String& s) {
    return ctx_.get_interned_string(strings_.value(s.value));
}

Value ModuleLoader::visit_symbol(const BytecodeMember::Symbol& s) {
    Scope sc(ctx_);
    Local name = sc.local(members_->unchecked_get(s.name.value()));
    return ctx_.get_symbol(name.must_cast<String>());
}

Value ModuleLoader::visit_import(const BytecodeMember::Import& i) {
    Scope sc(ctx_);
    Local name = sc.local(members_->checked_get(i.module_name.value()));
    return UnresolvedImport::make(ctx_, name.must_cast<String>());
}

Value ModuleLoader::visit_variable([[maybe_unused]] const BytecodeMember::Variable& v) {
    // TODO: Support constant values here if variable
    // is expanded to support constant initializers
    return ctx_.get_undefined();
}

Value ModuleLoader::visit_function(const BytecodeMember::Function& f) {
    const auto& func = compiled_[f.id];

    Scope sc(ctx_);
    Local name = sc.local<String>(defer_init);
    if (func.name()) {
        name = members_->checked_get(func.name().value()).must_cast<String>();
    } else {
        name = ctx_.get_interned_string("<UNNAMED>");
    }

    absl::InlinedVector<HandlerTable::Entry, 8> handlers;
    handlers.reserve(func.handlers().size());
    for (const auto& handler : func.handlers()) {
        handlers.push_back({handler.from.value(), handler.to.value(), handler.target.value()});
    }

    Local tmpl = sc.local(CodeFunctionTemplate::make(
        ctx_, name, module_, func.params(), func.locals(), handlers, func.code()));

    switch (func.type()) {
    case BytecodeFunctionType::Normal:
        return CodeFunction::make(ctx_, tmpl, {});
    case BytecodeFunctionType::Closure:
        return *tmpl;
    }
    TIRO_UNREACHABLE("Invalid function type.");
}

Value ModuleLoader::visit_record_template(const BytecodeMember::RecordTemplate& r) {
    const auto& compiled_tmpl = compiled_[r.id];
    Scope sc(ctx_);
    Local keys = sc.local(Array::make(ctx_, compiled_tmpl.keys().size()));
    Local key = sc.local<Symbol>(defer_init);
    for (const auto& compiled_key : compiled_tmpl.keys()) {
        key = members_->checked_get(compiled_key.value()).must_cast<Symbol>();
        keys->append(ctx_, key).must("failed to add record key"); // array has enough capacity
    }
    return RecordTemplate::make(ctx_, keys);
}

void ModuleLoader::create_export(u32 symbol_index, u32 value_index) {
    Scope sc(ctx_);
    Local symbol = sc.local(members_->checked_get(symbol_index).must_cast<Symbol>());
    Local index = sc.local(ctx_.get_integer(value_index));
    exported_->set(ctx_, symbol, index).must("failed to insert export");
}

} // namespace tiro::vm
