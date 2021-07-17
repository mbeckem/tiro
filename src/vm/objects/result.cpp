#include "vm/objects/result.hpp"

#include "vm/math.hpp"
#include "vm/object_support/factory.hpp"
#include "vm/object_support/type_desc.hpp"
#include "vm/objects/exception.hpp"
#include "vm/objects/primitives.hpp"

namespace tiro::vm {

Result Result::make_success(Context& ctx, Handle<Value> value) {
    Scope sc(ctx);
    Local which = sc.local(ctx.get_integer(Success));

    Layout* data = create_object<Result>(ctx, StaticSlotsInit());
    data->write_static_slot(WhichSlot, which);
    data->write_static_slot(ValueSlot, value);
    return Result(from_heap(data));
}

Result Result::make_error(Context& ctx, Handle<Value> error) {
    Scope sc(ctx);
    Local which = sc.local(ctx.get_integer(Error));

    Layout* data = create_object<Result>(ctx, StaticSlotsInit());
    data->write_static_slot(WhichSlot, which);
    data->write_static_slot(ValueSlot, error);
    return Result(from_heap(data));
}

Result::Which Result::which() {
    auto n = get_which().value();
    switch (n) {
    case Success:
        return Success;
    case Error:
        return Error;
    default:
        TIRO_UNREACHABLE("Invalid value for 'which'.");
    }
}

bool Result::is_success() {
    return which() == Success;
}

bool Result::is_error() {
    return which() == Error;
}

Value Result::unchecked_value() {
    TIRO_DEBUG_ASSERT(is_success(), "result does not store a value");
    return get_value();
}

Value Result::unchecked_error() {
    TIRO_DEBUG_ASSERT(is_error(), "result does not store an error");
    return get_value();
}

Value Result::get_value() {
    return layout()->read_static_slot<Value>(ValueSlot);
}

Integer Result::get_which() {
    return layout()->read_static_slot<Integer>(WhichSlot);
}

static void result_type_impl(NativeFunctionFrame& frame) {
    auto result = check_instance<Result>(frame);
    switch (result->which()) {
    case Result::Success:
        frame.return_value(frame.ctx().get_symbol("success"));
        break;
    case Result::Error:
        frame.return_value(frame.ctx().get_symbol("error"));
        break;
    }
}

static void result_is_success_impl(NativeFunctionFrame& frame) {
    auto result = check_instance<Result>(frame);
    frame.return_value(frame.ctx().get_boolean(result->is_success()));
}

static void result_is_error_impl(NativeFunctionFrame& frame) {
    auto result = check_instance<Result>(frame);
    frame.return_value(frame.ctx().get_boolean(result->is_error()));
}

static void result_value_impl(NativeFunctionFrame& frame) {
    auto result = check_instance<Result>(frame);
    if (!result->is_success()) {
        return frame.panic(
            TIRO_FORMAT_EXCEPTION(frame.ctx(), "cannot access value on failure result"));
    }
    frame.return_value(result->unchecked_value());
}

static void result_error_impl(NativeFunctionFrame& frame) {
    auto result = check_instance<Result>(frame);
    if (!result->is_error()) {
        return frame.panic(
            TIRO_FORMAT_EXCEPTION(frame.ctx(), "cannot access reason on successful result"));
    }
    frame.return_value(result->unchecked_error());
}

static constexpr FunctionDesc result_methods[] = {
    FunctionDesc::method("type"sv, 1, NativeFunctionStorage::static_sync<result_type_impl>()),
    FunctionDesc::method(
        "is_success"sv, 1, NativeFunctionStorage::static_sync<result_is_success_impl>()),
    FunctionDesc::method(
        "is_error"sv, 1, NativeFunctionStorage::static_sync<result_is_error_impl>()),
    FunctionDesc::method("value"sv, 1, NativeFunctionStorage::static_sync<result_value_impl>()),
    FunctionDesc::method("error"sv, 1, NativeFunctionStorage::static_sync<result_error_impl>()),
};

constexpr TypeDesc result_type_desc{"Result"sv, result_methods};

} // namespace tiro::vm
