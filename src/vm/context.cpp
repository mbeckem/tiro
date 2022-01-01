#include "vm/context.hpp"

#include "common/defs.hpp"
#include "common/scope_guards.hpp"
#include "vm/objects/all.hpp"

#include "vm/root_set.ipp"

#include <chrono>
#include <cmath>
#include <new>

namespace tiro::vm {

static constexpr tiro_native_type_t coroutine_callback_type = []() {
    tiro_native_type_t type{};
    type.name = "<internal coroutine callback>";
    type.finalizer = [](void* data, [[maybe_unused]] size_t size) {
        TIRO_DEBUG_ASSERT(data, "Invalid memory location");
        static_cast<CoroutineCallback*>(data)->~CoroutineCallback();
    };
    return type;
}();

static i64 timestamp() {
    using namespace std::chrono;

    auto now = time_point_cast<milliseconds>(steady_clock::now());
    return static_cast<i64>(now.time_since_epoch().count());
}

static ContextSettings default_settings([[maybe_unused]] Context& ctx, ContextSettings settings) {
    if (!settings.print_stdout) {
        settings.print_stdout = [](std::string_view message) {
            std::fwrite(message.data(), 1, message.size(), stdout);
            std::fflush(stdout);
        };
    }
    return settings;
}

CoroutineCallback::~CoroutineCallback() {}

Context::Context()
    : Context(ContextSettings{}) {}

Context::Context(ContextSettings settings)
    : settings_(default_settings(*this, std::move(settings)))
    , heap_(settings_.page_size_bytes, settings_.alloc)
    , startup_time_(timestamp()) {
    heap_.collector().roots(&roots_);
    heap_.max_size(settings_.max_heap_size_bytes);
    roots_.init(*this);
}

Context::~Context() {}

Coroutine Context::make_coroutine(Handle<Value> func, MaybeHandle<Tuple> args) {
    return roots_.get_interpreter().make_coroutine(func, args);
}

void Context::set_callback(Handle<Coroutine> coro, CoroutineCallback& on_complete) {
    Scope sc(*this);

    Local existing = sc.local(coro->native_callback());
    if (existing->has_value()) {
        coro->native_callback({});
    }

    Local callback = sc.local(
        NativeObject::make(*this, &coroutine_callback_type, on_complete.size()));
    void* data = callback->data();
    size_t size = callback->size();
    TIRO_DEBUG_ASSERT(data, "Invalid memory location");
    TIRO_DEBUG_ASSERT(size == on_complete.size(), "Invalid storage size.");
    TIRO_DEBUG_ASSERT(is_aligned(uintptr_t(data), uintptr_t(on_complete.align())),
        "Storage must have the required alignment.");
    on_complete.move(data, size);

    coro->native_callback(*callback);
}

void Context::start(Handle<Coroutine> coro) {
    if (TIRO_UNLIKELY(coro->state() != CoroutineState::New))
        TIRO_ERROR("coroutine must be in its initial state");

    coro->state(CoroutineState::Started);
    schedule_coroutine(coro);
}

void Context::run_ready() {
    if (running_)
        TIRO_ERROR("already running, nested calls are not allowed");

    Interpreter& interpreter = roots_.get_interpreter();

    running_ = true;
    ScopeExit reset = [&] { running_ = false; };

    loop_timestamp_ = timestamp() - startup_time_;

    Scope sc(*this);
    Local current = sc.local<Nullable<Coroutine>>();
    while (1) {
        current = dequeue_coroutine();
        if (current->is_null()) {
            break;
        }

        Handle<Coroutine> coro = current.must_cast<Coroutine>();
        interpreter.run(coro);
        if (coro->state() == CoroutineState::Done) {
            execute_callbacks(coro);
        }
    }
}

bool Context::has_ready() {
    return roots_.get_first_ready()->has_value();
}

Result Context::run_init(Handle<Value> func, MaybeHandle<Tuple> args) {
    Scope sc(*this);
    Local coro = sc.local(make_coroutine(func, args));
    start(coro);
    run_ready();

    const auto state = coro->state();
    if (state != CoroutineState::Done)
        TIRO_ERROR("async function calls during module initialization are not implemented yet");

    TIRO_DEBUG_ASSERT(coro->result().has_value(), "Coroutine result must not be null.");
    return coro->result().value();
}

void Context::schedule_coroutine(Handle<Coroutine> coro) {
    TIRO_DEBUG_ASSERT(!coro->is_null(), "Invalid coroutine.");
    TIRO_DEBUG_ASSERT(is_runnable(coro->state()), "Invalid coroutine state: cannot be run.");
    TIRO_DEBUG_ASSERT(!coro->next_ready().has_value(), "Runnable coroutine must not be linked.");

    MutHandle first_ready = roots_.get_first_ready();
    MutHandle last_ready = roots_.get_last_ready();
    if (*last_ready) {
        last_ready->value().next_ready(*coro);
        last_ready.set(coro);
    } else {
        first_ready.set(coro);
        last_ready.set(coro);
    }
}

Nullable<Coroutine> Context::dequeue_coroutine() {
    MutHandle first_ready = roots_.get_first_ready();
    if (!first_ready->has_value())
        return {};

    Coroutine coro = first_ready->value();
    first_ready.set(coro.next_ready());
    coro.next_ready({});

    if (!first_ready->has_value())
        roots_.get_last_ready().set(Nullable<Coroutine>());
    return coro;
}

void Context::execute_callbacks(Handle<Coroutine> coro) {
    TIRO_DEBUG_ASSERT(coro->state() == CoroutineState::Done, "Coroutine must have completed.");

    Scope sc(*this);
    Local callback = sc.local(coro->native_callback());
    if (!callback->has_value())
        return;

    Handle<NativeObject> callback_obj = callback.must_cast<NativeObject>();
    TIRO_DEBUG_ASSERT(callback_obj->native_type() == &coroutine_callback_type,
        "Coroutine completion callback has an unexpected tag.");

    CoroutineCallback* native_callback = static_cast<CoroutineCallback*>(callback_obj->data());
    native_callback->done(*this, coro);
}

void Context::resume_coroutine(Handle<Coroutine> coro) {
    TIRO_DEBUG_ASSERT(!coro->is_null(), "Invalid coroutine.");
    TIRO_DEBUG_ASSERT(
        coro->state() == CoroutineState::Running || coro->state() == CoroutineState::Waiting,
        "Coroutine must be in running or in waiting state.");

    // Resets the coroutine's current token. This ensures that users that still have a reference to the old token
    // cannot use it to resume the coroutine a second time. This is some basic error prevention against
    // issues that would be hard to track down otherwise.
    coro->reset_token();
    coro->state(CoroutineState::Ready);
    schedule_coroutine(coro);
}

Integer Context::get_integer(i64 value) {
    if (SmallInteger::fits(value)) {
        return SmallInteger::make(value);
    }
    return HeapInteger::make(*this, value);
}

String Context::get_interned_string(Handle<String> str) {
    if (str->interned())
        return *str;

    Scope sc(*this);
    Local interned = sc.local(str);
    intern_impl(interned.mut(), {});
    return *interned;
}

String Context::get_interned_string(std::string_view value) {
    // Improvement: we can avoid constructing the temporary string by introducing
    // a find_equivalent(hash, compare, ...) function to the table. Care must be taken
    // to use the same hash function in that case.
    Scope sc(*this);
    Local str = sc.local(String::make(*this, value));
    return get_interned_string(str);
}

Symbol Context::get_symbol(Handle<String> str) {
    Scope sc(*this);
    Local interned_str = sc.local(str);
    Local symbol = sc.local();

    intern_impl(interned_str.mut(), symbol.out());
    return *symbol.must_cast<Symbol>();
}

Symbol Context::get_symbol(std::string_view value) {
    Scope sc(*this);
    Local interned_str = sc.local(get_interned_string(value));
    return get_symbol(interned_str);
}

void Context::intern_impl(MutHandle<String> str, MaybeOutHandle<Symbol> assoc_symbol) {
    Scope sc(*this);

    Handle interned_strings = roots_.get_interned_strings();
    {
        Local existing_string = sc.local();
        Local existing_value = sc.local();
        if (auto found = interned_strings->find(*str)) {
            existing_string.set(found->first);
            existing_value.set(found->second);

            TIRO_DEBUG_ASSERT(existing_string->is<String>(), "Key must be a string.");
            TIRO_DEBUG_ASSERT(existing_string->must_cast<String>().interned(),
                "Existing string must have been interned.");
            TIRO_DEBUG_ASSERT(existing_value->is<Symbol>(), "Value must be a symbol.");

            if (assoc_symbol) {
                assoc_symbol.handle().set(existing_value.must_cast<Symbol>());
            }
            return str.set(existing_string.must_cast<String>());
        }
    }

    // TODO: I'm being lazy here, create a symbol right away. This could be delayed only
    // for those instances where a symbol is actually needed.
    Local symbol = sc.local(Symbol::make(*this, str));
    interned_strings->set(*this, str, symbol).must("failed to insert interned string");
    str->interned(true);

    if (assoc_symbol) {
        assoc_symbol.handle().set(symbol);
    }
}

} // namespace tiro::vm
