#include "hammer/vm/types.hpp"

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/classes.hpp"
#include "hammer/vm/objects/functions.hpp"
#include "hammer/vm/objects/hash_tables.hpp"
#include "hammer/vm/objects/modules.hpp"
#include "hammer/vm/objects/strings.hpp"

namespace hammer::vm {

namespace {

template<typename T>
Handle<T> check_instance(NativeFunction::Frame& frame) {
    Handle<Value> value = frame.arg(0);
    if (!value->is<T>()) {
        HAMMER_ERROR(
            "`this` is not a {}.", to_string(MapTypeToValueType<T>::type));
    }
    return value.cast<T>();
}

template<typename T>
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
    ClassBuilder<HashTable> builder(ctx);

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
    ClassBuilder<StringBuilder> builder(ctx);

    const auto append = [](NativeFunction::Frame& frame) {
        auto self = check_instance<StringBuilder>(frame);
        for (size_t i = 1; i < frame.arg_count(); ++i) {
            Handle<Value> arg = frame.arg(i);
            if (arg->is<String>()) {
                self->append(frame.ctx(), arg.cast<String>());
            } else if (arg->is<StringBuilder>()) {
                self->append(frame.ctx(), arg.cast<StringBuilder>());
            } else {
                HAMMER_ERROR(
                    "Cannot append values of type {}.", to_string(arg->type()));
            }
        }
    };

    const auto to_str = [](NativeFunction::Frame& frame) {
        auto self = check_instance<StringBuilder>(frame);
        frame.result(self->make_string(frame.ctx()));
    };

    builder //
        .add("append", 2, append)
        .add("to_str", 1, to_str);

    return builder.table();
}

void TypeSystem::init(Context& ctx) {
    classes_.emplace(ValueType::HashTable, hash_table_class(ctx));
    classes_.emplace(ValueType::StringBuilder, string_builder_class(ctx));
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
        HAMMER_ERROR("load_member not implemented for this type yet: {}.",
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
        HAMMER_ERROR("store_member not implemented for this type yet: {}.",
            to_string(object->type()));
    }
}

std::optional<Value> TypeSystem::load_method([[maybe_unused]] Context& ctx,
    Handle<Value> object, Handle<Symbol> member) {
    if (object->is<Module>()) {
        Handle<Module> module = object.cast<Module>();
        return module->exported().get(member.get());
    } else {
        auto class_pos = classes_.find(object->type());
        if (class_pos == classes_.end())
            return {};

        auto members = Handle<HashTable>::from_slot(&class_pos->second);
        return members->get(member.get());
    }
}

} // namespace hammer::vm
