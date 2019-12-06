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
    CoroutineFrame* top_frame = nullptr;
    byte* top;
    byte* end;
    alignas(CoroutineFrame) byte data[];
};

size_t CoroutineStack::object_size() const noexcept {
    return sizeof(Data) + stack_size();
}

template<typename W>
void CoroutineStack::walk(W&& w) {
    Data* const d = data();

    w(d->undef);

    byte* max = d->top;
    CoroutineFrame* frame = top_frame();
    while (frame) {
        // Visit all locals and values on the stack; params are not visited here,
        // the upper frame will do it since they are normal values there.
        w.array(ArrayVisitor(locals_begin(frame), values_end(frame, max)));

        switch (frame->type) {
        case FrameType::Async:
            HAMMER_UNREACHABLE("Not implemented."); // TODO
            break;
        case FrameType::User: {
            auto user_frame = static_cast<UserFrame*>(frame);
            w(user_frame->tmpl);
            w(user_frame->closure);
            break;
        }
        }

        max = reinterpret_cast<byte*>(frame);
        frame = frame->caller;
    }

    HAMMER_ASSERT(max == d->data, "Must have reached the bottom of the stack.");
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
    CoroutineState state = CoroutineState::New;
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
