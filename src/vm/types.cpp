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

    TypeBuilder& add(std::string_view name, u32 argc, NativeFunctionPtr func_ptr) {
        Scope sc(ctx_);
        Local member = sc.local(ctx_.get_symbol(name));
        Local member_str = sc.local(member->name());
        Local func = sc.local(NativeFunction::make(ctx_, member_str, {}, argc, func_ptr));
        Local method = sc.local(Method::make(ctx_, func));
        table_->set(ctx_, member, method);
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
        builder.add(method.name, method.params, method.func);
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
        TIRO_INIT(ArrayStorage);
        TIRO_INIT(Boolean);
        TIRO_INIT(BoundMethod);
        TIRO_INIT(Buffer);
        TIRO_INIT(Code);
        TIRO_INIT(Coroutine);
        TIRO_INIT(CoroutineStack);
        TIRO_INIT(DynamicObject);
        TIRO_INIT(Environment);
        TIRO_INIT(Float);
        TIRO_INIT(Function);
        TIRO_INIT(FunctionTemplate);
        TIRO_INIT(HashTable);
        TIRO_INIT(HashTableIterator);
        TIRO_INIT(HashTableStorage);
        TIRO_INIT(Integer);
        TIRO_INIT(Method);
        TIRO_INIT(Module);
        TIRO_INIT(NativeFunction);
        TIRO_INIT(NativeObject);
        TIRO_INIT(NativePointer);
        TIRO_INIT(Null);
        TIRO_INIT(SmallInteger);
        TIRO_INIT(String);
        TIRO_INIT(StringBuilder);
        TIRO_INIT(Symbol);
        TIRO_INIT(Tuple);
        TIRO_INIT(Type);
        TIRO_INIT(Undefined);
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
    TIRO_INIT(Boolean, simple_type(ctx, "Boolean"));
    TIRO_INIT(BoundMethod, *function_type);
    TIRO_INIT(Buffer, from_desc(ctx, buffer_type_desc));
    TIRO_INIT(Type, from_desc(ctx, type_type_desc));
    TIRO_INIT(Coroutine, simple_type(ctx, "Coroutine"));
    TIRO_INIT(DynamicObject, simple_type(ctx, "DynamicObject"));
    TIRO_INIT(Float, simple_type(ctx, "Float"));
    TIRO_INIT(Function, *function_type);
    TIRO_INIT(HashTable, from_desc(ctx, hash_table_type_desc));
    TIRO_INIT(Integer, *integer_type);
    TIRO_INIT(Module, simple_type(ctx, "Module"));
    TIRO_INIT(NativeFunction, *function_type);
    TIRO_INIT(NativeObject, simple_type(ctx, "NativeObject"));
    TIRO_INIT(NativePointer, simple_type(ctx, "NativePointer"));
    TIRO_INIT(Null, simple_type(ctx, "Null"));
    TIRO_INIT(SmallInteger, *integer_type);
    TIRO_INIT(String, from_desc(ctx, string_type_desc));
    TIRO_INIT(StringBuilder, from_desc(ctx, string_builder_type_desc));
    TIRO_INIT(Symbol, simple_type(ctx, "Symbol"));
    TIRO_INIT(Tuple, from_desc(ctx, tuple_type_desc));

#undef TIRO_INIT
}

Value TypeSystem::type_of(Handle<Value> object) {
    Value public_type;
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
    return public_type;
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
        return module->exported().get(*member);
    }
    case ValueType::DynamicObject: {
        Handle dyn = object.must_cast<DynamicObject>();
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
        auto dyn = object.must_cast<DynamicObject>();
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
        auto public_type = public_types_[type_index(object->type())];
        if (!public_type)
            return {};

        return public_type.value().find_method(member);
    }
    }
}

} // namespace tiro::vm
