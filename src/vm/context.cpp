#include "vm/context.hpp"

#include "common/defs.hpp"
#include "common/string_table.hpp"
#include "vm/objects/all.hpp"

#include "vm/context.ipp"

#include <chrono>
#include <cmath>
#include <new>

namespace tiro::vm {

namespace {

int completion_callback_dummy;

struct CompletionCallback {
    std::function<void(MutHandle<Coroutine>)> on_done;

    static constexpr const void* tag = &completion_callback_dummy;

    static void
    construct(void* data, size_t size, std::function<void(MutHandle<Coroutine>)> on_done) {
        TIRO_DEBUG_ASSERT(data, "Invalid memory location");
        TIRO_DEBUG_ASSERT(size == sizeof(CompletionCallback), "Invalid storage size.");
        new (data) CompletionCallback{std::move(on_done)};
    }

    static void destroy(void* data, size_t size) {
        TIRO_DEBUG_ASSERT(data, "Invalid memory location");
        TIRO_DEBUG_ASSERT(size == sizeof(CompletionCallback), "Invalid storage size.");
        static_cast<CompletionCallback*>(data)->~CompletionCallback();
    }
};

}; // namespace

static i64 timestamp() {
    using namespace std::chrono;

    auto now = time_point_cast<milliseconds>(steady_clock::now());
    return static_cast<i64>(now.time_since_epoch().count());
}

Context::Context()
    : heap_(this)
    , startup_time_(timestamp()) {
    interpreter_.init(*this);
    types_.init_internal(*this);

    true_ = Boolean::make(*this, true);
    false_ = Boolean::make(*this, false);
    undefined_ = Undefined::make(*this);
    interned_strings_ = HashTable::make(*this);
    modules_ = HashTable::make(*this);
    coroutines_ = Set::make(*this);

    types_.init_public(*this);
}

Context::~Context() {}

Coroutine Context::make_coroutine(Handle<Value> func, MaybeHandle<Tuple> args) {
    return interpreter_.make_coroutine(func, args);
}

void Context::set_callback(
    Handle<Coroutine> coro, std::function<void(MutHandle<Coroutine>)> on_complete) {
    Scope sc(*this);

    Local existing = sc.local(coro->native_callback());
    if (existing->has_value()) {
        existing->value().finalize();
        coro->native_callback({});
    }

    if (!on_complete)
        return;

    Local callback = sc.local(NativeObject::make(*this, sizeof(CompletionCallback)));
    callback->construct(
        CompletionCallback::tag,
        [&](void* data, size_t size) {
            CompletionCallback::construct(data, size, std::move(on_complete));
        },
        &CompletionCallback::destroy);
    coro->native_callback(*callback);
}

void Context::start(Handle<Coroutine> coro) {
    TIRO_CHECK(coro->state() == CoroutineState::New, "Coroutine must be in its initial state.");
    schedule_coroutine(coro);
}

void Context::run_ready() {
    if (running_) {
        TIRO_ERROR("Already running, nested calls are not allowed.");
    }

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
        interpreter_.run(coro);
        if (coro->state() == CoroutineState::Done) {
            execute_callbacks(coro);
        }
    }
}

bool Context::has_ready() const {
    return first_ready_.has_value();
}

Value Context::run_init(Handle<Value> func, MaybeHandle<Tuple> args) {
    loop_timestamp_ = timestamp() - startup_time_;

    Scope sc(*this);
    Local coro = sc.local(make_coroutine(func, args));
    interpreter_.run(coro);

    if (coro->state() != CoroutineState::Done)
        TIRO_ERROR("Async function calls during module initialization are not implemented yet.");

    return coro->result();
}

void Context::schedule_coroutine(Handle<Coroutine> coro) {
    TIRO_DEBUG_ASSERT(!coro->is_null(), "Invalid coroutine.");
    TIRO_DEBUG_ASSERT(is_runnable(coro->state()), "Invalid coroutine state: cannot be run.");
    TIRO_DEBUG_ASSERT(!coro->next_ready().has_value(), "Runnable coroutine must not be linked.");

    if (last_ready_) {
        last_ready_.value().next_ready(coro);
        last_ready_ = coro.get();
    } else {
        first_ready_ = last_ready_ = coro.get();
    }
}

Nullable<Coroutine> Context::dequeue_coroutine() {
    Nullable<Coroutine> next = first_ready_;
    if (!next)
        return next;

    first_ready_ = next.value().next_ready();
    if (!first_ready_)
        last_ready_ = Nullable<Coroutine>();

    return next;
}

void Context::execute_callbacks(Handle<Coroutine> coro) {
    TIRO_DEBUG_ASSERT(coro->state() == CoroutineState::Done, "Coroutine must have completed.");

    Scope sc(*this);
    Local coro_copy = sc.local(coro); // Mutable handle that will be passed to the callback
    Local callback = sc.local(coro->native_callback());
    if (!callback->has_value())
        return;

    Handle<NativeObject> callback_obj = callback.must_cast<NativeObject>();
    TIRO_DEBUG_ASSERT(callback_obj->tag() == CompletionCallback::tag,
        "Coroutine completion callback has an unexpected tag.");

    CompletionCallback* native_callback = static_cast<CompletionCallback*>(callback_obj->data());
    native_callback->on_done(coro_copy.mut());
    callback_obj->finalize();
}

/// This function is called by the runtime when an async (native) function resumes
/// after yielding. We are either being invoked from another thread (via post() on the io context)
/// or from this thread (from an async callback using dispatch()).
/// In any event, we will be run by the loop in Context::run().
void Context::resume_coroutine(Handle<Coroutine> coro) {
    TIRO_DEBUG_ASSERT(!coro->is_null(), "Invalid coroutine.");
    TIRO_DEBUG_ASSERT(
        coro->state() == CoroutineState::Waiting, "Coroutine must be in waiting state.");

    coro->state(CoroutineState::Ready);
    schedule_coroutine(coro);
}

bool Context::add_module(Handle<Module> module) {
    TIRO_CHECK(!module->name().is_null(), "Module must have a valid name.");

    if (modules_.value().contains(module->name())) {
        return false;
    }

    Scope sc(*this);
    Local name = sc.local(module->name());
    name = get_interned_string(name);
    modules_.value().set(*this, name, module);
    return true;
}

bool Context::find_module(Handle<String> name, OutHandle<Module> module) {
    if (auto opt = modules_.value().get(*name)) {
        module.set(static_cast<Module>(*opt));
        return true;
    }
    return false;
}

Value Context::get_integer(i64 value) {
    if (SmallInteger::fits(value)) {
        return SmallInteger::make(value);
    }
    return Integer::make(*this, value);
}

String Context::get_interned_string(Handle<String> str) {
    TIRO_CHECK(!str->is_null(), "String must not be null.");

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

    {
        Local existing_string = sc.local();
        Local existing_value = sc.local();
        if (auto found = interned_strings_.value().find(*str)) {
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
    interned_strings_.value().set(*this, str, symbol);
    str->interned(true);

    if (assoc_symbol) {
        assoc_symbol.handle().set(symbol);
    }
}

void Context::register_global(Value* slot) {
    TIRO_DEBUG_ASSERT(slot, "Slot pointer must not be null.");

    [[maybe_unused]] auto result = global_slots_.insert(slot);
    TIRO_DEBUG_ASSERT(result.second, "Slot pointer was already inserted previously.");
}

void Context::unregister_global(Value* slot) {
    [[maybe_unused]] size_t removed = global_slots_.erase(slot);
    TIRO_DEBUG_ASSERT(removed > 0, "Slot pointer was not removed.");
}

} // namespace tiro::vm
