#ifndef HAMMER_VM_OBJECTS_COROUTINE_IPP
#define HAMMER_VM_OBJECTS_COROUTINE_IPP

#include "hammer/vm/objects/coroutine.hpp"

#include "hammer/vm/objects/object.ipp"

namespace hammer::vm {

struct CoroutineStack::Data : Header {
    Data(Undefined undef_, size_t stack_size)
        : Header(ValueType::CoroutineStack)
        , undef(undef_) {
        top = &data[0];
        end = &data[stack_size];
        // Unused portions of the stack are uninitialized
    }

    Undefined undef;
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
    Data* const d = data();

    w(d->undef);

    byte* max = d->top;
    Frame* frame = top_frame();

    while (frame) {
        w(frame->tmpl);
        w(frame->closure);

        // Visit all locals and values on the stack; params are not visited here,
        // the upper frame will do it since they are normal values there.
        w.array(ArrayVisitor(locals_begin(frame), values_end(frame, max)));

        max = reinterpret_cast<byte*>(frame);
        frame = frame->caller;
    }

    // Values before the first function call frame.
    w.array(ArrayVisitor(values_begin(nullptr), values_end(nullptr, max)));
}

CoroutineStack::Data* CoroutineStack::data() const noexcept {
    return access_heap<Data>();
}

struct Coroutine::Data : public Header {
    Data(String name_, Value function_, CoroutineStack stack_)
        : Header(ValueType::Coroutine)
        , name(name_)
        , function(function_)
        , stack(stack_) {}

    String name;
    Value function;
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
    w(d->function);
    w(d->stack);
    w(d->result);
}

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_COROUTINE_IPP
