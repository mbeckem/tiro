#include "vm/types.hpp"

#include "vm/context.hpp"
#include "vm/math.hpp"
#include "vm/objects/array.hpp"
#include "vm/objects/buffer.hpp"
#include "vm/objects/function.hpp"
#include "vm/objects/module.hpp"
#include "vm/objects/string.hpp"

namespace tiro::vm {

namespace {

template<typename T>
Handle<T> check_instance(NativeFunction::Frame& frame) {
    Handle<Value> value = frame.arg(0);
    if (!value->is<T>()) {
        TIRO_ERROR("`this` is not a {}.", to_string(TypeToTag<T>));
    }
    return value.cast<T>();
}

class TypeBuilder {
public:
    explicit TypeBuilder(Context& ctx)
        : ctx_(ctx)
        , name_(ctx)
        , table_(ctx, HashTable::make(ctx)) {}

    TypeBuilder& name(std::string_view name) {
        name_.set(ctx_.get_interned_string(name));
        return *this;
    }

    TypeBuilder& add(std::string_view name, u32 argc, NativeFunction::FunctionType native_func) {
        Root<Symbol> member(ctx_, ctx_.get_symbol(name));
        Root<String> member_str(ctx_, member->name());
        Root<NativeFunction> func(
            ctx_, NativeFunction::make(ctx_, member_str, {}, argc, native_func));
        Root<Method> method(ctx_, Method::make(ctx_, func.handle()));
        table_->set(ctx_, member.handle(), method.handle());
        return *this;
    }

    Type build() {
        if (!name_.get()) {
            name_.set(ctx_.get_interned_string("<anonymous type>"));
        }
        return Type::make(ctx_, name_, table_);
    }

private:
    Context& ctx_;
    Root<String> name_;
    Root<HashTable> table_;
};

} // namespace

Type simple_type(Context& ctx, std::string_view name) {
    TypeBuilder builder(ctx);
    return builder.name(name).build();
}

static Type type_type(Context& ctx) {
    constexpr auto name = [](NativeFunction::Frame& frame) {
        auto self = check_instance<Type>(frame);
        frame.result(self->name());
    };

    TypeBuilder builder(ctx);
    return builder //
        .name("Type")
        .add("name", 1, name)
        .build();
}

static Type hash_table_type(Context& ctx) {
    constexpr auto set = [](NativeFunction::Frame& frame) {
        auto self = check_instance<HashTable>(frame);
        self->set(frame.ctx(), frame.arg(1), frame.arg(2));
    };

    constexpr auto contains = [](NativeFunction::Frame& frame) {
        auto self = check_instance<HashTable>(frame);
        bool result = self->contains(frame.arg(1));
        frame.result(frame.ctx().get_boolean(result));
    };

    constexpr auto remove = [](NativeFunction::Frame& frame) {
        auto self = check_instance<HashTable>(frame);
        self->remove(frame.arg(1));
    };

    TypeBuilder builder(ctx);
    return builder //
        .name("Map")
        .add("set", 3, set)
        .add("contains", 2, contains)
        .add("remove", 2, remove)
        .build();
}

static Type string_builder_type(Context& ctx) {
    const auto append = [](NativeFunction::Frame& frame) {
        auto self = check_instance<StringBuilder>(frame);
        for (size_t i = 1; i < frame.arg_count(); ++i) {
            Handle<Value> arg = frame.arg(i);
            if (arg->is<String>()) {
                self->append(frame.ctx(), arg.cast<String>());
            } else if (arg->is<StringBuilder>()) {
                self->append(frame.ctx(), arg.cast<StringBuilder>());
            } else {
                TIRO_ERROR("Cannot append values of type {}.", to_string(arg->type()));
            }
        }
    };

    const auto append_byte = [](NativeFunction::Frame& frame) {
        auto self = check_instance<StringBuilder>(frame);
        Handle arg = frame.arg(1);

        byte b;
        if (auto i = try_extract_integer(arg); i && *i >= 0 && *i <= 0xff) {
            b = *i;
        } else {
            TIRO_ERROR("Expected a byte argument (between 0 and 255).");
        }

        self->append(frame.ctx(), std::string_view((char*) &b, 1));
    };

    const auto clear = [](NativeFunction::Frame& frame) {
        auto self = check_instance<StringBuilder>(frame);
        self->clear();
    };

    const auto to_str = [](NativeFunction::Frame& frame) {
        auto self = check_instance<StringBuilder>(frame);
        frame.result(self->make_string(frame.ctx()));
    };

    TypeBuilder builder(ctx);
    return builder //
        .name("StringBuilder")
        .add("append", 2, append)
        .add("append_byte", 2, append_byte)
        .add("clear", 1, clear)
        .add("to_str", 1, to_str)
        .build();
}

static Type buffer_type(Context& ctx) {
    const auto size = [](NativeFunction::Frame& frame) {
        auto self = check_instance<Buffer>(frame);
        frame.result(frame.ctx().get_integer(static_cast<i64>(self->size())));
    };

    TypeBuilder builder(ctx);
    return builder //
        .name("Buffer")
        .add("size", 1, size)
        .build();
}

static Type array_type(Context& ctx) {
    const auto size = [](NativeFunction::Frame& frame) {
        auto self = check_instance<Array>(frame);
        frame.result(frame.ctx().get_integer(static_cast<i64>(self->size())));
    };

    const auto append = [](NativeFunction::Frame& frame) {
        auto self = check_instance<Array>(frame);
        auto value = frame.arg(1);
        self->append(frame.ctx(), value);
    };

    TypeBuilder builder(ctx);
    return builder //
        .name("Array")
        .add("append", 2, append)
        .add("size", 1, size)
        .build();
}

static Type tuple_type(Context& ctx) {
    const auto size = [](NativeFunction::Frame& frame) {
        auto self = check_instance<Tuple>(frame);
        frame.result(frame.ctx().get_integer(static_cast<i64>(self->size())));
    };

    TypeBuilder builder(ctx);
    return builder //
        .name("Tuple")
        .add("size", 1, size)
        .build();
}

void TypeSystem::init(Context& ctx) {
    Root integer_type(ctx, simple_type(ctx, "Integer"));
    Root function_type(ctx, simple_type(ctx, "Function"));

#define TIRO_TYPE(value_type, expr) (types_.emplace(ValueType::value_type, (expr)))

    TIRO_TYPE(Array, array_type(ctx));
    TIRO_TYPE(Boolean, simple_type(ctx, "Boolean"));
    TIRO_TYPE(BoundMethod, function_type.get());
    TIRO_TYPE(Buffer, buffer_type(ctx));
    TIRO_TYPE(Type, type_type(ctx));
    TIRO_TYPE(Coroutine, simple_type(ctx, "Coroutine"));
    TIRO_TYPE(DynamicObject, simple_type(ctx, "DynamicObject"));
    TIRO_TYPE(Float, simple_type(ctx, "Float"));
    TIRO_TYPE(Function, function_type.get());
    TIRO_TYPE(HashTable, hash_table_type(ctx));
    TIRO_TYPE(Integer, integer_type.get());
    TIRO_TYPE(Module, simple_type(ctx, "Module"));
    TIRO_TYPE(NativeAsyncFunction, function_type.get());
    TIRO_TYPE(NativeFunction, function_type.get());
    TIRO_TYPE(NativeObject, simple_type(ctx, "NativeObject"));
    TIRO_TYPE(NativePointer, simple_type(ctx, "NativePointer"));
    TIRO_TYPE(Null, simple_type(ctx, "Null"));
    TIRO_TYPE(SmallInteger, integer_type.get());
    TIRO_TYPE(String, simple_type(ctx, "String"));
    TIRO_TYPE(StringBuilder, string_builder_type(ctx));
    TIRO_TYPE(Symbol, simple_type(ctx, "Symbol"));
    TIRO_TYPE(Tuple, tuple_type(ctx));

#undef TIRO_CASE
}

Value TypeSystem::type_of(Handle<Value> object) {
    if (auto pos = types_.find(object->type()); pos != types_.end())
        return pos->second;

    TIRO_ERROR("Unsupported object type {} in type_of query (type is internal).",
        to_string(object->type()));
}

Value TypeSystem::load_index(Context& ctx, Handle<Value> object, Handle<Value> index) {
    switch (object->type()) {
    case ValueType::Array: {
        Handle<Array> array = object.cast<Array>();

        i64 raw_index;
        if (auto opt = try_extract_integer(index)) {
            raw_index = *opt;
        } else {
            TIRO_ERROR("Array index must be an integer.");
        }

        TIRO_CHECK(raw_index >= 0 && u64(raw_index) < array->size(),
            "Invalid index {} into array of size {}.", raw_index, array->size());
        return array->get(size_t(raw_index));
    }
    case ValueType::Tuple: {
        Handle<Tuple> tuple = object.cast<Tuple>();

        i64 raw_index;
        if (auto opt = try_extract_integer(index)) {
            raw_index = *opt;
        } else {
            TIRO_ERROR("Tuple index must be an integer.");
        }

        TIRO_CHECK(raw_index >= 0 && u64(raw_index) < tuple->size(),
            "Invalid index {} into tuple of size {}.", raw_index, tuple->size());
        return tuple->get(size_t(raw_index));
    }
    case ValueType::Buffer: {
        Handle<Buffer> buffer = object.cast<Buffer>();

        i64 raw_index;
        if (auto opt = try_extract_integer(index)) {
            raw_index = *opt;
        } else {
            TIRO_ERROR("Buffer index must be an integer.");
        }

        TIRO_CHECK(raw_index >= 0 && u64(raw_index) < buffer->size(),
            "Invalid index {} into buffer of size {}.", raw_index, buffer->size());

        return ctx.get_integer(buffer->get(size_t(raw_index)));
    }
    case ValueType::HashTable: {
        Handle<HashTable> table = object.cast<HashTable>();
        if (auto found = table->get(index.get())) {
            return *found;
        }
        return Value::null();
    }
    default:
        TIRO_ERROR(
            "Loading an index is not supported for objects of type {}.", to_string(object->type()));
    }
}

void TypeSystem::store_index(
    Context& ctx, Handle<Value> object, Handle<Value> index, Handle<Value> value) {
    switch (object->type()) {
    case ValueType::Array: {
        Handle<Array> array = object.cast<Array>();

        i64 raw_index;
        if (auto opt = try_extract_integer(index)) {
            raw_index = *opt;
        } else {
            TIRO_ERROR("Array index must be an integer.");
        }

        TIRO_CHECK(raw_index >= 0 && u64(raw_index) < array->size(),
            "Invalid index {} into array of size {}.", raw_index, array->size());
        array->set(static_cast<size_t>(raw_index), value);
        break;
    }
    case ValueType::Tuple: {
        Handle<Tuple> tuple = object.cast<Tuple>();

        i64 raw_index;
        if (auto opt = try_extract_integer(index)) {
            raw_index = *opt;
        } else {
            TIRO_ERROR("Tuple index must be an integer.");
        }

        TIRO_CHECK(raw_index >= 0 && u64(raw_index) < tuple->size(),
            "Invalid index {} into tuple of size {}.", raw_index, tuple->size());
        tuple->set(static_cast<size_t>(raw_index), value.get());
        break;
    }
    case ValueType::Buffer: {
        Handle<Buffer> buffer = object.cast<Buffer>();

        i64 raw_index;
        if (auto opt = try_extract_integer(index)) {
            raw_index = *opt;
        } else {
            TIRO_ERROR("Buffer index must be an integer.");
        }

        byte raw_value;
        if (auto val = try_extract_integer(value); val && *val >= 0 && *val <= 255) {
            raw_value = *val;
        } else {
            TIRO_ERROR("Buffer value must a valid byte (integers 0 through 255).");
        }

        TIRO_CHECK(raw_index >= 0 && u64(raw_index) < buffer->size(),
            "Invalid index {} into buffer of size {}.", raw_index, buffer->size());

        buffer->set(size_t(raw_index), raw_value);
        break;
    }
    case ValueType::HashTable: {
        Handle<HashTable> table = object.cast<HashTable>();
        table->set(ctx, index, value);
        break;
    }
    default:
        TIRO_ERROR(
            "Storing an index is not supported for objects of type {}.", to_string(object->type()));
    }
}

std::optional<Value> TypeSystem::load_member(
    [[maybe_unused]] Context& ctx, Handle<Value> object, Handle<Symbol> member) {
    switch (object->type()) {
    case ValueType::Module: {
        auto module = object.cast<Module>();
        // TODO Exported should be name -> index only instead of returning the values directly.
        // Encapsulate that in the module type.
        return module->exported().get(member.get());
    }
    case ValueType::DynamicObject: {
        auto dyn = object.cast<DynamicObject>();
        return dyn->get(member);
    }
    default:
        TIRO_ERROR("load_member not implemented for this type yet: {}.", to_string(object->type()));
    }
}

bool TypeSystem::store_member(
    Context& ctx, Handle<Value> object, Handle<Symbol> member, Handle<Value> value) {
    switch (object->type()) {
    case ValueType::Module:
        return false;
    case ValueType::DynamicObject: {
        auto dyn = object.cast<DynamicObject>();
        dyn->set(ctx, member, value);
        return true;
    }
    default:
        TIRO_ERROR(
            "store_member not implemented for this type yet: {}.", to_string(object->type()));
    }
}

std::optional<Value>
TypeSystem::load_method(Context& ctx, Handle<Value> object, Handle<Symbol> member) {
    // TODO: Implement fields.
    switch (object->type()) {
    case ValueType::Module:
    case ValueType::DynamicObject:
        return load_member(ctx, object, member);

    default: {
        auto pos = types_.find(object->type());
        if (pos == types_.end())
            return {};

        auto type = pos->second;
        return type.find_method(member);
    }
    }
}

} // namespace tiro::vm
