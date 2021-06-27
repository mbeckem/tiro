#include "vm/type_system.hpp"

#include "vm/context.hpp"
#include "vm/math.hpp"
#include "vm/object_support/type_desc.hpp"
#include "vm/objects/all.hpp"

namespace tiro::vm {

namespace {

class TypeBuilder {
public:
    explicit TypeBuilder(Context& ctx)
        : ctx_(ctx)
        , sc_(ctx)
        , name_(sc_.local<Nullable<String>>())
        , table_(sc_.local(HashTable::make(ctx))) {}

    TypeBuilder& name(std::string_view name) {
        name_.set(ctx_.get_interned_string(name));
        return *this;
    }

    TypeBuilder& add_method(const FunctionDesc& desc) {
        return add_method(desc.name, desc.params, desc.func, desc.flags);
    }

    TypeBuilder& add_method(std::string_view name, u32 argc, const NativeFunctionStorage& func,
        /* MethodDesc::Flags */ int flags) {
        Scope sc(ctx_);
        Local member_name = sc.local(ctx_.get_symbol(name));
        Local member_str = sc.local(member_name->name());
        Local member_value = sc.local<Value>(
            NativeFunction::make(ctx_, member_str, {}, argc, func));

        if (flags & FunctionDesc::InstanceMethod) {
            member_value = Method::make(ctx_, member_value);
        }

        // TODO: Flags::Variadic
        table_->set(ctx_, member_name, member_value);
        return *this;
    }

    Type build() {
        if (!name_.get()) {
            name_.set(ctx_.get_interned_string("<anonymous type>"));
        }
        return Type::make(ctx_, name_.must_cast<String>(), table_);
    }

private:
    Context& ctx_;
    Scope sc_;
    Local<Nullable<String>> name_;
    Local<HashTable> table_;
};

} // namespace

static Type simple_type(Context& ctx, std::string_view name) {
    TypeBuilder builder(ctx);
    return builder.name(name).build();
}

static Type from_desc(Context& ctx, const TypeDesc& desc) {
    TypeBuilder builder(ctx);

    builder.name(desc.name);
    for (const auto& method : desc.methods) {
        builder.add_method(method);
    }

    return builder.build();
}

static Exception format_type_error(Context& ctx, Handle<Value> value,
    FunctionRef<void(Scope&, Handle<StringBuilder> builder, Handle<String> type_name)>
        build_message) {

    Scope sc(ctx);
    Local builder = sc.local(StringBuilder::make(ctx));
    Local type_name = sc.local(ctx.types().type_of(value).name());
    build_message(sc, builder, type_name);

    Local message = sc.local(builder->to_string(ctx));
    return Exception::make(ctx, message);
}

static Exception get_index_not_supported_exception(Context& ctx, Handle<Value> value) {
    return format_type_error(ctx, value, [&](Scope&, auto builder, auto type_name) {
        builder->append(ctx, "reading an index is not supported on objects of type ");
        builder->append(ctx, type_name);
    });
}

static Exception set_index_not_supported_exception(Context& ctx, Handle<Value> value) {
    return format_type_error(ctx, value, [&](Scope&, auto builder, auto type_name) {
        builder->append(ctx, "writing an index is not supported on objects of type ");
        builder->append(ctx, type_name);
    });
}

static Exception member_assignment_not_supported_exception(Context& ctx, Handle<Value> value) {
    return format_type_error(ctx, value, [&](Scope&, auto builder, auto type_name) {
        builder->append(ctx, "writing to a member is not supported on objects of type ");
        builder->append(ctx, type_name);
    });
}

static Exception
member_not_found_exception(Context& ctx, Handle<Value> value, Handle<Symbol> member) {
    return format_type_error(ctx, value, [&](Scope& sc, auto builder, auto type_name) {
        Local name = sc.local(member->name());

        builder->append(ctx, "member '");
        builder->append(ctx, name);
        builder->append(ctx, "' does not exist on object of type ");
        builder->append(ctx, type_name);
    });
}

static Exception
member_not_found_in_module_exception(Context& ctx, Handle<Module> module, Handle<Symbol> member) {
    return format_type_error(
        ctx, module, [&](Scope& sc, auto builder, [[maybe_unused]] auto type_name) {
            Local member_name = sc.local(member->name());
            Local module_name = sc.local(module->name());

            builder->append(ctx, "export '");
            builder->append(ctx, member_name);
            builder->append(ctx, "' does not exist on module '");
            builder->append(ctx, module_name);
            builder->append(ctx, "'");
        });
}

static Exception
member_not_found_in_type_exception(Context& ctx, Handle<Type> type, Handle<Symbol> member) {
    return format_type_error(ctx, type, [&](Scope& sc, auto builder, [[maybe_unused]] auto) {
        Local member_name = sc.local(member->name());
        Local type_name = sc.local(type->name());

        builder->append(ctx, "member '");
        builder->append(ctx, member_name);
        builder->append(ctx, "' does not exist on type '");
        builder->append(ctx, type_name);
        builder->append(ctx, "'");
    });
}

static Exception iteration_not_supported_exception(Context& ctx, Handle<Value> value) {
    return format_type_error(ctx, value, [&](Scope&, auto builder, auto type_name) {
        builder->append(ctx, "object of type ");
        builder->append(ctx, type_name);
        builder->append(ctx, " does not support iteration");
    });
}

static Exception not_an_iterator_exception(Context& ctx, Handle<Value> value) {
    return format_type_error(ctx, value, [&](Scope&, auto builder, auto type_name) {
        builder->append(ctx, "object of type ");
        builder->append(ctx, type_name);
        builder->append(ctx, " is not an iterator");
    });
}

Exception function_call_not_supported_exception(Context& ctx, Handle<Value> value) {
    return format_type_error(ctx, value, [&](Scope&, auto builder, auto type_name) {
        builder->append(ctx, "cannot call objects of type ");
        builder->append(ctx, type_name);
        builder->append(ctx, " as a function");
    });
}

Exception
assertion_failed_exception(Context& ctx, Handle<String> expr, MaybeHandle<String> message) {
    Scope sc(ctx);
    Local builder = sc.local(StringBuilder::make(ctx));
    builder->append(ctx, "assertion `");
    builder->append(ctx, expr);
    builder->append(ctx, "` failed");
    if (message) {
        builder->append(ctx, ": ");
        builder->append(ctx, message.handle());
    }
    Local str = sc.local(builder->to_string(ctx));
    return Exception::make(ctx, str);
}

static Fallible<size_t>
check_index_impl(Context& ctx, std::string_view name, size_t size, Handle<Value> index) {
    i64 raw_index;
    if (auto opt = Integer::try_extract(*index); TIRO_LIKELY(opt)) {
        raw_index = *opt;
    } else {
        return TIRO_FORMAT_EXCEPTION(ctx, "{} index must be an integer", name);
    }

    if (TIRO_UNLIKELY(raw_index < 0 || u64(raw_index) >= size)) {
        return TIRO_FORMAT_EXCEPTION(
            ctx, "invalid index {} into {} of size {}", raw_index, name, size);
    }

    return raw_index;
};

template<typename T>
static Fallible<size_t>
check_index(Context& ctx, std::string_view name, Handle<T> handle, Handle<Value> index) {
    return check_index_impl(ctx, name, handle->size(), index);
};

void TypeSystem::init_internal(Context& ctx) {
    // Create internal type representations. These are used for the 'type' header field
    // of each object.
    {
        internal_types_[value_type_index<InternalType>()] = InternalType::make_root(ctx);

#define TIRO_INIT(T) \
    (internal_types_[value_type_index<T>()] = (InternalType::make(ctx, ValueType::T)))

        /* [[[cog
            from cog import outl
            from codegen.objects import VM_OBJECTS
            for object in VM_OBJECTS:
                if object.name != "InternalType":
                    outl(f"TIRO_INIT({object.type_name});")
        ]]] */
        TIRO_INIT(Array);
        TIRO_INIT(ArrayIterator);
        TIRO_INIT(ArrayStorage);
        TIRO_INIT(Boolean);
        TIRO_INIT(BoundMethod);
        TIRO_INIT(Buffer);
        TIRO_INIT(Code);
        TIRO_INIT(CodeFunction);
        TIRO_INIT(CodeFunctionTemplate);
        TIRO_INIT(Coroutine);
        TIRO_INIT(CoroutineStack);
        TIRO_INIT(CoroutineToken);
        TIRO_INIT(Environment);
        TIRO_INIT(Exception);
        TIRO_INIT(Float);
        TIRO_INIT(HandlerTable);
        TIRO_INIT(HashTable);
        TIRO_INIT(HashTableIterator);
        TIRO_INIT(HashTableKeyIterator);
        TIRO_INIT(HashTableKeyView);
        TIRO_INIT(HashTableStorage);
        TIRO_INIT(HashTableValueIterator);
        TIRO_INIT(HashTableValueView);
        TIRO_INIT(HeapInteger);
        TIRO_INIT(MagicFunction);
        TIRO_INIT(Method);
        TIRO_INIT(Module);
        TIRO_INIT(NativeFunction);
        TIRO_INIT(NativeObject);
        TIRO_INIT(NativePointer);
        TIRO_INIT(Null);
        TIRO_INIT(Record);
        TIRO_INIT(RecordTemplate);
        TIRO_INIT(Result);
        TIRO_INIT(Set);
        TIRO_INIT(SetIterator);
        TIRO_INIT(SmallInteger);
        TIRO_INIT(String);
        TIRO_INIT(StringBuilder);
        TIRO_INIT(StringIterator);
        TIRO_INIT(StringSlice);
        TIRO_INIT(Symbol);
        TIRO_INIT(Tuple);
        TIRO_INIT(TupleIterator);
        TIRO_INIT(Type);
        TIRO_INIT(Undefined);
        TIRO_INIT(UnresolvedImport);
        // [[[end]]]

#undef TIRO_INIT
    }
}

void TypeSystem::init_public(Context& ctx) {
    // Initialize the public type object. These can be used from interpreted code.
    Scope sc(ctx);

#define TIRO_INIT(pt, expr)                                                              \
    {                                                                                    \
        /* note: slot is rooted */                                                       \
        auto& instance_slot = public_types_[public_type_index(PublicType::pt)] = (expr); \
        constexpr auto value_types = to_value_types(PublicType::pt);                     \
        for (auto vt : value_types) {                                                    \
            auto internal_instance = internal_types_[value_type_index(vt)].value();      \
            internal_instance.public_type(Handle<Type>::from_raw_slot(&instance_slot));  \
        }                                                                                \
    }

    TIRO_INIT(Array, from_desc(ctx, array_type_desc));
    TIRO_INIT(ArrayIterator, simple_type(ctx, "ArrayIterator"));
    TIRO_INIT(Boolean, simple_type(ctx, "Boolean"));
    TIRO_INIT(Buffer, from_desc(ctx, buffer_type_desc));
    TIRO_INIT(Coroutine, from_desc(ctx, coroutine_type_desc));
    TIRO_INIT(CoroutineToken, from_desc(ctx, coroutine_token_type_desc));
    TIRO_INIT(Exception, from_desc(ctx, exception_type_desc));
    TIRO_INIT(Float, simple_type(ctx, "Float"));
    TIRO_INIT(Function, simple_type(ctx, "Function"));
    TIRO_INIT(Integer, simple_type(ctx, "Integer"));
    TIRO_INIT(Map, from_desc(ctx, hash_table_type_desc));
    TIRO_INIT(MapIterator, simple_type(ctx, "MapIterator"));
    TIRO_INIT(MapKeyIterator, simple_type(ctx, "MapKeyIterator"));
    TIRO_INIT(MapKeyView, simple_type(ctx, "MapKeyView"));
    TIRO_INIT(MapValueIterator, simple_type(ctx, "MapValueIterator"));
    TIRO_INIT(MapValueView, simple_type(ctx, "MapValueView"));
    TIRO_INIT(Module, simple_type(ctx, "Module"));
    TIRO_INIT(NativeObject, simple_type(ctx, "NativeObject"));
    TIRO_INIT(NativePointer, simple_type(ctx, "NativePointer"));
    TIRO_INIT(Null, simple_type(ctx, "Null"));
    TIRO_INIT(Record, simple_type(ctx, "Record"));
    TIRO_INIT(Result, from_desc(ctx, result_type_desc));
    TIRO_INIT(Set, from_desc(ctx, set_type_desc));
    TIRO_INIT(SetIterator, simple_type(ctx, "SetIterator"));
    TIRO_INIT(String, from_desc(ctx, string_type_desc));
    TIRO_INIT(StringBuilder, from_desc(ctx, string_builder_type_desc));
    TIRO_INIT(StringIterator, simple_type(ctx, "StringIterator"));
    TIRO_INIT(StringSlice, from_desc(ctx, string_slice_type_desc));
    TIRO_INIT(Symbol, simple_type(ctx, "Symbol"));
    TIRO_INIT(Tuple, from_desc(ctx, tuple_type_desc));
    TIRO_INIT(TupleIterator, simple_type(ctx, "TupleIterator"));
    TIRO_INIT(Type, from_desc(ctx, type_type_desc));

#undef TIRO_INIT

#ifdef TIRO_DEBUG
    {
        size_t index = 0;
        for (const auto& instance : public_types_) {
            if (instance.is_null()) {
                TIRO_ERROR("Public type instance for '{}' is not initialized",
                    static_cast<PublicType>(index));
            }
        }
    }
#endif
}

Type TypeSystem::type_of(PublicType pt) {
    auto instance = public_types_[public_type_index(pt)];
    TIRO_DEBUG_ASSERT(!instance.is_null(), "Public type was not initalized.");
    return instance.value();
}

Type TypeSystem::type_of(Handle<Value> object) {
    Nullable<Type> public_type;
    switch (object->category()) {
    case ValueCategory::Null:
        public_type = type_of(PublicType::Null);
        break;
    case ValueCategory::EmbeddedInteger:
        public_type = type_of(PublicType::Integer);
        break;
    case ValueCategory::Heap:
        public_type = HeapValue(*object).type_instance().public_type();
        break;
    }

    if (!public_type) {
        TIRO_ERROR("Unsupported object type {} in type_of query (type is internal).",
            to_string(object->type()));
    }
    return public_type.value();
}

Type TypeSystem::type_of(ValueType builtin) {
    InternalType internal_instance = internal_types_[value_type_index(builtin)].value();
    Nullable<Type> public_type = internal_instance.public_type();
    if (!public_type) {
        TIRO_ERROR(
            "Unsupported object type {} in type_of query (type is internal).", to_string(builtin));
    }
    return public_type.value();
}

Fallible<Value> TypeSystem::load_index(Context& ctx, Handle<Value> object, Handle<Value> index) {
    switch (object->type()) {
    case ValueType::Array: {
        Handle array = object.must_cast<Array>();
        auto checked = check_index(ctx, "array", array, index);
        if (TIRO_UNLIKELY(checked.has_exception()))
            return checked.exception();

        return array->get(checked.value());
    }

    case ValueType::Tuple: {
        Handle tuple = object.must_cast<Tuple>();
        auto checked = check_index(ctx, "tuple", tuple, index);
        if (TIRO_UNLIKELY(checked.has_exception()))
            return checked.exception();

        return tuple->get(checked.value());
    }

    case ValueType::Buffer: {
        Handle buffer = object.must_cast<Buffer>();
        auto checked = check_index(ctx, "buffer", buffer, index);
        if (checked.has_exception())
            return checked.exception();

        return ctx.get_integer(buffer->get(checked.value()));
    }
    case ValueType::HashTable: {
        Handle table = object.must_cast<HashTable>();
        if (auto found = table->get(index.get())) {
            return *found;
        }
        return Value::null();
    }
    default:
        return get_index_not_supported_exception(ctx, object);
    }
}

Fallible<void> TypeSystem::store_index(
    Context& ctx, Handle<Value> object, Handle<Value> index, Handle<Value> value) {
    switch (object->type()) {
    case ValueType::Array: {
        Handle array = object.must_cast<Array>();
        auto checked = check_index(ctx, "array", array, index);
        if (TIRO_UNLIKELY(checked.has_exception()))
            return checked.exception();

        array->set(checked.value(), value);
        break;
    }
    case ValueType::Tuple: {
        Handle tuple = object.must_cast<Tuple>();
        auto checked = check_index(ctx, "tuple", tuple, index);
        if (TIRO_UNLIKELY(checked.has_exception()))
            return checked.exception();

        tuple->set(checked.value(), *value);
        break;
    }
    case ValueType::Buffer: {
        Handle buffer = object.must_cast<Buffer>();
        auto checked = check_index(ctx, "buffer", buffer, index);
        if (TIRO_UNLIKELY(checked.has_exception()))
            return checked.exception();

        byte raw_value;
        if (auto val = Integer::try_extract(*value); TIRO_LIKELY(val && *val >= 0 && *val <= 255)) {
            raw_value = *val;
        } else {
            return TIRO_FORMAT_EXCEPTION(
                ctx, "buffer value must a valid byte (integers 0 through 255)");
        }

        buffer->set(checked.value(), raw_value);
        break;
    }
    case ValueType::HashTable: {
        Handle table = object.must_cast<HashTable>();
        table->set(ctx, index, value);
        break;
    }
    default:
        return set_index_not_supported_exception(ctx, object);
    }

    return {};
}

Fallible<Value> TypeSystem::load_member(Context& ctx, Handle<Value> object, Handle<Symbol> member) {
    switch (object->type()) {
    case ValueType::Module: {
        Handle module = object.must_cast<Module>();
        // TODO Exported should be name -> index only instead of returning the values directly.
        // Encapsulate that in the module type.
        if (auto found = module->find_exported(*member)) {
            return *found;
        }
        return member_not_found_in_module_exception(ctx, module, member);
    }
    case ValueType::Record: {
        Handle record = object.must_cast<Record>();
        if (auto found = record->get(*member)) {
            return *found;
        }
        return member_not_found_exception(ctx, object, member);
    }
    case ValueType::Type: {
        Handle type = object.must_cast<Type>();

        // Static data and plain function can be returned as-is. Methods must be unwrapped:
        // `const method = Type.method` returns a function that takes an instance of `Type` as its first argument.
        // TODO: Static members on types
        auto found = type->find_member(member);
        if (!found || !found->is<Method>())
            return member_not_found_in_type_exception(ctx, type, member);

        return found->must_cast<Method>().function();
    }
    default: {
        // TODO: Lookup instance fields!
        Type type = type_of(object);
        auto found = type.find_member(member);

        // TODO: Static members on types
        if (!found || !found->is<Method>())
            return member_not_found_exception(ctx, object, member);

        // Example: `const fn = object.member` where `member` is an instance method. The object
        // instance is implicitly bound.
        Scope sc(ctx);
        Local function = sc.local(found->must_cast<Method>().function());
        return BoundMethod::make(ctx, function, object);
    }
    }
}

Fallible<void> TypeSystem::store_member(
    Context& ctx, Handle<Value> object, Handle<Symbol> member, Handle<Value> value) {
    switch (object->type()) {
    case ValueType::Module:
        return member_assignment_not_supported_exception(ctx, object);
    case ValueType::Record: {
        auto record = object.must_cast<Record>();
        if (TIRO_UNLIKELY(!Record::set(ctx, record, member, value)))
            return member_not_found_exception(ctx, object, member);
        return {};
    }

    case ValueType::Type: // TODO Static members
    default:
        return member_assignment_not_supported_exception(ctx, object);
    }
}

Fallible<Value> TypeSystem::load_method(Context& ctx, Handle<Value> object, Handle<Symbol> member) {
    // TODO: Implement fields.
    switch (object->type()) {
    case ValueType::Module:
    case ValueType::Record:
    case ValueType::Type:
        return load_member(ctx, object, member);

    default: {
        // TODO: Instance fields are not implemented.
        auto public_type = type_of(object);
        auto found = public_type.find_member(member);
        if (!found)
            return member_not_found_exception(ctx, object, member);

        return *found;
    }
    }
}

Fallible<Value> TypeSystem::iterator(Context& ctx, Handle<Value> object) {
    switch (object->type()) {
    case ValueType::Array:
        return ArrayIterator::make(ctx, object.must_cast<Array>());
    case ValueType::HashTable:
        return HashTableIterator::make(ctx, object.must_cast<HashTable>());
    case ValueType::HashTableKeyView: {
        Scope sc(ctx);
        Local table = sc.local(object.must_cast<HashTableKeyView>()->table());
        return HashTableKeyIterator::make(ctx, table);
    }
    case ValueType::HashTableValueView: {
        Scope sc(ctx);
        Local table = sc.local(object.must_cast<HashTableValueView>()->table());
        return HashTableValueIterator::make(ctx, table);
    }
    case ValueType::Set:
        return SetIterator::make(ctx, object.must_cast<Set>());
    case ValueType::String:
        return StringIterator::make(ctx, object.must_cast<String>());
    case ValueType::StringSlice:
        return StringIterator::make(ctx, object.must_cast<StringSlice>());
    case ValueType::Tuple:
        return TupleIterator::make(ctx, object.must_cast<Tuple>());
    default:
        return iteration_not_supported_exception(ctx, object);
    }
}

Fallible<std::optional<Value>> TypeSystem::iterator_next(Context& ctx, Handle<Value> iterator) {
    switch (iterator->type()) {
    case ValueType::ArrayIterator:
        return iterator.must_cast<ArrayIterator>()->next();
    case ValueType::HashTableIterator:
        return iterator.must_cast<HashTableIterator>()->next(ctx);
    case ValueType::HashTableKeyIterator:
        return iterator.must_cast<HashTableKeyIterator>()->next(ctx);
    case ValueType::HashTableValueIterator:
        return iterator.must_cast<HashTableValueIterator>()->next(ctx);
    case ValueType::SetIterator:
        return iterator.must_cast<SetIterator>()->next(ctx);
    case ValueType::StringIterator:
        return iterator.must_cast<StringIterator>()->next(ctx);
    case ValueType::TupleIterator:
        return iterator.must_cast<TupleIterator>()->next();
    default:
        return not_an_iterator_exception(ctx, iterator);
    }
}

} // namespace tiro::vm
