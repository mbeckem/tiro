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

        HAMMER_ASSERT(code_.size() <= std::numeric_limits<u32>::max(), "Code too large.");
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

struct Function::Data : Header {
    Data(FunctionTemplate tmpl_, Value closure_)
        : Header(ValueType::Function)
        , tmpl(tmpl_)
        , closure(closure_) {}

    FunctionTemplate tmpl;
    Value closure;
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

struct CoroutineStack::Data : Header {
    Data(size_t stack_size)
        : Header(ValueType::CoroutineStack) {
        top = &data[0];
        end = &data[stack_size];
        // Unused portions of the stack are uninitialized
    }

    Frame* top_frame = nullptr;
    byte* top;
    byte* end;
    alignas(Frame) byte data[];
};

size_t CoroutineStack::object_size() const noexcept {
    return sizeof(Data) + stack_size();
}

template<typename W>
void CoroutineStack::walk(W&& w) {
    byte* max = data()->top;
    Frame* frame = top_frame();

    while (frame) {
        w(frame->tmpl);
        w(frame->closure);

        // Visit all locals and values on the stack; params are not visited here,
        // the upper frame will do it since they are normal values there.
        w(Span<Value>(locals_begin(frame), values_end(frame, max)));

        frame = frame->caller;
        max = reinterpret_cast<byte*>(frame);
    }

    // Values before the first function call frame.
    w(Span<Value>(values_begin(nullptr), values_end(nullptr, max)));
}

CoroutineStack::Data* CoroutineStack::data() const noexcept {
    return access_heap<Data>();
}

struct Coroutine::Data : public Header {
    Data(String name_, CoroutineStack stack_)
        : Header(ValueType::Coroutine)
        , name(name_)
        , stack(stack_) {}

    String name;
    CoroutineStack stack;
    CoroutineState state = CoroutineState::Ready;
    Value result = Value::null();
};

size_t Coroutine::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Coroutine::walk(W&& w) {
    Data* d = access_heap<Data>();
    w(d->name);
    w(d->stack);
    w(d->result);
}

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECT_IPP
