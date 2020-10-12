#ifndef TIRO_COMPILER_IR_GEN_CLOSURES_HPP
#define TIRO_COMPILER_IR_GEN_CLOSURES_HPP

#include "common/adt/index_map.hpp"
#include "common/adt/vec_ptr.hpp"
#include "common/format.hpp"
#include "common/hash.hpp"
#include "common/id_type.hpp"
#include "common/memory/ref_counted.hpp"
#include "common/not_null.hpp"
#include "compiler/semantics/symbol_table.hpp"

#include "absl/container/flat_hash_map.h"

#include <optional>

namespace tiro {

TIRO_DEFINE_ID(ClosureEnvId, u32);

/// Represents a closure environment.
///
/// Closure environments store captured variables and form a tree.
class ClosureEnv final {
public:
    ClosureEnv(u32 size)
        : ClosureEnv(ClosureEnvId(), size) {}

    ClosureEnv(ClosureEnvId parent, u32 size)
        : parent_(parent)
        , size_(size) {}

    ClosureEnvId parent() const { return parent_; }
    u32 size() const { return size_; }

    void format(FormatStream& stream) const;

public:
    ClosureEnvId parent_;
    u32 size_;
};

/// Represents the location of a symbol (variable) within a closure environment.
struct ClosureEnvLocation {
    /// The closure environment that contains the symbol.
    ClosureEnvId env;

    /// The index of the symbol in the environment.
    u32 index;

    ClosureEnvLocation(ClosureEnvId env_, u32 index_)
        : env(env_)
        , index(index_) {}
};

void format(const ClosureEnvLocation& loc, FormatStream& stream);

/// Maintains a collection of closure environments. An instance of this class
/// is created for every top level function (on demand) and passed to all children (direct or
/// indirect) of that function.
///
/// Improvement: this approach makes memory management simple (shared between all children) but
/// makes compiling in parallel very hard (shared state!). By keeping all function compilations
/// independent of each other, we could parallelize them easily.
class ClosureEnvCollection final : public RefCounted {
public:
    ClosureEnvCollection();
    ~ClosureEnvCollection();

    ClosureEnvCollection(const ClosureEnvCollection&) = delete;
    ClosureEnvCollection& operator=(const ClosureEnvCollection&) = delete;

    ClosureEnvId make(const ClosureEnv& env);
    NotNull<IndexMapPtr<ClosureEnv>> operator[](ClosureEnvId id);
    NotNull<IndexMapPtr<const ClosureEnv>> operator[](ClosureEnvId id) const;

    /// Associates the given symbol with its location within the closure env collection.
    /// \pre `symbol` has not been inserted already.
    /// \pre The location must be valid.
    void write_location(SymbolId symbol, const ClosureEnvLocation& loc);

    /// Returns the location of the given symbol (previously registered via write_location()).
    std::optional<ClosureEnvLocation> read_location(SymbolId symbol) const;

    auto environments() const { return IterRange(envs_.begin(), envs_.end()); }
    size_t environment_count() const { return envs_.size(); }

    auto locations() const { return IterRange(locs_.begin(), locs_.end()); }
    size_t location_count() const { return locs_.size(); }

private:
    void check_id(ClosureEnvId id) const;

private:
    IndexMap<ClosureEnv, IdMapper<ClosureEnvId>> envs_;
    absl::flat_hash_map<SymbolId, ClosureEnvLocation, UseHasher> locs_;
};

void dump_envs(const ClosureEnvCollection& envs, const SymbolTable& symbols,
    const StringTable& strings, FormatStream& stream);

} // namespace tiro

TIRO_ENABLE_MEMBER_FORMAT(tiro::ClosureEnv)
TIRO_ENABLE_FREE_FORMAT(tiro::ClosureEnvLocation)

#endif // TIRO_COMPILER_IR_GEN_CLOSURES_HPP
