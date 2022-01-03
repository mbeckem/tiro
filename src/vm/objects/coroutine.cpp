#include "vm/objects/coroutine.hpp"

#include "vm/context.hpp"
#include "vm/object_support/factory.hpp"
#include "vm/object_support/type_desc.hpp"
#include "vm/objects/result.hpp"

// #define TIRO_VM_DEBUG_COROUTINE_STATE

#ifdef TIRO_VM_DEBUG_COROUTINE_STATE
#    include <iostream>
#    define TIRO_VM_CORO_STATE(...) \
        (std::cout << "Coroutine state: " << fmt::format(__VA_ARGS__) << std::endl)
#else
#    define TIRO_VM_CORO_STATE(...)
#endif

namespace tiro::vm {

bool is_runnable(CoroutineState state) {
    return state == CoroutineState::Started || state == CoroutineState::Ready;
}

std::string_view to_string(CoroutineState state) {
    switch (state) {
    case CoroutineState::New:
        return "New";
    case CoroutineState::Started:
        return "Started";
    case CoroutineState::Ready:
        return "Ready";
    case CoroutineState::Running:
        return "Running";
    case CoroutineState::Waiting:
        return "Waiting";
    case CoroutineState::Done:
        return "Done";
    }

    TIRO_UNREACHABLE("invalid coroutine state");
}

Coroutine Coroutine::make(Context& ctx, Handle<String> name, Handle<Value> function,
    MaybeHandle<Tuple> arguments, Handle<CoroutineStack> stack) {
    Layout* data = create_object<Coroutine>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(FunctionSlot, function);
    data->write_static_slot(ArgumentsSlot, arguments.to_nullable());
    data->write_static_slot(StackSlot, stack);
    return Coroutine(from_heap(data));
}

String Coroutine::name() {
    return layout()->read_static_slot<String>(NameSlot);
}

Value Coroutine::function() {
    return layout()->read_static_slot(FunctionSlot);
}

Nullable<Tuple> Coroutine::arguments() {
    return layout()->read_static_slot<Nullable<Tuple>>(ArgumentsSlot);
}

Nullable<CoroutineStack> Coroutine::stack() {
    return layout()->read_static_slot<Nullable<CoroutineStack>>(StackSlot);
}

void Coroutine::stack(Nullable<CoroutineStack> stack) {
    layout()->write_static_slot(StackSlot, stack);
}

Nullable<Result> Coroutine::result() {
    return layout()->read_static_slot<Nullable<Result>>(ResultSlot);
}

void Coroutine::result(Nullable<Result> result) {
    layout()->write_static_slot(ResultSlot, result);
}

CoroutineState Coroutine::state() {
    return layout()->static_payload()->state;
}

void Coroutine::state(CoroutineState state) {
#ifdef TIRO_VM_DEBUG_COROUTINE_STATE
    {
        const auto old_state = access_heap()->state;
        if (state != old_state) {
            TIRO_VM_CORO_STATE("@{} changed from {} to {}.", (void*) heap_ptr(),
                to_string(old_state), to_string(state));
        }
    }
#endif

    layout()->static_payload()->state = state;
}

Nullable<NativeObject> Coroutine::native_callback() {
    return layout()->read_static_slot<Nullable<NativeObject>>(NativeCallbackSlot);
}

void Coroutine::native_callback(Nullable<NativeObject> callback) {
    layout()->write_static_slot(NativeCallbackSlot, callback);
}

Nullable<CoroutineToken> Coroutine::current_token() {
    return layout()->read_static_slot<Nullable<CoroutineToken>>(CurrentTokenSlot);
}

void Coroutine::reset_token() {
    layout()->write_static_slot<Nullable<CoroutineToken>>(CurrentTokenSlot, {});
}

Nullable<Coroutine> Coroutine::next_ready() {
    return layout()->read_static_slot<Nullable<Coroutine>>(NextReadySlot);
}

void Coroutine::next_ready(Nullable<Coroutine> next) {
    layout()->write_static_slot(NextReadySlot, next);
}

CoroutineToken Coroutine::create_token(Context& ctx, Handle<Coroutine> coroutine) {
    if (auto current = coroutine->current_token())
        return current.value();

    // Dangling, but following code does not allocate.
    CoroutineToken token = CoroutineToken::make(ctx, coroutine);
    coroutine->layout()->write_static_slot(CurrentTokenSlot, token);
    return token;
}

void Coroutine::schedule(Context& ctx, Handle<Coroutine> coroutine) {
    TIRO_DEBUG_ASSERT(coroutine->state() == CoroutineState::Running, "Coroutine must be running.");
    ctx.resume_coroutine(coroutine);
}

CoroutineToken CoroutineToken::make(Context& ctx, Handle<Coroutine> coroutine) {
    Layout* data = create_object<CoroutineToken>(ctx, StaticSlotsInit());
    data->write_static_slot(CoroutineSlot, coroutine);
    return CoroutineToken(from_heap(data));
}

Coroutine CoroutineToken::coroutine() {
    return layout()->read_static_slot<Coroutine>(CoroutineSlot);
}

bool CoroutineToken::valid() {
    return same(coroutine().current_token());
}

bool CoroutineToken::resume(Context& ctx, Handle<CoroutineToken> token) {
    if (!token->valid())
        return false;

    Scope sc(ctx);
    Local coroutine = sc.local(token->coroutine());
    if (coroutine->state() != CoroutineState::Waiting)
        return false;

    ctx.resume_coroutine(coroutine);
    return true;
}

static void coroutine_name_impl(NativeFunctionFrame& frame) {
    auto coroutine = check_instance<Coroutine>(frame);
    frame.return_value(coroutine->name());
}

static const FunctionDesc coroutine_methods[] = {
    FunctionDesc::method("name"sv, 1, NativeFunctionStorage::static_sync<coroutine_name_impl>()),
};

const TypeDesc coroutine_type_desc{"Coroutine"sv, coroutine_methods};

static void coroutine_token_coroutine_impl(NativeFunctionFrame& frame) {
    auto token = check_instance<CoroutineToken>(frame);
    frame.return_value(token->coroutine());
}

static void coroutine_token_valid_impl(NativeFunctionFrame& frame) {
    auto token = check_instance<CoroutineToken>(frame);
    frame.return_value(frame.ctx().get_boolean(token->valid()));
}

static void coroutine_token_resume_impl(NativeFunctionFrame& frame) {
    auto token = check_instance<CoroutineToken>(frame);
    bool success = CoroutineToken::resume(frame.ctx(), token);
    frame.return_value(frame.ctx().get_boolean(success));
}

static constexpr FunctionDesc coroutine_token_methods[] = {
    FunctionDesc::method(
        "coroutine"sv, 1, NativeFunctionStorage::static_sync<coroutine_token_coroutine_impl>()),
    FunctionDesc::method(
        "valid"sv, 1, NativeFunctionStorage::static_sync<coroutine_token_valid_impl>()),
    FunctionDesc::method(
        "resume"sv, 1, NativeFunctionStorage::static_sync<coroutine_token_resume_impl>()),
};

constexpr TypeDesc coroutine_token_type_desc{"CoroutineToken"sv, coroutine_token_methods};

} // namespace tiro::vm
