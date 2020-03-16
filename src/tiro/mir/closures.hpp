#ifndef TIRO_MIR_CLOSURES_HPP
#define TIRO_MIR_CLOSURES_HPP

#include "tiro/core/format.hpp"
#include "tiro/core/id_type.hpp"
#include "tiro/core/index_map.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/core/ref_counted.hpp"
#include "tiro/core/vec_ptr.hpp"
#include "tiro/semantics/symbol_table.hpp"
#include "tiro/syntax/ast.hpp"

#include <optional>

namespace tiro::compiler::mir_transform {

TIRO_DEFINE_ID(ClosureEnvID, u32);

/// Represents a closure environment.
///
/// Closure environments store captured variables and form a tree.
class ClosureEnv final {
public:
    ClosureEnv(u32 size)
        : ClosureEnv(ClosureEnvID(), size) {}

    ClosureEnv(ClosureEnvID parent, u32 size)
        : parent_(parent)
        , size_(size) {}

    ClosureEnvID parent() const { return parent_; }
    u32 size() const { return size_; }

    void format(FormatStream& stream) const;

public:
    ClosureEnvID parent_;
    u32 size_;
};

/// Represents the location of a symbol (variable) within a closure environment.
struct ClosureEnvLocation {
    /// The closure environment that contains the symbol.
    ClosureEnvID env;

    /// The index of the symbol in the environment.
    u32 index;

    ClosureEnvLocation(ClosureEnvID env_, u32 index_)
        : env(env_)
        , index(index_) {}
};

void format(const ClosureEnvLocation& loc, FormatStream& stream);

/// Maintains a collection of closure environments. An instance of this class
/// is created for every top level function (on demand) and passed to all children (direct or
/// indirect) of that function.
///
/// Improvement: this approach makes memory management simple (shared between all children) but
/// makes compiling in parallel very hard (shared state!). By keeping all function compliations
/// independent of each other, we could parllelize them easily.
class ClosureEnvCollection final : public RefCounted {
public:
    ClosureEnvCollection();
    ~ClosureEnvCollection();

    ClosureEnvCollection(const ClosureEnvCollection&) = delete;
    ClosureEnvCollection& operator=(const ClosureEnvCollection&) = delete;

    ClosureEnvID make(const ClosureEnv& env);
    NotNull<VecPtr<ClosureEnv>> operator[](ClosureEnvID id);
    NotNull<VecPtr<const ClosureEnv>> operator[](ClosureEnvID id) const;

    /// Associates the given symbol with its location within the closure env collection.
    /// \pre `symbol` has not been inserted already.
    /// \pre The location must be valid.
    void write_location(NotNull<Symbol*> symbol, const ClosureEnvLocation& loc);

    /// Returns the location of the given symbol (previously registered via write_location()).
    std::optional<ClosureEnvLocation>
    read_location(NotNull<Symbol*> symbol) const;

    auto environments() const { return IterRange(envs_.begin(), envs_.end()); }
    size_t environment_count() const { return envs_.size(); }

    auto locations() const { return IterRange(locs_.begin(), locs_.end()); }
    size_t location_count() const { return locs_.size(); }

private:
    void check_id(ClosureEnvID id) const;

private:
    IndexMap<ClosureEnv, IDMapper<ClosureEnvID>> envs_;
    std::unordered_map<Symbol*, ClosureEnvLocation> locs_; // TODO faster table
};

void dump_envs(const ClosureEnvCollection& envs, const StringTable& strings,
    FormatStream& stream);

} // namespace tiro::compiler::mir_transform

TIRO_ENABLE_MEMBER_FORMAT(tiro::compiler::mir_transform::ClosureEnv)
TIRO_ENABLE_FREE_FORMAT(tiro::compiler::mir_transform::ClosureEnvLocation)

#endif // TIRO_MIR_CLOSURES_HPP
