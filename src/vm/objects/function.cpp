#include "vm/objects/function.hpp"

#include "vm/context.hpp"
#include "vm/object_support/factory.hpp"
#include "vm/objects/coroutine.hpp"
#include "vm/objects/module.hpp"
#include "vm/objects/native.hpp"

#include <algorithm>

// TODO Assertions vs checks

namespace tiro::vm {

Code Code::make(Context& ctx, Span<const byte> code) {
    Layout* data = create_object<Code>(
        ctx, code.size(), BufferInit(code.size(), [&](Span<byte> bytes) {
            TIRO_DEBUG_ASSERT(bytes.size() == code.size(), "Unexpected allocation size.");
            if (code.size() > 0)
                std::memcpy(bytes.data(), code.data(), code.size());
        }));
    return Code(from_heap(data));
}

const byte* Code::data() {
    return layout()->buffer_begin();
}

size_t Code::size() {
    return layout()->buffer_capacity();
}

HandlerTable HandlerTable::make(Context& ctx, Span<const Entry> entries) {
    Layout* data = create_object<HandlerTable>(
        ctx, entries.size(), BufferInit(entries.size(), [&](Span<Entry> dest_entries) {
            TIRO_DEBUG_ASSERT(entries.size() == dest_entries.size(), "Unexpected allocation size.");
            std::uninitialized_copy(entries.begin(), entries.end(), dest_entries.begin());
        }));
    return HandlerTable(from_heap(data));
}

const HandlerTable::Entry* HandlerTable::data() {
    return layout()->buffer_begin();
}

size_t HandlerTable::size() {
    return layout()->buffer_capacity();
}

const HandlerTable::Entry* HandlerTable::find_entry(u32 pc) {
    auto entries = view();
    auto pos = std::upper_bound(entries.begin(), entries.end(), pc,
        [&](u32 lhs, const Entry& rhs) { return lhs < rhs.to; });
    if (pos == entries.end())
        return nullptr;

    TIRO_DEBUG_ASSERT(pos->to > pc, "Interval end must be to the right of pc.");
    return pos->from <= pc ? pos : nullptr;
}

CodeFunctionTemplate
CodeFunctionTemplate::make(Context& ctx, Handle<String> name, Handle<Module> module, u32 params,
    u32 locals, Span<const HandlerTable::Entry> handlers, Span<const byte> code) {

    Scope sc(ctx);
    Local code_obj = sc.local(Code::make(ctx, code));
    Local handlers_obj = sc.local();
    if (!handlers.empty())
        handlers_obj = HandlerTable::make(ctx, handlers);

    Layout* data = create_object<CodeFunctionTemplate>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(ModuleSlot, module);
    data->write_static_slot(CodeSlot, code_obj);
    data->write_static_slot(HandlersSlot, handlers_obj);
    data->static_payload()->params = params;
    data->static_payload()->locals = locals;
    return CodeFunctionTemplate(from_heap(data));
}

String CodeFunctionTemplate::name() {
    return layout()->read_static_slot<String>(NameSlot);
}

Module CodeFunctionTemplate::module() {
    return layout()->read_static_slot<Module>(ModuleSlot);
}

Code CodeFunctionTemplate::code() {
    return layout()->read_static_slot<Code>(CodeSlot);
}

Nullable<HandlerTable> CodeFunctionTemplate::handlers() {
    return layout()->read_static_slot<Nullable<HandlerTable>>(HandlersSlot);
}

u32 CodeFunctionTemplate::params() {
    return layout()->static_payload()->params;
}

u32 CodeFunctionTemplate::locals() {
    return layout()->static_payload()->locals;
}

Environment Environment::make(Context& ctx, size_t size, MaybeHandle<Environment> parent) {
    TIRO_DEBUG_ASSERT(size > 0, "0 sized closure context is useless.");

    Layout* data = create_object<Environment>(ctx, size,
        FixedSlotsInit(size,
            [&](Span<Value> values) {
                auto undef = ctx.get_undefined();
                std::uninitialized_fill_n(values.data(), values.size(), undef);
            }),
        StaticSlotsInit());
    data->write_static_slot(ParentSlot, parent.to_nullable());
    return Environment(from_heap(data));
}

Nullable<Environment> Environment::parent() {
    return layout()->read_static_slot<Nullable<Environment>>(ParentSlot);
}

Value* Environment::data() {
    return layout()->fixed_slots_begin();
}

size_t Environment::size() {
    return layout()->fixed_slot_capacity();
}

Value Environment::get(size_t index) {
    TIRO_DEBUG_ASSERT(index < size(), "Environment::get(): index out of bounds.");
    return *layout()->fixed_slot(index);
}

void Environment::set(size_t index, Value value) {
    TIRO_DEBUG_ASSERT(index < size(), "Environment::set(): index out of bounds.");
    *layout()->fixed_slot(index) = value;
}

Nullable<Environment> Environment::parent(size_t level) {
    Nullable<Environment> current = *this;
    TIRO_DEBUG_ASSERT(!current.is_null(), "The current closure context cannot be null.");

    while (level != 0) {
        current = current.value().parent();

        if (TIRO_UNLIKELY(current.is_null()))
            break;

        --level;
    }
    return current;
}

CodeFunction CodeFunction::make(
    Context& ctx, Handle<CodeFunctionTemplate> tmpl, MaybeHandle<Environment> closure) {
    Layout* data = create_object<CodeFunction>(ctx, StaticSlotsInit());
    data->write_static_slot(TmplSlot, tmpl);
    data->write_static_slot(ClosureSlot, closure.to_nullable());
    return CodeFunction(from_heap(data));
}

CodeFunctionTemplate CodeFunction::tmpl() {
    return layout()->read_static_slot<CodeFunctionTemplate>(TmplSlot);
}

Nullable<Environment> CodeFunction::closure() {
    return layout()->read_static_slot<Nullable<Environment>>(ClosureSlot);
}

BoundMethod BoundMethod::make(Context& ctx, Handle<Value> function, Handle<Value> object) {
    Layout* data = create_object<BoundMethod>(ctx, StaticSlotsInit());
    data->write_static_slot(FunctionSlot, function);
    data->write_static_slot(ObjectSlot, object);
    return BoundMethod(from_heap(data));
}

Value BoundMethod::function() {
    return layout()->read_static_slot<Value>(FunctionSlot);
}

Value BoundMethod::object() {
    return layout()->read_static_slot<Value>(ObjectSlot);
}

MagicFunction MagicFunction::make(Context& ctx, Which which) {
    Layout* data = create_object<MagicFunction>(ctx, StaticPayloadInit());
    data->static_payload()->which = which;
    return MagicFunction(from_heap(data));
}

MagicFunction::Which MagicFunction::which() {
    return layout()->static_payload()->which;
}

std::string_view to_string(MagicFunction::Which which) {
    switch (which) {
    case MagicFunction::Catch:
        return "Catch";
    }

    TIRO_UNREACHABLE("Invalid magic function type");
}

Function::Function(BoundMethod func)
    : Function(static_cast<Value>(func)) {}

Function::Function(CodeFunction func)
    : Function(static_cast<Value>(func)) {}

Function::Function(MagicFunction func)
    : Function(static_cast<Value>(func)) {}

Function::Function(NativeFunction func)
    : Function(static_cast<Value>(func)) {}

} // namespace tiro::vm
