#ifndef HAMMER_VM_OBJECT_IPP
#define HAMMER_VM_OBJECT_IPP

#include "hammer/vm/object.hpp"

namespace hammer::vm {

struct Undefined::Data : Header {
    Data()
        : Header(ValueType::Undefined) {}
};

size_t Undefined::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Undefined::walk(W&&) {}

struct Boolean::Data : Header {
    Data(bool v)
        : Header(ValueType::Boolean)
        , value(v) {}

    bool value;
};

size_t Boolean::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Boolean::walk(W&&) {}

struct Integer::Data : Header {
    Data(i64 v)
        : Header(ValueType::Integer)
        , value(v) {}

    i64 value;
};

size_t Integer::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Integer::walk(W&&) {}

struct Float::Data : Header {
    Data(double v)
        : Header(ValueType::Float)
        , value(v) {}

    double value;
};

size_t Float::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
inline void Float::walk(W&&) {}

struct String::Data : Header {
    Data(std::string_view str)
        : Header(ValueType::String)
        , size(str.size()) {
        std::memcpy(data, str.data(), str.size());
    }

    size_t hash = 0;
    size_t size;
    char data[];
};

size_t String::object_size() const noexcept {
    return sizeof(Data) + size();
}

template<typename W>
void String::walk(W&&) {}

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
    Array literals;
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
    Data(ClosureContext parent_, size_t size_)
        : Header(ValueType::ClosureContext)
        , parent(parent_)
        , size(size_) {
        std::uninitialized_fill_n(values, size, Undefined());
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
    w(Span<Value>(d->values, d->size));
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

struct Module::Data : Header {
    Data(String name_, Array members_)
        : Header(ValueType::Module)
        , name(name_)
        , members(members_) {}

    String name;
    Array members;
};

size_t Module::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Module::walk(W&& w) {
    Data* d = access_heap<Data>();
    w(d->name);
    w(d->members);
}

struct Array::Data : Header {
    Data(size_t size_)
        : Header(ValueType::Array)
        , size(size_) {
        std::uninitialized_fill_n(values, size, Value::null());
    }

    Data(Span<const Value> initial_values)
        : Header(ValueType::Array)
        , size(initial_values.size()) {
        std::uninitialized_copy_n(initial_values.data(), size, values);
    }

    size_t size;
    Value values[];
};

size_t Array::object_size() const noexcept {
    return sizeof(Data) + size() * sizeof(Value);
}

template<typename W>
void Array::walk(W&& w) {
    Data* d = access_heap<Data>();
    w(Span<Value>(d->values, d->size));
}

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECT_IPP
