#include "hammer/vm/context.hpp"

#include "hammer/compiler/output.hpp"
#include "hammer/compiler/string_table.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/vm/objects/classes.hpp"
#include "hammer/vm/objects/modules.hpp"
#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/objects/primitives.hpp"
#include "hammer/vm/objects/strings.hpp"

#include "hammer/vm/context.ipp"

#include <boost/asio/executor_work_guard.hpp>

#include <cmath>

namespace hammer::vm {

Context::Context()
    : heap_(this) {
    true_ = Boolean::make(*this, true);
    false_ = Boolean::make(*this, false);
    undefined_ = Undefined::make(*this);
    stop_iteration_ = SpecialValue::make(*this, "STOP_ITERATION");
    interned_strings_ = HashTable::make(*this);
    modules_ = HashTable::make(*this);

    interpreter_.init(*this);
    types_.init(*this);
}

Context::~Context() {}

Value Context::run(Handle<Value> function) {
    if (running_) {
        HAMMER_ERROR("Already running, nested calls are not allowed.");
    }

    running_ = true;
    ScopeExit reset = [&] {
        running_ = false;
        io_context_.restart();
    };

    // Keep the main loop running until we manually break from it.
    auto work = boost::asio::make_work_guard(io_context_);

    // Create a new coroutine to execute the function.
    Root coro(*this, interpreter_.create_coroutine(function));
    schedule_coroutine(coro);

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
    HAMMER_ASSERT(!coro->is_null(), "Invalid coroutine.");
    HAMMER_ASSERT(
        is_runnable(coro->state()), "Invalid coroutine state: cannot be run.");
    HAMMER_ASSERT(
        !coro->next_ready(), "Runnable coroutine must not be linked.");

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

/*
 * This function is called by the runtime when an async (native) function resumes 
 * after yielding. We are either being invoked from another thread (via post() on the io context)
 * or from this thread (from an async callback using dispatch()).
 * In any event, we will be run by the loop in Context::run().
 */
void Context::resume_coroutine(Handle<Coroutine> coro) {
    HAMMER_ASSERT(!coro->is_null(), "Invalid coroutine.");
    HAMMER_ASSERT(coro->state() == CoroutineState::Waiting,
        "Coroutine must be in waiting state.");

    coro->state(CoroutineState::Ready);
    schedule_coroutine(coro);
}

bool Context::add_module(Handle<Module> module) {
    HAMMER_CHECK(!module->is_null(), "Module must not be null.");
    HAMMER_CHECK(!module->name().is_null(), "Module must have a valid name.");

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
    HAMMER_CHECK(!str->is_null(), "String must not be null.");

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
            HAMMER_ASSERT(
                existing_string->is<String>(), "Key must be a string.");
            HAMMER_ASSERT(existing_string->as<String>().interned(),
                "Existing string must have been interned.");
            HAMMER_ASSERT(
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

void Context::register_global(Value* slot) {
    HAMMER_ASSERT(slot, "Slot pointer must not be null.");

    [[maybe_unused]] auto result = global_slots_.insert(slot);
    HAMMER_ASSERT(
        result.second, "Slot pointer was already inserted previously.");
}

void Context::unregister_global(Value* slot) {
    [[maybe_unused]] size_t removed = global_slots_.erase(slot);
    HAMMER_ASSERT(removed > 0, "Slot pointer was not removed.");
}

} // namespace hammer::vm
