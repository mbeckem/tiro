#include "vm/module_registry.hpp"

#include "bytecode/module.hpp"
#include "vm/context.hpp"
#include "vm/handles/scope.hpp"
#include "vm/objects/all.hpp"

namespace tiro::vm {

namespace {

class ModuleLoader final {
public:
    static_assert(std::is_same_v<BytecodeMemberId::UnderlyingType, u32>, "Type mismatch.");

    static constexpr u32 max_module_size = 1 << 20; // # of members

    explicit ModuleLoader(Context& ctx, const BytecodeModule& compiled_module);

    Module run();

    Value visit_integer(const BytecodeMember::Integer& i, u32 index);
    Value visit_float(const BytecodeMember::Float& f, u32 index);
    Value visit_string(const BytecodeMember::String& s, u32 index);
    Value visit_symbol(const BytecodeMember::Symbol& s, u32 index);
    Value visit_import(const BytecodeMember::Import& i, u32 index);
    Value visit_variable(const BytecodeMember::Variable& v, u32 index);
    Value visit_function(const BytecodeMember::Function& f, u32 index);
    Value visit_record_template(const BytecodeMember::RecordTemplate& r, u32 index);

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

    Scope sc_;
    Local<Module> module_;
    Local<Tuple> members_;
    Local<HashTable> exported_;
};

} // namespace

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
        const u32 index = valid(member_id);
        const auto member = compiled_[member_id];

        value = member->visit(*this, index);
        members_->set(index, *value);
    }

    for (auto [symbol_id, value_id] : compiled_.exports()) {
        create_export(valid(symbol_id), valid(value_id));
    }

    const auto init_id = compiled_.init();
    if (init_id) {
        const auto init_index = valid(init_id);
        init = members_->get(init_index);
        module_->initializer(*init);
    }

    return *module_;
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

    Scope sc(ctx_);
    Local name = sc.local(members_->get(name_index));
    if (!name->is<String>()) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format("Module member at index {} is not a string.", name_index));
    }
    return ctx_.get_symbol(name.must_cast<String>());
}

Value ModuleLoader::visit_import(const BytecodeMember::Import& i, u32 index) {
    const auto name_index = seen(index, i.module_name);

    Scope sc(ctx_);
    Local name = sc.local(members_->get(name_index));
    if (!name->is<String>()) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format("Module member at index {} is not a string.", index));
    }

    return UnresolvedImport::make(ctx_, name.must_cast<String>());
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

    Scope sc(ctx_);
    Local name = sc.local();
    if (func->name()) {
        const auto name_index = seen(index, func->name());

        name = members_->get(name_index);
        if (!name->is<String>()) {
            err(TIRO_SOURCE_LOCATION(),
                fmt::format("Module member at index {} is not a string.", name_index));
        }
    } else {
        name = ctx_.get_interned_string("<UNNAMED>");
    }

    Local tmpl = sc.local(FunctionTemplate::make(
        ctx_, name.must_cast<String>(), module_, func->params(), func->locals(), func->code()));

    switch (func->type()) {
    case BytecodeFunctionType::Normal:
        return Function::make(ctx_, tmpl, {});
    case BytecodeFunctionType::Closure:
        return *tmpl;
    }
    TIRO_UNREACHABLE("Invalid function type.");
}

Value ModuleLoader::visit_record_template(const BytecodeMember::RecordTemplate& r, u32 index) {
    if (!r.id) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format("Refers to an invalid record template (at index {}).", index));
    }

    auto compiled_tmpl = compiled_[r.id];
    Scope sc(ctx_);
    Local keys = sc.local(Array::make(ctx_, compiled_tmpl->keys().size()));
    Local key = sc.local();
    for (const auto& compiled_key : compiled_tmpl->keys()) {
        const auto key_index = seen(index, compiled_key);

        key = members_->get(key_index);
        if (!key->is<Symbol>()) {
            err(TIRO_SOURCE_LOCATION(),
                fmt::format("Module member at index {} is not a symbol.", key_index));
        }
        keys->append(ctx_, key);
    }

    return RecordTemplate::make(ctx_, keys);
}

void ModuleLoader::create_export(u32 symbol_index, u32 value_index) {
    Scope sc(ctx_);
    Local symbol = sc.local(members_->get(symbol_index));
    if (!symbol->is<Symbol>()) {
        err(TIRO_SOURCE_LOCATION(),
            fmt::format(
                "Module member at index {} used as export name is not a symbol.", symbol_index));
    }

    if (exported_->contains(*symbol)) {
        err(TIRO_SOURCE_LOCATION(), fmt::format("The name '{}' is exported more than once.",
                                        symbol.must_cast<Symbol>()->name().view()));
    }

    Local index = sc.local(ctx_.get_integer(value_index));
    exported_->set(ctx_, symbol, index);
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

void ModuleRegistry::init(Context& ctx) {
    modules_ = HashTable::make(ctx, 64);
}

bool ModuleRegistry::add_module(Context& ctx, Handle<Module> module) {
    TIRO_CHECK(!module->name().is_null(), "Module must have a valid name.");

    if (modules_.value().contains(module->name())) {
        return false;
    }

    Scope sc(ctx);
    Local name = sc.local(module->name());
    name = ctx.get_interned_string(name);
    modules_.value().set(ctx, name, module);
    return true;
}

std::optional<Module> ModuleRegistry::get_module(Context& ctx, Handle<String> module_name) {
    Scope sc(ctx);
    Local module = sc.local<Module>(defer_init);
    if (auto found = find_module(*module_name)) {
        module = *found;
    } else {
        return {};
    }

    resolve_module(ctx, module);
    return *module;
}

// FIXME: Handle and test import cycles.
void ModuleRegistry::resolve_module(Context& ctx, Handle<Module> module) {
    struct Frame {
        UniqueExternal<Module> module_;
        size_t next_member_ = 0;
        size_t total_members_ = 0;

        Frame(ExternalStorage& storage, Handle<Module> module)
            : module_(storage, storage.allocate(module))
            , total_members_(module->members().size()) {}
    };

    std::vector<Frame> stack;
    auto recurse = [&](Handle<Module> m) {
        if (m->initialized())
            return false;

        stack.emplace_back(ctx.externals(), m);
        return true;
    };

    if (!recurse(module))
        return;

    Scope sc(ctx);
    Local current_module = sc.local<Module>(defer_init);
    Local current_members = sc.local<Tuple>(defer_init);
    Local current_member = sc.local();
    Local current_init = sc.local();
    Local imported_name = sc.local<String>(defer_init);
    Local imported_module = sc.local<Module>(defer_init);
    while (!stack.empty()) {
        auto& frame = stack.back();
        TIRO_DEBUG_ASSERT(!frame.module_->initialized(), "Module must not be initialized.");

        current_module = frame.module_;

        // Iterate over all pending module members, resolving imports if necessary. Resolving an import
        // may make recursion necessary, in which case a frame is pushed and execution within the current
        // frame is paused.
        {
            size_t& i = frame.next_member_;
            size_t n = frame.total_members_;
            if (i < n) {
                current_members = current_module->members();
                while (i < n) {
                    current_member = current_members->get(i);
                    if (!current_member->is<UnresolvedImport>()) {
                        ++i;
                        continue;
                    }

                    // Search for the imported module and link it into the members tuple on success.
                    imported_name = current_member.must_cast<UnresolvedImport>()->module_name();
                    if (auto found = find_module(*imported_name)) {
                        imported_module = *found;
                    } else {
                        TIRO_ERROR("Module was not found.");
                    }
                    current_members->set(i, *imported_module);
                    ++i;

                    // Recurse if necessary. CAREFUL: if recurse() pushes a new frame,
                    // the `frame` is invalidated!
                    if (recurse(imported_module)) {
                        goto dispatch;
                    }
                }
            }
        }

        // All module members have been resolved.
        current_init = frame.module_->initializer();
        if (!current_init->is_null())
            ctx.run_init(current_init, {});
        frame.module_->initialized(true);
        stack.pop_back(); // frame invalidated

    dispatch:
        (void) 0;
    }
}

std::optional<Module> ModuleRegistry::find_module(String name) {
    if (auto found = modules_.value().get(name))
        return found->must_cast<Module>();
    return {};
}

Module load_module(Context& ctx, const BytecodeModule& compiled_module) {
    TIRO_CHECK(compiled_module.name().valid(), "Module definition without a valid module name.");
    TIRO_CHECK(compiled_module.member_count() <= ModuleLoader::max_module_size,
        "Module definition is too large.");

    ModuleLoader loader(ctx, compiled_module);
    return loader.run();
}

} // namespace tiro::vm
