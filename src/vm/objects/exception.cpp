#include "vm/objects/exception.hpp"

#include "vm/error_utils.hpp"
#include "vm/handles/scope.hpp"
#include "vm/object_support/factory.hpp"
#include "vm/object_support/type_desc.hpp"
#include "vm/objects/array.hpp"
#include "vm/objects/string.hpp"

namespace tiro::vm {

Exception Exception::make(Context& ctx, Handle<String> message) {
    return make(ctx, message, 0);
}

Exception Exception::make(Context& ctx, Handle<String> message, u32 skip_frames) {
    Scope sc(ctx);
    if (!ctx.settings().enable_panic_stack_traces)
        return make_impl(ctx, message, {});

    Local coroutine = sc.local(ctx.interpreter().current_coroutine());
    if (coroutine->is_null())
        return make_impl(ctx, message, {});

    // collect coroutine stack representation
    Local stack = sc.local(coroutine->value().stack());
    if (stack->is_null())
        return make_impl(ctx, message, {});

    // coroutine name
    Local builder = sc.local(StringBuilder::make(ctx));
    {
        Local name = sc.local(coroutine->value().name());
        builder->append(ctx, name);
        builder->append(ctx, ":");
    }

    // function names
    bool has_entries = false;
    CoroutineStack::walk(ctx, stack.must_cast<CoroutineStack>(), [&](Handle<String> function_name) {
        if (skip_frames > 0) {
            skip_frames -= 1;
            return;
        }

        builder->append(ctx, "\n");
        builder->append(ctx, "  - ");
        builder->append(ctx, function_name);
        has_entries = true;
    });
    if (!has_entries)
        builder->append(ctx, "\n  <empty call stack>");

    Local trace = sc.local(builder->to_string(ctx));
    return make_impl(ctx, message, trace);
}

String Exception::message() {
    return layout()->read_static_slot<String>(MessageSlot);
}

Nullable<String> Exception::trace() {
    return layout()->read_static_slot<Nullable<String>>(TraceSlot);
}

Nullable<Array> Exception::secondary() {
    return layout()->read_static_slot<Nullable<Array>>(SecondarySlot);
}

Exception Exception::make_impl(Context& ctx, Handle<String> message, MaybeHandle<String> trace) {
    Layout* data = create_object<Exception>(ctx, StaticSlotsInit());
    data->write_static_slot(MessageSlot, message);
    data->write_static_slot(TraceSlot, trace.to_nullable());
    return Exception(from_heap(data));
}

void Exception::secondary(Nullable<Array> secondary) {
    layout()->write_static_slot(SecondarySlot, secondary);
}

void Exception::add_secondary(Context& ctx, Handle<Exception> sec) {
    Scope sc(ctx);

    Local maybe_array = sc.local(secondary());
    if (maybe_array->is_null()) {
        maybe_array = Array::make(ctx);
        secondary(*maybe_array);
    }

    auto array = maybe_array.must_cast<Array>();
    // can only fail in theory for ridiculous amounts of nested exceptions
    array->append(ctx, sec).must("failed to add secondary exception");
}

Exception vformat_exception_impl(Context& ctx, std::string_view format, fmt::format_args args) {
    Scope sc(ctx);
    Local message = sc.local(String::vformat(ctx, format, args));
    return Exception::make(ctx, message);
}

static void exception_message_impl(SyncFrameContext& frame) {
    auto ex = check_instance<Exception>(frame);
    frame.return_value(ex->message());
}

static void exception_trace_impl(SyncFrameContext& frame) {
    auto ex = check_instance<Exception>(frame);
    frame.return_value(ex->trace());
}

static constexpr FunctionDesc exception_methods[] = {
    FunctionDesc::method("message"sv, 1, exception_message_impl),
    FunctionDesc::method("trace"sv, 1, exception_trace_impl),
};

constexpr TypeDesc exception_type_desc{"Exception"sv, exception_methods};

} // namespace tiro::vm
