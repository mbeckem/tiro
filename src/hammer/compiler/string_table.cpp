#include "hammer/compiler/string_table.hpp"

#include "hammer/core/defs.hpp"
#include "hammer/core/math.hpp"
#include "hammer/core/scope.hpp"

#include <new>

namespace hammer::compiler {

StringTable::StringTable() {}

StringTable::~StringTable() {}

InternedString StringTable::insert(std::string_view str) {
    if (auto pos = strings_by_content_.find(str);
        pos != strings_by_content_.end()) {
        return InternedString(pos->second);
    }

    if (HAMMER_UNLIKELY(next_index_ == 0)) {
        HAMMER_ERROR("Too many interned strings.");
    }

    size_t total_size = 0;
    if (HAMMER_UNLIKELY(!checked_add(sizeof(Storage), str.size(), total_size)))
        HAMMER_ERROR("Allocation size overflow.");

    void* memory = arena_.allocate(total_size, alignof(Storage));
    Storage* entry = new (memory) Storage;
    entry->size = str.size();
    std::copy(str.begin(), str.end(), entry->data);

    const u32 index = next_index_;
    {
        [[maybe_unused]] auto [pos, inserted] = strings_by_index_.emplace(
            index, std::move(entry));
        HAMMER_ASSERT(inserted, "Unique value was not inserted.");
    }
    ScopeExit guard = [&] { strings_by_index_.erase(index); };

    strings_by_content_.emplace(view(entry), index);

    guard.disable();
    next_index_ += 1;
    total_bytes_ += str.size();
    return InternedString(index);
}

std::optional<InternedString> StringTable::find(std::string_view str) const {
    if (auto pos = strings_by_content_.find(str);
        pos != strings_by_content_.end()) {
        return InternedString(pos->second);
    }
    return {};
}

std::string_view StringTable::value(const InternedString& str) const {
    HAMMER_CHECK(str, "Invalid interned string instance.");

    auto pos = strings_by_index_.find(str.value());
    HAMMER_ASSERT(pos != strings_by_index_.end(),
        "Interned string index not found in string table.");
    return view(pos->second);
}

} // namespace hammer::compiler
