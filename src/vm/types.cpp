#include "vm/types.hpp"

#include "vm/context.hpp"
#include "vm/math.hpp"
#include "vm/objects/all.hpp"
#include "vm/objects/type_desc.hpp"

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

    TypeBuilder& add_method(const MethodDesc& desc) {
        return add_method(desc.name, desc.params, desc.func, desc.flags);
    }

    TypeBuilder& add_method(std::string_view name, u32 argc, const NativeFunctionArg& func,
        /* MethodDesc::Flags */ int flags) {
        Scope sc(ctx_);
        Local member_name = sc.local(ctx_.get_symbol(name));
        Local member_str = sc.local(member_name->name());
        Local member_value = sc.local<Value>(
            NativeFunction::make(ctx_, member_str, {}, argc, func));

        if ((flags & MethodDesc::Static) == 0) {
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

void TypeSystem::init_internal(Context& ctx) {
    // Create internal type representations. These are used for the 'type' header field
    // of each object.
    {
        internal_types_[type_index<InternalType>()] = InternalType::make_root(ctx);

#define TIRO_INIT(T) (internal_types_[type_index<T>()] = (InternalType::make(ctx, ValueType::T)))

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
        TIRO_INIT(Coroutine);
        TIRO_INIT(CoroutineStack);
        TIRO_INIT(CoroutineToken);
        TIRO_INIT(Environment);
        TIRO_INIT(Float);
        TIRO_INIT(Function);
        TIRO_INIT(FunctionTemplate);
        TIRO_INIT(HashTable);
        TIRO_INIT(HashTableIterator);
        TIRO_INIT(HashTableKeyIterator);
        TIRO_INIT(HashTableKeyView);
        TIRO_INIT(HashTableStorage);
        TIRO_INIT(HashTableValueIterator);
        TIRO_INIT(HashTableValueView);
        TIRO_INIT(Integer);
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
    Local integer_type = sc.local(simple_type(ctx, "Integer"));
    Local function_type = sc.local(simple_type(ctx, "Function"));

#define TIRO_INIT(T, expr)                                                 \
    {                                                                      \
        public_types_[type_index<T>()] = (expr);                           \
        internal_types_[type_index<T>()].value().public_type(              \
            Handle<Type>::from_raw_slot(&public_types_[type_index<T>()])); \
    }

    TIRO_INIT(Array, from_desc(ctx, array_type_desc));
    TIRO_INIT(ArrayIterator, simple_type(ctx, "ArrayIterator"));
    TIRO_INIT(Boolean, simple_type(ctx, "Boolean"));
    TIRO_INIT(BoundMethod, *function_type);
    TIRO_INIT(Buffer, from_desc(ctx, buffer_type_desc));
    TIRO_INIT(Coroutine, from_desc(ctx, coroutine_type_desc));
    TIRO_INIT(CoroutineToken, from_desc(ctx, coroutine_token_type_desc));
    TIRO_INIT(Float, simple_type(ctx, "Float"));
    TIRO_INIT(Function, *function_type);
    TIRO_INIT(HashTable, from_desc(ctx, hash_table_type_desc));
    TIRO_INIT(HashTableKeyView, simple_type(ctx, "MapKeyView"));
    TIRO_INIT(HashTableValueView, simple_type(ctx, "MapValueView"));
    TIRO_INIT(HashTableIterator, simple_type(ctx, "MapIterator"));
    TIRO_INIT(HashTableKeyIterator, simple_type(ctx, "MapKeyIterator"));
    TIRO_INIT(HashTableValueIterator, simple_type(ctx, "MapValueIterator"));
    TIRO_INIT(Integer, *integer_type);
    TIRO_INIT(Module, simple_type(ctx, "Module"));
    TIRO_INIT(NativeFunction, *function_type);
    TIRO_INIT(NativeObject, simple_type(ctx, "NativeObject"));
    TIRO_INIT(NativePointer, simple_type(ctx, "NativePointer"));
    TIRO_INIT(Null, simple_type(ctx, "Null"));
    TIRO_INIT(Record, simple_type(ctx, "Record"));
    TIRO_INIT(Result, from_desc(ctx, result_type_desc));
    TIRO_INIT(Set, from_desc(ctx, set_type_desc));
    TIRO_INIT(SmallInteger, *integer_type);
    TIRO_INIT(String, from_desc(ctx, string_type_desc));
    TIRO_INIT(StringIterator, simple_type(ctx, "StringIterator"));
    TIRO_INIT(StringBuilder, from_desc(ctx, string_builder_type_desc));
    TIRO_INIT(StringSlice, from_desc(ctx, string_slice_type_desc));
    TIRO_INIT(Symbol, simple_type(ctx, "Symbol"));
    TIRO_INIT(Tuple, from_desc(ctx, tuple_type_desc));
    TIRO_INIT(TupleIterator, simple_type(ctx, "TupleIterator"));
    TIRO_INIT(Type, from_desc(ctx, type_type_desc));

#undef TIRO_INIT
}

Type TypeSystem::type_of(Handle<Value> object) {
    Nullable<Type> public_type;
    switch (object->category()) {
    case ValueCategory::Null:
        public_type = public_types_[type_index<Null>()];
        break;
    case ValueCategory::EmbeddedInteger:
        public_type = public_types_[type_index<SmallInteger>()];
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
    Nullable<Type> public_type = public_types_[type_index(builtin)];
    if (!public_type) {
        TIRO_ERROR(
            "Unsupported object type {} in type_of query (type is internal).", to_string(builtin));
    }
    return public_type.value();
}

Value TypeSystem::load_index(Context& ctx, Handle<Value> object, Handle<Value> index) {
    switch (object->type()) {
    case ValueType::Array: {
        Handle array = object.must_cast<Array>();

        i64 raw_index;
        if (auto opt = try_extract_integer(*index)) {
            raw_index = *opt;
        } else {
            TIRO_ERROR("Array index must be an integer.");
        }

        TIRO_CHECK(raw_index >= 0 && u64(raw_index) < array->size(),
            "Invalid index {} into array of size {}.", raw_index, array->size());
        return array->get(size_t(raw_index));
    }
    case ValueType::Tuple: {
        Handle tuple = object.must_cast<Tuple>();

        i64 raw_index;
        if (auto opt = try_extract_integer(*index)) {
            raw_index = *opt;
        } else {
            TIRO_ERROR("Tuple index must be an integer.");
        }

        TIRO_CHECK(raw_index >= 0 && u64(raw_index) < tuple->size(),
            "Invalid index {} into tuple of size {}.", raw_index, tuple->size());
        return tuple->get(size_t(raw_index));
    }
    case ValueType::Buffer: {
        Handle buffer = object.must_cast<Buffer>();

        i64 raw_index;
        if (auto opt = try_extract_integer(*index)) {
            raw_index = *opt;
        } else {
            TIRO_ERROR("Buffer index must be an integer.");
        }

        TIRO_CHECK(raw_index >= 0 && u64(raw_index) < buffer->size(),
            "Invalid index {} into buffer of size {}.", raw_index, buffer->size());

        return ctx.get_integer(buffer->get(size_t(raw_index)));
    }
    case ValueType::HashTable: {
        Handle table = object.must_cast<HashTable>();
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
        Handle array = object.must_cast<Array>();

        i64 raw_index;
        if (auto opt = try_extract_integer(*index)) {
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
        Handle tuple = object.must_cast<Tuple>();

        i64 raw_index;
        if (auto opt = try_extract_integer(*index)) {
            raw_index = *opt;
        } else {
            TIRO_ERROR("Tuple index must be an integer.");
        }

        TIRO_CHECK(raw_index >= 0 && u64(raw_index) < tuple->size(),
            "Invalid index {} into tuple of size {}.", raw_index, tuple->size());
        tuple->set(static_cast<size_t>(raw_index), *value);
        break;
    }
    case ValueType::Buffer: {
        Handle buffer = object.must_cast<Buffer>();

        i64 raw_index;
        if (auto opt = try_extract_integer(*index)) {
            raw_index = *opt;
        } else {
            TIRO_ERROR("Buffer index must be an integer.");
        }

        byte raw_value;
        if (auto val = try_extract_integer(*value); val && *val >= 0 && *val <= 255) {
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
        Handle table = object.must_cast<HashTable>();
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
        Handle module = object.must_cast<Module>();
        // TODO Exported should be name -> index only instead of returning the values directly.
        // Encapsulate that in the module type.
        return module->find_exported(*member);
    }
    case ValueType::Record: {
        Handle record = object.must_cast<Record>();
        return record->get(*member);
    }
    case ValueType::Type: {
        Handle type = object.must_cast<Type>();

        // Static data and plain function can be returned as-is. Methods must be unwrapped:
        // `const method = Type.method` returns a function that takes an instance of `Type` as its first argument.
        auto found = type->find_member(member);
        if (!found || !found->is<Method>())
            return found;

        return found->must_cast<Method>().function();
    }
    default: {
        // TODO: Lookup instance fields!
        Type type = type_of(object);
        auto found = type.find_member(member);

        if (!found || !found->is<Method>())
            return found;

        // Example: `const fn = object.member` where `member` is an instance method. The object
        // instance is implicitly bound.
        Scope sc(ctx);
        Local function = sc.local(found->must_cast<Method>().function());
        return BoundMethod::make(ctx, function, object);
    }
    }
}

bool TypeSystem::store_member(
    Context& ctx, Handle<Value> object, Handle<Symbol> member, Handle<Value> value) {
    switch (object->type()) {
    case ValueType::Module:
        return false;
    case ValueType::Record: {
        auto record = object.must_cast<Record>();
        return Record::set(ctx, record, member, value);
    }
    case ValueType::Type: {
        TIRO_ERROR("Cannot modify values on type instances yet."); // TODO Static fields
    }
    default:
        TIRO_ERROR(
            "store_member not implemented for this type yet: {}.", to_string(object->type()));
    }
}

Value TypeSystem::iterator(Context& ctx, Handle<Value> object) {
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
        TIRO_ERROR("The type '{}' does not support iteration.", to_string(object->type()));
    }
}

std::optional<Value> TypeSystem::iterator_next(Context& ctx, Handle<Value> iterator) {
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
        TIRO_ERROR("The type '{}' does not support the iterator protocol.");
    }
}

std::optional<Value>
TypeSystem::load_method(Context& ctx, Handle<Value> object, Handle<Symbol> member) {
    // TODO: Implement fields.
    switch (object->type()) {
    case ValueType::Module:
    case ValueType::Record:
    case ValueType::Type:
        return load_member(ctx, object, member);

    default: {
        // TODO: Instance fields are not implemented.
        auto public_type = public_types_[type_index(object->type())];
        if (!public_type)
            return {};

        auto found = public_type.value().find_member(member);
        return found;
    }
    }
}

} // namespace tiro::vm
