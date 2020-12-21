#include "vm/objects/exception.hpp"

#include "vm/object_support/factory.hpp"

namespace tiro::vm {

Exception Exception::make(Context& ctx, Handle<String> message) {
    Layout* data = create_object<Exception>(ctx, StaticSlotsInit());
    data->write_static_slot(MessageSlot, message);
    return Exception(from_heap(data));
}

String Exception::message() {
    return layout()->read_static_slot<String>(MessageSlot);
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

} // namespace tiro::vm
