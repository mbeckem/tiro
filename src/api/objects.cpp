#include "api/internal.hpp"

#include "vm/math.hpp"
#include "vm/objects/all.hpp"

#include <new>

using namespace tiro;
using namespace tiro::api;

const char* tiro_kind_str(tiro_kind_t kind) {
    switch (kind) {
#define TIRO_CASE(Kind)    \
    case TIRO_KIND_##Kind: \
        return #Kind;

        TIRO_CASE(NULL)
        TIRO_CASE(BOOLEAN)
        TIRO_CASE(INTEGER)
        TIRO_CASE(FLOAT)
        TIRO_CASE(STRING)
        TIRO_CASE(FUNCTION)
        TIRO_CASE(TUPLE)
        TIRO_CASE(RECORD)
        TIRO_CASE(RECORD_SCHEMA)
        TIRO_CASE(ARRAY)
        TIRO_CASE(RESULT)
        TIRO_CASE(EXCEPTION)
        TIRO_CASE(COROUTINE)
        TIRO_CASE(MODULE)
        TIRO_CASE(TYPE)
        TIRO_CASE(NATIVE)
        TIRO_CASE(INTERNAL)
        TIRO_CASE(INVALID)

#undef TIRO_KIND
    }

    return "<INVALID KIND>";
}

tiro_kind_t tiro_value_kind(tiro_vm_t vm, tiro_handle_t value) {
    if (!vm || !value)
        return TIRO_KIND_INVALID;

    return entry_point(nullptr, TIRO_KIND_INVALID, [&] {
        auto handle = to_internal(value);
        switch (handle->type()) {
#define TIRO_MAP(VmType, Kind)  \
    case vm::ValueType::VmType: \
        return TIRO_KIND_##Kind;

            TIRO_MAP(Null, NULL)
            TIRO_MAP(Boolean, BOOLEAN)
            TIRO_MAP(SmallInteger, INTEGER)
            TIRO_MAP(HeapInteger, INTEGER)
            TIRO_MAP(Float, FLOAT)
            TIRO_MAP(String, STRING)
            TIRO_MAP(Tuple, TUPLE)
            TIRO_MAP(Record, RECORD)
            TIRO_MAP(RecordSchema, RECORD_SCHEMA)
            TIRO_MAP(BoundMethod, FUNCTION)
            TIRO_MAP(CodeFunction, FUNCTION)
            TIRO_MAP(MagicFunction, FUNCTION)
            TIRO_MAP(NativeFunction, FUNCTION)
            TIRO_MAP(Array, ARRAY)
            TIRO_MAP(Result, RESULT)
            TIRO_MAP(Exception, EXCEPTION)
            TIRO_MAP(Coroutine, COROUTINE)
            TIRO_MAP(Module, MODULE)
            TIRO_MAP(Type, TYPE)
            TIRO_MAP(NativeObject, NATIVE)

        default:
            return TIRO_KIND_INTERNAL;

#undef TIRO_MAP
        }
    });
}

bool tiro_value_same(tiro_vm_t vm, tiro_handle_t a, tiro_handle_t b) {
    (void) vm;
    if (!a) {
        return !b;
    }
    if (!b) {
        return false;
    }
    return to_internal(a)->same(*to_internal(b));
}

static std::optional<vm::PublicType> get_type(tiro_kind_t kind) {
    switch (kind) {
#define TIRO_MAP(Kind, VmType) \
    case TIRO_KIND_##Kind:     \
        return vm::PublicType::VmType;

        TIRO_MAP(NULL, Null)
        TIRO_MAP(BOOLEAN, Boolean)
        TIRO_MAP(INTEGER, Integer)
        TIRO_MAP(FLOAT, Float)
        TIRO_MAP(STRING, String)
        TIRO_MAP(TUPLE, Tuple)
        TIRO_MAP(RECORD, Record)
        TIRO_MAP(RECORD_SCHEMA, RecordSchema)
        TIRO_MAP(FUNCTION, Function)
        TIRO_MAP(ARRAY, Array)
        TIRO_MAP(RESULT, Result)
        TIRO_MAP(EXCEPTION, Exception)
        TIRO_MAP(COROUTINE, Coroutine)
        TIRO_MAP(MODULE, Module)
        TIRO_MAP(TYPE, Type)
        TIRO_MAP(NATIVE, NativeObject)

    default:
        return {};

#undef TIRO_MAP
    }
};

void tiro_value_type(tiro_vm_t vm, tiro_handle_t value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !value || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto value_handle = to_internal(value);
        auto result_handle = to_internal(result);
        result_handle.set(ctx.types().type_of(value_handle));
    });
}

void tiro_value_to_string(
    tiro_vm_t vm, tiro_handle_t value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !value || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto value_handle = to_internal(value);
        auto result_handle = to_internal(result);

        vm::Scope sc(ctx);
        vm::Local builder = sc.local(vm::StringBuilder::make(ctx, 32));
        vm::to_string(ctx, builder, value_handle);
        result_handle.set(builder->to_string(ctx));
    });
}

void tiro_value_copy(tiro_vm_t vm, tiro_handle_t value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !value || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto value_handle = to_internal(value);
        auto result_handle = to_internal(result);
        result_handle.set(value_handle);
    });
}

void tiro_kind_type(tiro_vm_t vm, tiro_kind_t kind, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto public_type = get_type(kind);
        if (!public_type)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto result_handle = to_internal(result);
        result_handle.set(ctx.types().type_of(*public_type));
    });
}

void tiro_make_null(tiro_vm_t vm, tiro_handle_t result) {
    return entry_point(nullptr, [&] {
        if (!vm || !result)
            return;

        to_internal(result).set(vm::Value::null());
    });
}

void tiro_make_boolean(tiro_vm_t vm, bool value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!vm || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto result_handle = to_internal(result);
        result_handle.set(ctx.get_boolean(value));
    });
}

bool tiro_boolean_value(tiro_vm_t vm, tiro_handle_t value) {
    return entry_point(nullptr, false, [&] {
        if (!vm)
            return false;

        vm::Context& ctx = vm->ctx;
        return ctx.is_truthy(to_internal(value));
    });
}

void tiro_make_integer(tiro_vm_t vm, int64_t value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto result_handle = to_internal(result);
        result_handle.set(ctx.get_integer(value));
    });
}

int64_t tiro_integer_value(tiro_vm_t vm, tiro_handle_t value) {
    return entry_point(nullptr, 0, [&]() -> int64_t {
        if (!vm || !value)
            return 0;

        auto maybe_number = to_internal(value).try_cast<vm::Number>();
        if (maybe_number)
            return maybe_number.handle()->convert_int();
        return 0;
    });
}

void tiro_make_float(tiro_vm_t vm, double value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto result_handle = to_internal(result);
        result_handle.set(vm::Float::make(ctx, value));
    });
}

double tiro_float_value(tiro_vm_t vm, tiro_handle_t value) {
    return entry_point(nullptr, 0, [&]() -> double {
        if (!vm || !value)
            return 0;

        auto maybe_number = to_internal(value).try_cast<vm::Number>();
        if (maybe_number)
            return maybe_number.handle()->convert_float();
        return 0;
    });
}

void tiro_make_string(tiro_vm_t vm, tiro_string_t value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !result || !valid_string(value))
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto result_handle = to_internal(result);
        result_handle.set(vm::String::make(ctx, to_internal(value)));
    });
}

void tiro_string_value(
    tiro_vm_t vm, tiro_handle_t string, tiro_string_t* value, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !string || !value)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_string_handle = to_internal(string).try_cast<vm::String>();
        if (!maybe_string_handle)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto string_handle = maybe_string_handle.handle();
        auto storage = string_handle->view();
        *value = to_external(storage);
    });
}

void tiro_string_cstr(tiro_vm_t vm, tiro_handle_t string, char** result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !string || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_string_handle = to_internal(string).try_cast<vm::String>();
        if (!maybe_string_handle)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto string_handle = maybe_string_handle.handle();
        *result = copy_to_cstr(string_handle->view());
    });
}

void tiro_make_tuple(tiro_vm_t vm, size_t size, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto result_handle = to_internal(result);
        result_handle.set(vm::Tuple::make(ctx, size));
    });
}

size_t tiro_tuple_size(tiro_vm_t vm, tiro_handle_t tuple) {
    return entry_point(nullptr, 0, [&]() -> size_t {
        if (!vm || !tuple)
            return 0;

        auto maybe_tuple = to_internal(tuple).try_cast<vm::Tuple>();
        if (!maybe_tuple)
            return 0;

        return maybe_tuple.handle()->size();
    });
}

void tiro_tuple_get(
    tiro_vm_t vm, tiro_handle_t tuple, size_t index, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !tuple || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_tuple = to_internal(tuple).try_cast<vm::Tuple>();
        if (!maybe_tuple)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto tuple_handle = maybe_tuple.handle();
        if (size_t size = tuple_handle->size(); TIRO_UNLIKELY(index >= size))
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        to_internal(result).set(tuple_handle->unchecked_get(index));
    });
}

void tiro_tuple_set(
    tiro_vm_t vm, tiro_handle_t tuple, size_t index, tiro_handle_t value, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !tuple || !value)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_tuple = to_internal(tuple).try_cast<vm::Tuple>();
        if (!maybe_tuple)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto tuple_handle = maybe_tuple.handle();
        if (size_t size = tuple_handle->size(); TIRO_UNLIKELY(index >= size))
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        tuple_handle->unchecked_set(index, *to_internal(value));
    });
}

void tiro_make_record_schema(
    tiro_vm_t vm, tiro_handle_t keys, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !keys || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto maybe_array = to_internal(keys).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);
        auto array = maybe_array.handle();

        // Expensive mapping of strings to symbols, but we don't want to expose symbols in the public api (yet).
        vm::Scope sc(ctx);
        vm::Local symbols = sc.local(vm::Array::make(ctx, array->size()));
        {
            vm::Local key = sc.local();
            vm::Local symbol = sc.local();
            for (size_t i = 0, n = array->size(); i < n; ++i) {
                key = array->unchecked_get(i);
                if (!key->is<vm::String>())
                    return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

                symbol = ctx.get_symbol(key.must_cast<vm::String>());
                symbols->append(ctx, symbol)
                    .must("failed to add record key"); // array has needed capacity
            }
        }

        auto result_handle = to_internal(result);
        result_handle.set(vm::RecordSchema::make(ctx, symbols));
    });
}

void tiro_make_record(tiro_vm_t vm, tiro_handle_t schema, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !schema || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto maybe_schema = to_internal(schema).try_cast<vm::RecordSchema>();
        if (!maybe_schema)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto schema_handle = maybe_schema.handle();
        auto result_handle = to_internal(result);
        result_handle.set(vm::Record::make(ctx, schema_handle));
    });
}

void tiro_record_keys(tiro_vm_t vm, tiro_handle_t record, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !record || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto maybe_record = to_internal(record).try_cast<vm::Record>();
        if (!maybe_record)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);
        auto record_handle = maybe_record.handle();

        // Map symbol keys back to strings again.
        vm::Scope sc(ctx);
        vm::Local symbols = sc.local(vm::Record::keys(ctx, record_handle));
        vm::Local keys = sc.local(vm::Array::make(ctx, symbols->size()));
        {
            vm::Local key = sc.local();
            for (size_t i = 0, n = symbols->size(); i < n; ++i) {
                key = symbols->unchecked_get(i).must_cast<vm::Symbol>().name();
                // array has needed capacity
                keys->append(ctx, key).must("failed to add record key");
            }
        }

        auto result_handle = to_internal(result);
        result_handle.set(keys);
    });
}

void tiro_record_get(tiro_vm_t vm, tiro_handle_t record, tiro_handle_t key, tiro_handle_t result,
    tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !record || !key || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto maybe_record = to_internal(record).try_cast<vm::Record>();
        if (!maybe_record)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);
        auto record_handle = maybe_record.handle();

        auto maybe_string = to_internal(key).try_cast<vm::String>();
        if (!maybe_string)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);
        auto string = maybe_string.handle();

        auto value = record_handle->get(ctx.get_symbol(string));
        if (!value)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_KEY);

        to_internal(result).set(*value);
    });
}

void tiro_record_set(
    tiro_vm_t vm, tiro_handle_t record, tiro_handle_t key, tiro_handle_t value, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !record || !key || !value)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto maybe_record = to_internal(record).try_cast<vm::Record>();
        if (!maybe_record)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);
        auto record_handle = maybe_record.handle();

        auto maybe_string = to_internal(key).try_cast<vm::String>();
        if (!maybe_string)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);
        auto string = maybe_string.handle();

        vm::Scope sc(ctx);
        vm::Local symbol = sc.local(ctx.get_symbol(string));

        bool success = record_handle->set(*symbol, *to_internal(value));
        if (!success)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_KEY);
    });
}

void tiro_make_array(
    tiro_vm_t vm, size_t initial_capacity, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto result_handle = to_internal(result);
        result_handle.set(vm::Array::make(ctx, initial_capacity));
    });
}

size_t tiro_array_size(tiro_vm_t vm, tiro_handle_t array) {
    return entry_point(nullptr, 0, [&]() -> size_t {
        if (!vm || !array)
            return 0;

        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return 0;

        return maybe_array.handle()->size();
    });
}

void tiro_array_get(
    tiro_vm_t vm, tiro_handle_t array, size_t index, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !array || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        if (size_t size = array_handle->size(); TIRO_UNLIKELY(index >= size))
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        to_internal(result).set(array_handle->unchecked_get(index));
    });
}

void tiro_array_set(
    tiro_vm_t vm, tiro_handle_t array, size_t index, tiro_handle_t value, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !array || !value)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        if (size_t size = array_handle->size(); TIRO_UNLIKELY(index >= size))
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        array_handle->unchecked_set(index, *to_internal(value));
    });
}

void tiro_array_push(tiro_vm_t vm, tiro_handle_t array, tiro_handle_t value, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !array || !value)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        if (TIRO_UNLIKELY(!array_handle->try_append(ctx, to_internal(value)))) {
            return TIRO_REPORT(
                err, TIRO_ERROR_ALLOC, [&]() -> std::string { return "array size too large"; });
        }
    });
}

void tiro_array_pop(tiro_vm_t vm, tiro_handle_t array, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        if (array_handle->size() == 0)
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        array_handle->remove_last();
    });
}

void tiro_array_clear(tiro_vm_t vm, tiro_handle_t array, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        array_handle->clear();
    });
}

void tiro_make_success(tiro_vm_t vm, tiro_handle_t value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !value || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto value_handle = to_internal(value);
        auto result_handle = to_internal(result);
        result_handle.set(vm::Result::make_success(ctx, value_handle));
    });
}

void tiro_make_error(tiro_vm_t vm, tiro_handle_t error, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !error || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto reason_handle = to_internal(error);
        auto result_handle = to_internal(result);
        result_handle.set(vm::Result::make_error(ctx, reason_handle));
    });
}

static std::optional<vm::Result::Which> result_which(tiro_vm_t vm, tiro_handle_t handle) noexcept {
    return entry_point(nullptr, std::nullopt, [&]() -> std::optional<vm::Result::Which> {
        if (!vm || !handle)
            return {};

        auto maybe_result = to_internal(handle).try_cast<vm::Result>();
        if (!maybe_result)
            return {};

        return maybe_result.handle()->which();
    });
}

bool tiro_result_is_success(tiro_vm_t vm, tiro_handle_t instance) {
    return result_which(vm, instance) == vm::Result::Success;
}

bool tiro_result_is_error(tiro_vm_t vm, tiro_handle_t instance) {
    return result_which(vm, instance) == vm::Result::Error;
}

void tiro_result_value(tiro_vm_t vm, tiro_handle_t instance, tiro_handle_t out, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !instance || !out)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_result = to_internal(instance).try_cast<vm::Result>();
        if (!maybe_result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto result_handle = maybe_result.handle();
        if (!result_handle->is_success())
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        auto out_handle = to_internal(out);
        out_handle.set(result_handle->unchecked_value());
    });
}

void tiro_result_error(tiro_vm_t vm, tiro_handle_t instance, tiro_handle_t out, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !instance || !out)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_result = to_internal(instance).try_cast<vm::Result>();
        if (!maybe_result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto result_handle = maybe_result.handle();
        if (!result_handle->is_error())
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        auto out_handle = to_internal(out);
        out_handle.set(result_handle->unchecked_error());
    });
}

void tiro_exception_message(
    tiro_vm_t vm, tiro_handle_t instance, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !instance || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_exception = to_internal(instance).try_cast<vm::Exception>();
        if (!maybe_exception)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto exception_handle = maybe_exception.handle();
        to_internal(result).set(exception_handle->message());
    });
}

void tiro_exception_trace(
    tiro_vm_t vm, tiro_handle_t instance, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !instance || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_exception = to_internal(instance).try_cast<vm::Exception>();
        if (!maybe_exception)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto exception_handle = maybe_exception.handle();
        to_internal(result).set(exception_handle->trace());
    });
}

void tiro_make_coroutine(tiro_vm_t vm, tiro_handle_t func, tiro_handle_t arguments,
    tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !func || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_func = to_internal(func).try_cast<vm::Function>();
        if (!maybe_func)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto args_handle = to_internal_maybe(arguments);
        auto result_handle = to_internal(result);

        if (args_handle) {
            auto args = args_handle.handle();
            if (!args->is<vm::Null>() && !args->is<vm::Tuple>())
                return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);
        }

        vm::Context& ctx = vm->ctx;
        result_handle.set(
            ctx.make_coroutine(maybe_func.handle(), args_handle.try_cast<vm::Tuple>()));
    });
}

bool tiro_coroutine_started(tiro_vm_t vm, tiro_handle_t coroutine) {
    return entry_point(nullptr, false, [&] {
        if (!vm || !coroutine)
            return false;

        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return false;

        return maybe_coro.handle()->state() != vm::CoroutineState::New;
    });
}

bool tiro_coroutine_completed(tiro_vm_t vm, tiro_handle_t coroutine) {
    return entry_point(nullptr, false, [&] {
        if (!vm || !coroutine)
            return false;

        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return false;

        return maybe_coro.handle()->state() == vm::CoroutineState::Done;
    });
}

void tiro_coroutine_result(
    tiro_vm_t vm, tiro_handle_t coroutine, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !coroutine || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto coro_handle = maybe_coro.handle();
        if (coro_handle->state() != vm::CoroutineState::Done)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        auto result_handle = to_internal(result);
        result_handle.set(coro_handle->result());
    });
}

void tiro_coroutine_set_callback(tiro_vm_t vm, tiro_handle_t coroutine,
    tiro_coroutine_callback_t callback, tiro_coroutine_cleanup_t cleanup, void* userdata,
    tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !coroutine || !callback)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        struct Callback final : vm::CoroutineCallback {
            tiro_coroutine_callback_t callback;
            tiro_coroutine_cleanup_t cleanup;
            void* userdata;

            Callback(tiro_coroutine_callback_t callback_, tiro_coroutine_cleanup_t cleanup_,
                void* userdata_) noexcept
                : callback(callback_)
                , cleanup(cleanup_)
                , userdata(userdata_) {}

            Callback(Callback&& other) noexcept
                : callback(std::exchange(other.callback, nullptr))
                , cleanup(std::exchange(other.cleanup, nullptr))
                , userdata(std::exchange(other.userdata, nullptr)) {}

            ~Callback() {
                if (cleanup)
                    cleanup(userdata);
            }

            void done(vm::Context& ctx, vm::Handle<vm::Coroutine> coroutine) override {
                TIRO_DEBUG_ASSERT(callback, "Cannot invoke callback on invalid instance.");

                // Must create a local handle because the external c api only understands
                // mutable values for now. Improvement: const_handles in the api.
                vm::Scope sc(ctx);
                vm::Local coro = sc.local<vm::Value>(coroutine);
                callback(vm_from_context(ctx), to_external(coro.mut()), userdata);
                if (cleanup) {
                    cleanup(userdata);
                    cleanup = nullptr;
                }
            }

            void move(void* dest, [[maybe_unused]] size_t size) noexcept override {
                TIRO_DEBUG_ASSERT(dest, "Invalid move destination.");
                TIRO_DEBUG_ASSERT(size == sizeof(*this), "Invalid move destination size.");
                new (dest) Callback(std::move(*this));
            }

            size_t size() override { return sizeof(Callback); }

            size_t align() override { return alignof(Callback); }
        };

        vm::Context& ctx = vm->ctx;

        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto coro_handle = maybe_coro.handle();

        Callback cb(callback, cleanup, userdata);
        ctx.set_callback(coro_handle, cb);
    });
}

void tiro_coroutine_start(tiro_vm_t vm, tiro_handle_t coroutine, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !coroutine)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto coro_handle = maybe_coro.handle();
        if (coro_handle->state() != vm::CoroutineState::New)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        ctx.start(coro_handle);
    });
}

void tiro_make_module(tiro_vm_t vm, tiro_string_t name, tiro_module_member_t* members,
    size_t members_length, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !valid_string(name) || name.length == 0 || (members_length > 0 && !members)
            || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        vm::Scope sc(ctx);
        vm::Local module_name = sc.local(ctx.get_interned_string(to_internal(name)));
        vm::Local module_members = sc.local(vm::Tuple::make(ctx, members_length));
        vm::Local module_exports = sc.local(vm::HashTable::make(ctx));

        vm::Local export_name = sc.local();
        vm::Local module_index = sc.local();
        for (size_t i = 0; i < members_length; ++i) {
            tiro_string_t raw_name = members[i].name;
            tiro_handle_t value = members[i].value;

            if (!valid_string(raw_name) || raw_name.length == 0 || !value)
                return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

            export_name = ctx.get_symbol(to_internal(raw_name));
            module_index = ctx.get_integer(i);
            module_members->unchecked_set(i, *to_internal(value));
            module_exports->set(ctx, export_name, module_index)
                .must("failed to insert module member"); // Size is preallocated
        }

        vm::Local module = sc.local(
            vm::Module::make(ctx, module_name, module_members, module_exports));
        module->initialized(true);
        to_internal(result).set(module);
    });
}

void tiro_module_get_export(tiro_vm_t vm, tiro_handle_t module, tiro_string_t export_name,
    tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !module || !valid_string(export_name) || export_name.length == 0 || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        vm::Scope sc(ctx);

        auto maybe_module = to_internal(module).try_cast<vm::Module>();
        if (!maybe_module)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto module_handle = maybe_module.handle();

        vm::Local export_symbol = sc.local(ctx.get_symbol(to_internal(export_name)));
        if (auto found = module_handle->find_exported(*export_symbol)) {
            to_internal(result).set(*found);
        } else {
            return TIRO_REPORT(err, TIRO_ERROR_EXPORT_NOT_FOUND);
        }
    });
}

void tiro_type_name(tiro_vm_t vm, tiro_handle_t type, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !type || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_type = to_internal(type).try_cast<vm::Type>();
        if (!maybe_type)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto type_handle = maybe_type.handle();
        auto result_handle = to_internal(result);
        result_handle.set(type_handle->name());
    });
}

void tiro_make_native(tiro_vm_t vm, const tiro_native_type_t* type_descriptor, size_t size,
    tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !type_descriptor || size == 0 || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto result_handle = to_internal(result);
        result_handle.set(vm::NativeObject::make(ctx, type_descriptor, size));
    });
}

const tiro_native_type_t* tiro_native_type_descriptor(tiro_vm_t vm, tiro_handle_t object) {
    return entry_point(nullptr, nullptr, [&]() -> const tiro_native_type_t* {
        if (!vm || !object)
            return nullptr;

        auto maybe_object = to_internal(object).try_cast<vm::NativeObject>();
        if (!maybe_object)
            return nullptr;

        return maybe_object.handle()->native_type();
    });
}

void* tiro_native_data(tiro_vm_t vm, tiro_handle_t object) {
    return entry_point(nullptr, nullptr, [&]() -> void* {
        if (!vm || !object)
            return nullptr;

        auto maybe_object = to_internal(object).try_cast<vm::NativeObject>();
        if (!maybe_object)
            return nullptr;

        return maybe_object.handle()->data();
    });
}

size_t tiro_native_size(tiro_vm_t vm, tiro_handle_t object) {
    return entry_point(nullptr, 0, [&]() -> size_t {
        if (!vm || !object)
            return 0;

        auto maybe_object = to_internal(object).try_cast<vm::NativeObject>();
        if (!maybe_object)
            return 0;

        return maybe_object.handle()->size();
    });
}
