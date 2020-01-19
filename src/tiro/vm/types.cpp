#include "tiro/vm/types.hpp"

#include "tiro/vm/context.hpp"
#include "tiro/vm/math.hpp"
#include "tiro/vm/objects/arrays.hpp"
#include "tiro/vm/objects/buffers.hpp"
#include "tiro/vm/objects/classes.hpp"
#include "tiro/vm/objects/functions.hpp"
#include "tiro/vm/objects/hash_tables.hpp"
#include "tiro/vm/objects/modules.hpp"
#include "tiro/vm/objects/strings.hpp"

namespace tiro::vm {

namespace {

template<typename T>
Handle<T> check_instance(NativeFunction::Frame& frame) {
    Handle<Value> value = frame.arg(0);
    if (!value->is<T>()) {
        TIRO_ERROR(
            "`this` is not a {}.", to_string(MapTypeToValueType<T>::type));
    }
    return value.cast<T>();
}

class ClassBuilder {
public:
    explicit ClassBuilder(Context& ctx)
        : ctx_(ctx)
        , table_(ctx, HashTable::make(ctx)) {}

    ClassBuilder& add(std::string_view name, u32 argc,
        NativeFunction::FunctionType native_func) {
        Root<Symbol> member(ctx_, ctx_.get_symbol(name));
        Root<String> member_str(ctx_, member->name());
        Root<NativeFunction> func(ctx_,
            NativeFunction::make(ctx_, member_str, {}, argc, native_func));
        Root<Method> method(ctx_, Method::make(ctx_, func.handle()));
        table_->set(ctx_, member.handle(), method.handle());
        return *this;
    }

    HashTable table() { return table_; }

private:
    Context& ctx_;
    Root<HashTable> table_;
};

} // namespace

static HashTable hash_table_class(Context& ctx) {
    ClassBuilder builder(ctx);

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

    builder //
        .add("set", 3, set)
        .add("contains", 2, contains)
        .add("remove", 2, remove);
    return builder.table();
}

static HashTable string_builder_class(Context& ctx) {
    ClassBuilder builder(ctx);

    const auto append = [](NativeFunction::Frame& frame) {
        auto self = check_instance<StringBuilder>(frame);
        for (size_t i = 1; i < frame.arg_count(); ++i) {
            Handle<Value> arg = frame.arg(i);
            if (arg->is<String>()) {
                self->append(frame.ctx(), arg.cast<String>());
            } else if (arg->is<StringBuilder>()) {
                self->append(frame.ctx(), arg.cast<StringBuilder>());
            } else {
                TIRO_ERROR(
                    "Cannot append values of type {}.", to_string(arg->type()));
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

    builder //
        .add("append", 2, append)
        .add("append_byte", 2, append_byte)
        .add("clear", 1, clear)
        .add("to_str", 1, to_str);

    return builder.table();
}

static HashTable buffer_class(Context& ctx) {
    ClassBuilder builder(ctx);

    const auto size = [](NativeFunction::Frame& frame) {
        auto self = check_instance<Buffer>(frame);
        frame.result(frame.ctx().get_integer(static_cast<i64>(self->size())));
    };

    builder.add("size", 1, size);
    return builder.table();
}

void TypeSystem::init(Context& ctx) {
    classes_.emplace(ValueType::HashTable, hash_table_class(ctx));
    classes_.emplace(ValueType::StringBuilder, string_builder_class(ctx));
    classes_.emplace(ValueType::Buffer, buffer_class(ctx));
}

Value TypeSystem::load_index(
    Context& ctx, Handle<Value> object, Handle<Value> index) {
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
            "Invalid index {} into array of size {}.", raw_index,
            array->size());
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
            "Invalid index {} into tuple of size {}.", raw_index,
            tuple->size());
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
            "Invalid index {} into buffer of size {}.", raw_index,
            buffer->size());

        return ctx.get_integer(buffer->get(size_t(raw_index)));
    }
    case ValueType::HashTable: {
        Handle<HashTable> table = object.cast<HashTable>();
        if (auto found = table->get(index.get())) {
            return *found;
        } else {
            return Value::null();
        }
        break;
    }
    default:
        TIRO_ERROR("Loading an index is not supported for objects of type {}.",
            to_string(object->type()));
    }
}

void TypeSystem::store_index(Context& ctx, Handle<Value> object,
    Handle<Value> index, Handle<Value> value) {
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
            "Invalid index {} into array of size {}.", raw_index,
            array->size());
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
            "Invalid index {} into tuple of size {}.", raw_index,
            tuple->size());
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
        if (auto val = try_extract_integer(value);
            val && *val >= 0 && *val <= 255) {
            raw_value = *val;
        } else {
            TIRO_ERROR(
                "Buffer value must a valid byte (integers 0 through 255).");
        }

        TIRO_CHECK(raw_index >= 0 && u64(raw_index) < buffer->size(),
            "Invalid index {} into buffer of size {}.", raw_index,
            buffer->size());

        buffer->set(size_t(raw_index), raw_value);
        break;
    }
    case ValueType::HashTable: {
        Handle<HashTable> table = object.cast<HashTable>();
        table->set(ctx, index, value);
        break;
    }
    default:
        TIRO_ERROR("Storing an index is not supported for objects of type {}.",
            to_string(object->type()));
    }
}

std::optional<Value> TypeSystem::load_member([[maybe_unused]] Context& ctx,
    Handle<Value> object, Handle<Symbol> member) {
    switch (object->type()) {
    case ValueType::Module: {
        auto module = object.cast<Module>();
        // TODO Exported should be name -> index only instead of returning the values directly.
        // Encapsulate that in the module class.
        return module->exported().get(member.get());
    }
    case ValueType::DynamicObject: {
        auto dyn = object.cast<DynamicObject>();
        return dyn->get(member);
    }
    default:
        TIRO_ERROR("load_member not implemented for this type yet: {}.",
            to_string(object->type()));
    }
}

bool TypeSystem::store_member(Context& ctx, Handle<Value> object,
    Handle<Symbol> member, Handle<Value> value) {
    switch (object->type()) {
    case ValueType::Module:
        return false;
    case ValueType::DynamicObject: {
        auto dyn = object.cast<DynamicObject>();
        dyn->set(ctx, member, value);
        return true;
    }
    default:
        TIRO_ERROR("store_member not implemented for this type yet: {}.",
            to_string(object->type()));
    }
}

std::optional<Value> TypeSystem::load_method(
    Context& ctx, Handle<Value> object, Handle<Symbol> member) {

    switch (object->type()) {
    case ValueType::Module:
    case ValueType::DynamicObject:
        return load_member(ctx, object, member);

    default: {
        auto class_pos = classes_.find(object->type());
        if (class_pos == classes_.end())
            return {};

        auto members = Handle<HashTable>::from_slot(&class_pos->second);
        return members->get(member.get());
    }
    }
}

} // namespace tiro::vm
