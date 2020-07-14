#include "vm/objects/function.hpp"

#include "vm/context.hpp"
#include "vm/objects/coroutine.hpp"
#include "vm/objects/factory.hpp"
#include "vm/objects/module.hpp"

#include <algorithm>

// TODO Assertions vs checks

namespace tiro::vm {

Code Code::make(Context& ctx, Span<const byte> code) {
    Layout* data = create_object<Code>(
        ctx, code.size(), BufferInit(code.size(), [&](Span<byte> bytes) {
            TIRO_DEBUG_ASSERT(bytes.size() == code.size(), "Unexpected allocation size.");
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

FunctionTemplate FunctionTemplate::make(Context& ctx, Handle<String> name, Handle<Module> module,
    u32 params, u32 locals, Span<const byte> code) {

    Scope sc(ctx);
    Local code_obj = sc.local(Code::make(ctx, code));

    Layout* data = create_object<FunctionTemplate>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(ModuleSlot, module);
    data->write_static_slot(CodeSlot, code_obj);
    data->static_payload()->params = params;
    data->static_payload()->locals = locals;
    return FunctionTemplate(from_heap(data));
}

String FunctionTemplate::name() {
    return layout()->read_static_slot<String>(NameSlot);
}

Module FunctionTemplate::module() {
    return layout()->read_static_slot<Module>(ModuleSlot);
}

Code FunctionTemplate::code() {
    return layout()->read_static_slot<Code>(CodeSlot);
}

u32 FunctionTemplate::params() {
    return layout()->static_payload()->params;
}

u32 FunctionTemplate::locals() {
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
    TIRO_CHECK(index < size(), "Environment::get(): index out of bounds.");
    return *layout()->fixed_slot(index);
}

void Environment::set(size_t index, Value value) {
    TIRO_CHECK(index < size(), "Environment::set(): index out of bounds.");
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

Function
Function::make(Context& ctx, Handle<FunctionTemplate> tmpl, MaybeHandle<Environment> closure) {
    Layout* data = create_object<Function>(ctx, StaticSlotsInit());
    data->write_static_slot(TmplSlot, tmpl);
    data->write_static_slot(ClosureSlot, closure.to_nullable());
    return Function(from_heap(data));
}

FunctionTemplate Function::tmpl() {
    return layout()->read_static_slot<FunctionTemplate>(TmplSlot);
}

Nullable<Environment> Function::closure() {
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

} // namespace tiro::vm
