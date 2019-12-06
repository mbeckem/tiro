#ifndef HAMMER_VM_OBJECTS_FUNCTION_IPP
#define HAMMER_VM_OBJECTS_FUNCTION_IPP

#include "hammer/vm/objects/function.hpp"

#include "hammer/vm/objects/string.hpp"

namespace hammer::vm {

struct Code::Data : Header {
    Data(Span<const byte> code_)
        : Header(ValueType::Code)
        , size(code_.size()) {

        HAMMER_ASSERT(
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

struct ClosureContext::Data : public Header {
    Data(ClosureContext parent_, Undefined undef, size_t size_)
        : Header(ValueType::ClosureContext)
        , parent(parent_)
        , size(size_) {
        std::uninitialized_fill_n(values, size, undef);
    }

    ClosureContext parent;
    size_t size;
    Value values[];
};

size_t ClosureContext::object_size() const noexcept {
    return sizeof(Data) + size() * sizeof(Value);
}

template<typename W>
void ClosureContext::walk(W&& w) {
    Data* d = access_heap<Data>();
    w(d->parent);
    w.array(ArrayVisitor<Value>(d->values, d->size));
}

struct Function::Data : Header {
    Data(FunctionTemplate tmpl_, ClosureContext closure_)
        : Header(ValueType::Function)
        , tmpl(tmpl_)
        , closure(closure_) {}

    FunctionTemplate tmpl;
    ClosureContext closure;
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
    u32 min_params = 0;
    FunctionType func;
};

size_t NativeFunction::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void NativeFunction::walk(W&& w) {
    Data* d = access_heap();
    w(d->name);
}

NativeFunction::Data* NativeFunction::access_heap() const {
    return Value::access_heap<Data>();
}

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_FUNCTION_IPP
