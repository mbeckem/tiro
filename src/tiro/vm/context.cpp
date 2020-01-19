#include "tiro/vm/context.hpp"

#include "tiro/compiler/output.hpp"
#include "tiro/compiler/string_table.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/vm/objects/classes.hpp"
#include "tiro/vm/objects/modules.hpp"
#include "tiro/vm/objects/primitives.hpp"
#include "tiro/vm/objects/strings.hpp"

#include "tiro/vm/context.ipp"

#include <asio/executor_work_guard.hpp>

#include <chrono>
#include <cmath>

namespace tiro::vm {

static i64 timestamp() {
    using namespace std::chrono;

    auto now = time_point_cast<milliseconds>(steady_clock::now());
    return static_cast<i64>(now.time_since_epoch().count());
}

Context::Context()
    : heap_(this)
    , startup_time_(timestamp()) {
    true_ = Boolean::make(*this, true);
    false_ = Boolean::make(*this, false);
    undefined_ = Undefined::make(*this);
    interned_strings_ = HashTable::make(*this);
    modules_ = HashTable::make(*this);
    stop_iteration_ = get_symbol("STOP_ITERATION");

    interpreter_.init(*this);
    types_.init(*this);
}

Context::~Context() {}

Value Context::run(Handle<Value> function) {
    if (running_) {
        TIRO_ERROR("Already running, nested calls are not allowed.");
    }

    running_ = true;
    ScopeExit reset = [&] {
        running_ = false;
        io_context_.restart();
    };

    // Keep the main loop running until we manually break from it.
    auto work = asio::make_work_guard(io_context_);

    // Create a new coroutine to execute the function.
    Root coro(*this, make_coroutine(function));

    // Run until the coroutine completes. This will block and execute
    // async handlers as soon as they arrive. Note that we're probably
    // checking coro->state() too often. We could also track the root coroutine
    // and reset "work" when we're done.
    while (coro->state() != CoroutineState::Done) {
        io_context_.run_one();
    }
    return coro->result();
}

void Context::execute_coroutines() {
    if (coroutines_executing_) {
        return;
    }

    coroutines_executing_ = true;
    ScopeExit reset = [&] { coroutines_executing_ = false; };

    loop_timestamp_ = timestamp() - startup_time_;

    Root<Coroutine> coro(*this);
    while (1) {
        coro.set(dequeue_coroutine());
        if (coro->is_null()) {
            break;
        }
        interpreter_.run(coro);
    }
}

void Context::schedule_coroutine(Handle<Coroutine> coro) {
    TIRO_ASSERT(!coro->is_null(), "Invalid coroutine.");
    TIRO_ASSERT(
        is_runnable(coro->state()), "Invalid coroutine state: cannot be run.");
    TIRO_ASSERT(!coro->next_ready(), "Runnable coroutine must not be linked.");

    if (last_ready_) {
        last_ready_.next_ready(coro.get());
        last_ready_ = coro.get();
    } else {
        first_ready_ = last_ready_ = coro.get();
    }

    execute_coroutines();
}

Coroutine Context::dequeue_coroutine() {
    if (!first_ready_) {
        return Coroutine();
    }

    Coroutine next = first_ready_;

    first_ready_ = first_ready_.next_ready();
    if (!first_ready_) {
        last_ready_ = Coroutine();
    }

    return next;
}

/// This function is called by the runtime when an async (native) function resumes
/// after yielding. We are either being invoked from another thread (via post() on the io context)
/// or from this thread (from an async callback using dispatch()).
/// In any event, we will be run by the loop in Context::run().
void Context::resume_coroutine(Handle<Coroutine> coro) {
    TIRO_ASSERT(!coro->is_null(), "Invalid coroutine.");
    TIRO_ASSERT(coro->state() == CoroutineState::Waiting,
        "Coroutine must be in waiting state.");

    coro->state(CoroutineState::Ready);
    schedule_coroutine(coro);
}

bool Context::add_module(Handle<Module> module) {
    TIRO_CHECK(!module->is_null(), "Module must not be null.");
    TIRO_CHECK(!module->name().is_null(), "Module must have a valid name.");

    if (modules_.contains(module->name())) {
        return false;
    }

    Root<String> name(*this, module->name());
    name.set(intern_string(name));

    modules_.set(*this, name.handle(), module);
    return true;
}

bool Context::find_module(Handle<String> name, MutableHandle<Module> module) {
    if (auto opt = modules_.get(name)) {
        module.set(opt->as_strict<Module>());
        return true;
    }
    return false;
}

String Context::intern_string(Handle<String> str) {
    TIRO_CHECK(!str->is_null(), "String must not be null.");

    if (str->interned())
        return str;

    Root interned(*this, str.get());
    intern_impl(interned.mut_handle(), {});
    return interned;
}

Value Context::get_integer(i64 value) {
    if (SmallInteger::fits(value)) {
        return SmallInteger::make(value);
    }
    return Integer::make(*this, value);
}

String Context::get_interned_string(std::string_view view) {
    // Improvement: we can avoid constructing the temporary string by introducing
    // a find_equivalent(hash, compare, ...) function to the table. Care must be taken
    // to use the same hash function in that case.
    Root str(*this, String::make(*this, view));
    return intern_string(str);
}

Symbol Context::get_symbol(Handle<String> str) {
    Root<String> interned_str(*this, str);
    Root<Symbol> symbol(*this);

    intern_impl(interned_str.mut_handle(), symbol.mut_handle());
    return symbol;
}

Symbol Context::get_symbol(std::string_view value) {
    Root<String> interned(*this, get_interned_string(value));
    return get_symbol(interned.handle());
}

void Context::intern_impl(MutableHandle<String> str,
    std::optional<MutableHandle<Symbol>> assoc_symbol) {

    {
        Root<Value> existing_string(*this);
        Root<Value> existing_value(*this);
        if (interned_strings_.find(str, existing_string.mut_handle(),
                existing_value.mut_handle())) {
            TIRO_ASSERT(existing_string->is<String>(), "Key must be a string.");
            TIRO_ASSERT(existing_string->as<String>().interned(),
                "Existing string must have been interned.");
            TIRO_ASSERT(
                existing_value->is<Symbol>(), "Value must be a symbol.");

            if (assoc_symbol) {
                assoc_symbol->set(existing_value->as<Symbol>());
            }
            return str.set(existing_string->as<String>());
        }
    }

    // TODO: I'm being lazy here, create a symbol right away. This could be delayed only
    // for those instances where a symbol is actually needed.
    Root symbol(*this, Symbol::make(*this, str));
    interned_strings_.set(*this, str, symbol.handle());
    str->interned(true);

    if (assoc_symbol) {
        assoc_symbol->set(symbol);
    }
}

Coroutine Context::make_coroutine(Handle<Value> func) {
    Root coro(*this, interpreter_.make_coroutine(func));
    schedule_coroutine(coro);
    return coro;
}

void Context::register_global(Value* slot) {
    TIRO_ASSERT(slot, "Slot pointer must not be null.");

    [[maybe_unused]] auto result = global_slots_.insert(slot);
    TIRO_ASSERT(result.second, "Slot pointer was already inserted previously.");
}

void Context::unregister_global(Value* slot) {
    [[maybe_unused]] size_t removed = global_slots_.erase(slot);
    TIRO_ASSERT(removed > 0, "Slot pointer was not removed.");
}

} // namespace tiro::vm
