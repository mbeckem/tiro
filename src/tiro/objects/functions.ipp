#ifndef TIRO_OBJECTS_FUNCTIONS_IPP
#define TIRO_OBJECTS_FUNCTIONS_IPP

#include "tiro/objects/functions.hpp"

#include "tiro/objects/modules.hpp"
#include "tiro/objects/strings.hpp"

namespace tiro::vm {

struct Code::Data : Header {
    Data(Span<const byte> code_)
        : Header(ValueType::Code)
        , size(code_.size()) {

        TIRO_ASSERT(
            code_.size() <= std::numeric_limits<u32>::max(), "Code too large.");
        std::memcpy(code, code_.data(), code_.size());
    }

    u32 size = 0;
    byte code[];
};

size_t Code::object_size() const noexcept {
    return sizeof(Data) + size();
}

template<typename W>
void Code::walk(W&&) {}

struct FunctionTemplate::Data : public Header {
    Data()
        : Header(ValueType::FunctionTemplate) {}

    String name;
    Module module;
    Tuple literals;
    Code code;
    u32 params = 0;
    u32 locals = 0;
};

size_t FunctionTemplate::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void FunctionTemplate::walk(W&& w) {
    Data* d = access_heap<Data>();
    w(d->name);
    w(d->module);
    w(d->literals);
    w(d->code);
}

struct Environment::Data : public Header {
    Data(Environment parent_, Undefined undef, size_t size_)
        : Header(ValueType::Environment)
        , parent(parent_)
        , size(size_) {
        std::uninitialized_fill_n(values, size, undef);
    }

    Environment parent;
    size_t size;
    Value values[];
};

size_t Environment::object_size() const noexcept {
    return sizeof(Data) + size() * sizeof(Value);
}

template<typename W>
void Environment::walk(W&& w) {
    Data* d = access_heap<Data>();
    w(d->parent);
    w.array(ArrayVisitor<Value>(d->values, d->size));
}

struct Function::Data : Header {
    Data(FunctionTemplate tmpl_, Environment closure_)
        : Header(ValueType::Function)
        , tmpl(tmpl_)
        , closure(closure_) {}

    FunctionTemplate tmpl;
    Environment closure;
};

size_t Function::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Function::walk(W&& w) {
    Data* d = access_heap<Data>();
    w(d->tmpl);
    w(d->closure);
}

struct BoundMethod::Data : Header {
    Data(Value function_, Value object_)
        : Header(ValueType::BoundMethod)
        , function(function_)
        , object(object_) {}

    Value function;
    Value object;
};

size_t BoundMethod::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void BoundMethod::walk(W&& w) {
    Data* d = access_heap();
    w(d->function);
    w(d->object);
}

BoundMethod::Data* BoundMethod::access_heap() const {
    return Value::access_heap<Data>();
}

struct NativeFunction::Data final : Header {
    Data()
        : Header(ValueType::NativeFunction) {}

    String name;
    Tuple values;
    u32 params = 0;
    FunctionType func;
};

size_t NativeFunction::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void NativeFunction::walk(W&& w) {
    Data* d = access_heap();
    w(d->name);
    w(d->values);
}

NativeFunction::Data* NativeFunction::access_heap() const {
    return Value::access_heap<Data>();
}

struct NativeAsyncFunction::Data : Header {
    Data()
        : Header(ValueType::NativeAsyncFunction) {}

    String name;
    Tuple values;
    u32 params = 0;
    FunctionType function;
};

size_t NativeAsyncFunction::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void NativeAsyncFunction::walk(W&& w) {
    Data* d = access_heap();
    w(d->name);
    w(d->values);
}

NativeAsyncFunction::Data* NativeAsyncFunction::access_heap() const {
    return Value::access_heap<Data>();
}

} // namespace tiro::vm

#endif // TIRO_OBJECTS_FUNCTIONS_IPP
