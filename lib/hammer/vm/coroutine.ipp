#ifndef HAMMER_VM_COROUTINE_IPP
#define HAMMER_VM_COROUTINE_IPP

#include "hammer/vm/coroutine.hpp"

#include "hammer/vm/object.ipp"

namespace hammer::vm {

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

#endif // HAMMER_VM_COROUTINE_IPP
