#include "vm/objects/exception.hpp"

#include "vm/error_utils.hpp"
#include "vm/handles/scope.hpp"
#include "vm/object_support/factory.hpp"
#include "vm/objects/array.hpp"

namespace tiro::vm {

Exception Exception::make(Context& ctx, Handle<String> message) {
    Layout* data = create_object<Exception>(ctx, StaticSlotsInit());
    data->write_static_slot(MessageSlot, message);
    return Exception(from_heap(data));
}

String Exception::message() {
    return layout()->read_static_slot<String>(MessageSlot);
}

Nullable<Array> Exception::secondary() {
    return layout()->read_static_slot<Nullable<Array>>(SecondarySlot);
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

static void exception_message_impl(NativeFunctionFrame& frame) {
    auto ex = check_instance<Exception>(frame);
    frame.return_value(ex->message());
}

static constexpr FunctionDesc exception_methods[] = {
    FunctionDesc::method(
        "message"sv, 1, NativeFunctionStorage::static_sync<exception_message_impl>()),
};

constexpr TypeDesc exception_type_desc{"Exception"sv, exception_methods};

} // namespace tiro::vm
