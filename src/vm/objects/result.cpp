#include "vm/objects/result.hpp"

#include "vm/math.hpp"
#include "vm/objects/factory.hpp"

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
    auto n = extract_integer(get_which());
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

Value Result::value() {
    TIRO_CHECK(is_success(), "Result::value(): cannot access value on error result.");
    return get_value();
}

Value Result::error() {
    TIRO_CHECK(is_error(), "Result::error(): cannot access error on successful result.");
    return get_value();
}

Value Result::get_value() {
    return layout()->read_static_slot<Value>(ValueSlot);
}

Value Result::get_which() {
    return layout()->read_static_slot<Value>(WhichSlot);
}

// TODO: Static methods as factories
static constexpr MethodDesc result_methods[] = {
    {
        "type"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto result = check_instance<Result>(frame);
            switch (result->which()) {
            case Result::Success:
                frame.result(frame.ctx().get_symbol("success"));
                break;
            case Result::Error:
                frame.result(frame.ctx().get_symbol("error"));
                break;
            }
        },
    },
    {
        "is_success"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto result = check_instance<Result>(frame);
            frame.result(frame.ctx().get_boolean(result->is_success()));
        },
    },
    {
        "is_error"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto result = check_instance<Result>(frame);
            frame.result(frame.ctx().get_boolean(result->is_error()));
        },
    },
    {
        "value"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto result = check_instance<Result>(frame);
            frame.result(result->value());
        },
    },
    {
        "error"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto result = check_instance<Result>(frame);
            frame.result(result->error());
        },
    },
};

constexpr TypeDesc result_type_desc{"Result"sv, result_methods};

} // namespace tiro::vm
