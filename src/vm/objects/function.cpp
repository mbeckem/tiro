#include "vm/objects/function.hpp"

#include "vm/context.hpp"
#include "vm/objects/coroutine.hpp"
#include "vm/objects/module.hpp"

#include "vm/context.ipp"

// TODO Assertions vs checks

namespace tiro::vm {

Code Code::make(Context& ctx, Span<const byte> code) {
    size_t allocation_size = LayoutTraits<Layout>::dynamic_size(code.size());
    Layout* data = ctx.heap().create_varsize<Layout>(
        allocation_size, ValueType::Code, BufferInit(code.size(), [&](Span<byte> bytes) {
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
    Root<Code> code_obj(ctx, Code::make(ctx, code));

    Layout* data = ctx.heap().create<Layout>(
        ValueType::FunctionTemplate, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(ModuleSlot, module);
    data->write_static_slot(CodeSlot, code_obj.get());
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

Environment Environment::make(Context& ctx, size_t size, Handle<Environment> parent) {
    TIRO_DEBUG_ASSERT(size > 0, "0 sized closure context is useless.");

    size_t allocation_size = LayoutTraits<Layout>::dynamic_size(size);
    Layout* data = ctx.heap().create_varsize<Layout>(allocation_size, ValueType::Environment,
        FixedSlotsInit(size,
            [&](Span<Value> values) {
                auto undef = ctx.get_undefined();
                std::uninitialized_fill_n(values.data(), values.size(), undef);
            }),
        StaticSlotsInit());
    data->write_static_slot(ParentSlot, parent);
    return Environment(from_heap(data));
}

Environment Environment::parent() {
    return layout()->read_static_slot<Environment>(ParentSlot);
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

Environment Environment::parent(size_t level) {
    Environment ctx = *this;
    TIRO_DEBUG_ASSERT(!ctx.is_null(), "The current closure context cannot be null.");

    while (level != 0) {
        ctx = ctx.parent();

        if (TIRO_UNLIKELY(ctx.is_null()))
            break;
        --level;
    }
    return ctx;
}

Function Function::make(Context& ctx, Handle<FunctionTemplate> tmpl, Handle<Environment> closure) {
    Layout* data = ctx.heap().create<Layout>(ValueType::Function, StaticSlotsInit());
    data->write_static_slot(TmplSlot, tmpl);
    data->write_static_slot(ClosureSlot, closure);
    return Function(from_heap(data));
}

FunctionTemplate Function::tmpl() {
    return layout()->read_static_slot<FunctionTemplate>(TmplSlot);
}

Environment Function::closure() {
    return layout()->read_static_slot<Environment>(ClosureSlot);
}

BoundMethod BoundMethod::make(Context& ctx, Handle<Value> function, Handle<Value> object) {
    TIRO_DEBUG_ASSERT(function.get(), "BoundMethod::make(): Invalid function.");
    TIRO_DEBUG_ASSERT(object.get(), "BoundMethod::make(): Invalid object.");

    Layout* data = ctx.heap().create<Layout>(ValueType::BoundMethod, StaticSlotsInit());
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
