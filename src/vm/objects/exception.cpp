#include "vm/objects/exception.hpp"

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

    Local array = sc.local(secondary());
    if (array->is_null()) {
        array = Array::make(ctx);
        secondary(*array);
    }

    array.must_cast<Array>()->append(ctx, sec);
}

Exception vformat_exception_impl(
    Context& ctx, const SourceLocation& loc, std::string_view format, fmt::format_args args) {
    Scope sc(ctx);

    Local builder = sc.local(StringBuilder::make(ctx));
    builder->vformat(ctx, format, args);
    if (loc) {
        builder->format(ctx, "\nIn: {} ({}:{})", loc.function, loc.file, loc.line);
    }

    Local message = sc.local(builder->to_string(ctx));
    return Exception::make(ctx, message);
}

static const MethodDesc exception_methods[] = {
    {
        "message"sv,
        1,
        NativeFunctionArg::sync([](NativeFunctionFrame& frame) {
            auto ex = check_instance<Exception>(frame);
            frame.return_value(ex->message());
        }),
    },
};

const TypeDesc exception_type_desc{"Exception"sv, exception_methods};

} // namespace tiro::vm
