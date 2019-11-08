#include "hammer/vm/objects/string.hpp"

#include "hammer/vm/context.hpp"

#include "hammer/vm/objects/string.ipp"

namespace hammer::vm {

String String::make(Context& ctx, std::string_view str) {
    size_t total_size = variable_allocation<Data, char>(str.size());
    Data* data = ctx.heap().create_varsize<Data>(total_size, str);
    return String(from_heap(data));
}

const char* String::data() const noexcept {
    return access_heap<Data>()->data;
}

size_t String::size() const noexcept {
    return access_heap<Data>()->size;
}

size_t String::hash() const noexcept {
    // TODO not thread safe
    size_t& hash = access_heap<Data>()->hash;
    if (hash == 0) {
        hash = std::hash<std::string_view>()(view());
        hash = hash == 0 ? 1 : hash;
    }
    return hash;
}

bool String::equal(String other) const {
    HAMMER_ASSERT(!other.is_null(), "The other string must not be null.");

    // TODO fast path for interned strings.
    return view() == other.view();
}

} // namespace hammer::vm
