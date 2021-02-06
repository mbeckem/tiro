#ifndef TIRO_COMPILER_IR_RECORD_HPP
#define TIRO_COMPILER_IR_RECORD_HPP

#include "common/defs.hpp"
#include "common/text/string_table.hpp"
#include "compiler/ir/entities.hpp"
#include "compiler/ir/fwd.hpp"

#include <absl/container/inlined_vector.h>

namespace tiro::ir {

/// A record represents an object that maps keys (symbols) to values.
/// In this instance, the keys are known at compile time.
/// Record entries are currently represented as a simple vector because
/// because the semantic analysis pass already checks that keys are unique.
class Record final {
public:
    using Entry = std::pair<InternedString, InstId>;
    using Storage = absl::InlinedVector<Entry, 2>;

    Record();
    ~Record();

    Record(Record&&) noexcept = default;
    Record& operator=(Record&&) = default;

    Record(const Record&) = delete;
    Record& operator=(const Record&) = delete;

    auto begin() const { return entries_.begin(); }
    auto end() const { return entries_.end(); }

    size_t size() const { return entries_.size(); }

    void insert(InternedString name, InstId value);

private:
    Storage entries_;
};

} // namespace tiro::ir

#endif // TIRO_COMPILER_IR_RECORD_HPP
