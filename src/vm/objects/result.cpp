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

Result Result::make_failure(Context& ctx, Handle<Value> error) {
    Scope sc(ctx);
    Local which = sc.local(ctx.get_integer(Failure));

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
    case Failure:
        return Failure;
    default:
        TIRO_UNREACHABLE("Invalid value for 'which'.");
    }
}

bool Result::is_success() {
    return which() == Success;
}

bool Result::is_failure() {
    return which() == Failure;
}

Value Result::value() {
    TIRO_CHECK(is_success(), "Result::value(): cannot access value on failure result.");
    return get_value();
}

Value Result::reason() {
    TIRO_CHECK(is_failure(), "Result::reason(): cannot access reason on successful result.");
    return get_value();
}

Value Result::get_value() {
    return layout()->read_static_slot<Value>(ValueSlot);
}

Value Result::get_which() {
    return layout()->read_static_slot<Value>(WhichSlot);
}

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
            case Result::Failure:
                frame.result(frame.ctx().get_symbol("failure"));
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
        "is_failure"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto result = check_instance<Result>(frame);
            frame.result(frame.ctx().get_boolean(result->is_failure()));
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
        "reason"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto result = check_instance<Result>(frame);
            frame.result(result->reason());
        },
    },
};

constexpr TypeDesc result_type_desc{"Result"sv, result_methods};

} // namespace tiro::vm
