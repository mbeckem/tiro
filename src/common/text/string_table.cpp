#include "common/text/string_table.hpp"

#include "common/defs.hpp"
#include "common/math.hpp"
#include "common/scope_guards.hpp"

#include <new>

namespace tiro {

StringTable::StringTable() {}

StringTable::~StringTable() {}

InternedString StringTable::insert(std::string_view str) {
    if (auto pos = strings_by_content_.find(str); pos != strings_by_content_.end()) {
        return InternedString(pos->second);
    }

    if (TIRO_UNLIKELY(next_index_ == 0)) {
        TIRO_ERROR("Too many interned strings.");
    }

    size_t total_size = 0;
    if (TIRO_UNLIKELY(!checked_add(sizeof(Storage), str.size(), total_size)))
        TIRO_ERROR("Allocation size overflow.");

    void* memory = arena_.allocate(total_size, alignof(Storage));
    NotNull<Storage*> entry(guaranteed_not_null, new (memory) Storage);
    entry->size = str.size();
    std::copy(str.begin(), str.end(), entry->data);

    const u32 index = next_index_;
    {
        [[maybe_unused]] auto [pos, inserted] = strings_by_index_.emplace(index, entry);
        TIRO_DEBUG_ASSERT(inserted, "Unique value was not inserted.");
        ScopeFailure guard = [&] { strings_by_index_.erase(index); };

        strings_by_content_.emplace(view(entry), index);
    }

    next_index_ += 1;
    total_bytes_ += str.size();
    return InternedString(index);
}

std::optional<InternedString> StringTable::find(std::string_view str) const {
    if (auto pos = strings_by_content_.find(str); pos != strings_by_content_.end()) {
        return InternedString(pos->second);
    }
    return {};
}

std::string_view StringTable::value(const InternedString& str) const {
    TIRO_CHECK(str, "Invalid interned string instance.");

    auto pos = strings_by_index_.find(str.value());
    TIRO_DEBUG_ASSERT(
        pos != strings_by_index_.end(), "Interned string index not found in string table.");
    return view(NotNull(guaranteed_not_null, pos->second));
}

std::string_view StringTable::value_or(const InternedString& str, std::string_view def) const {
    return str ? value(str) : def;
}

std::string_view StringTable::dump(const InternedString& str) const {
    return value_or(str, "N/A");
}

void InternedString::format(FormatStream& stream) const {
    if (!valid()) {
        stream.format("InternedString(invalid)");
    } else {
        stream.format("InternedString({})", value());
    }
}

} // namespace tiro
