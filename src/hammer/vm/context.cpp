#include "hammer/vm/context.hpp"

#include "hammer/compiler/output.hpp"
#include "hammer/compiler/string_table.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/objects/small_integer.hpp"
#include "hammer/vm/objects/string.hpp"

#include "hammer/vm/context.ipp"

#include <cmath>

namespace hammer::vm {

Context::Context() {
    true_ = Boolean::make(*this, true);
    false_ = Boolean::make(*this, false);
    undefined_ = Undefined::make(*this);
    stop_iteration_ = SpecialValue::make(*this, "STOP_ITERATION");
    interned_strings_ = HashTable::make(*this);
    modules_ = HashTable::make(*this);

    types_.init(*this);
}

Context::~Context() {}

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
    if (value >= SmallInteger::min && value <= SmallInteger::max) {
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

Value Context::run(Handle<Function> fn) {
    Root coro(*this, interpreter_.create_coroutine(*this, fn));
    return interpreter_.run(*this, coro);
}

} // namespace hammer::vm
