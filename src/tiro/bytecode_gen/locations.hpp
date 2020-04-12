#ifndef TIRO_BYTECODE_GEN_LOCATIONS_HPP
#define TIRO_BYTECODE_GEN_LOCATIONS_HPP

#include "tiro/bytecode_gen/fwd.hpp"

#include "tiro/bytecode/instruction.hpp"
#include "tiro/core/index_map.hpp"
#include "tiro/ir/types.hpp"

#include <optional>

namespace tiro {

/* [[[cog
    import unions
    import bytecode_gen
    unions.define_type(bytecode_gen.CompiledLocationType)
]]] */
/// Represents the type of a compiled location.
enum class CompiledLocationType : u8 {
    Value,
    Method,
};

std::string_view to_string(CompiledLocationType type);
// [[[end]]]

/* [[[cog
    import unions
    import bytecode_gen
    unions.define_type(bytecode_gen.CompiledLocation)
]]] */
/// Represents a location that has been assigned to a ir value. Usually locations
/// are only concerned with single local (at bytecode level). Some special cases
/// exist where a virtual ir value is mapped to multiple physical locals.
class CompiledLocation final {
public:
    /// Represents a single value. This is the usual case.
    using Value = BytecodeRegister;

    /// Represents a method value. Two locals are needed to represent a method:
    /// One for the object instance and one for the actual method function value.
    struct Method final {
        /// The 'this' argument of the method call.
        BytecodeRegister instance;

        /// The function value.
        BytecodeRegister function;

        Method(const BytecodeRegister& instance_,
            const BytecodeRegister& function_)
            : instance(instance_)
            , function(function_) {}
    };

    static CompiledLocation make_value(const Value& value);
    static CompiledLocation make_method(
        const BytecodeRegister& instance, const BytecodeRegister& function);

    CompiledLocation(const Value& value);
    CompiledLocation(const Method& method);

    CompiledLocationType type() const noexcept { return type_; }

    const Value& as_value() const;
    const Method& as_method() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto)
    visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    CompiledLocationType type_;
    union {
        Value value_;
        Method method_;
    };
};

bool operator==(const CompiledLocation& lhs, const CompiledLocation& rhs);
bool operator!=(const CompiledLocation& lhs, const CompiledLocation& rhs);
// [[[end]]]

u32 physical_locals_count(const CompiledLocation& loc);

void visit_physical_locals(
    const CompiledLocation& loc, FunctionRef<void(BytecodeRegister)> callback);

/// Represents a copy between two registers. Typically used for the implementation
/// of phi operand passing.
struct RegisterCopy {
    BytecodeRegister src;
    BytecodeRegister dest;
};

/// Maps virtual locals (from the ir layer) to physical locals (at the bytecode layer).
class CompiledLocations final {
public:
    CompiledLocations();
    explicit CompiledLocations(size_t total_blocks, size_t total_ssa_locals);

    CompiledLocations(CompiledLocations&&) noexcept = default;
    CompiledLocations& operator=(CompiledLocations&&) noexcept = default;

    /// Returns the required number of physical local variable slots.
    u32 total_registers() const { return total_registers_; }

    /// Sets the required number of physical local variable slots.
    void total_registers(u32 total) { total_registers_ = total; }

    /// Returns true if the given ssa_local was assigned a physical location.
    bool contains(LocalID ssa_local) const;

    /// Assigns the physical location to the given ssa_local.
    void set(LocalID ssa_local, const CompiledLocation& location);

    /// Returns the physical location of the given ssa_local.
    /// \pre ssa_local must have been assigned a location.
    CompiledLocation get(LocalID ssa_local) const;

    /// Returns the physical location of the given ssa local, or an empty
    /// optional if the ssa local has not been assigned a location.
    std::optional<CompiledLocation> try_get(LocalID ssa_local) const;

    /// Returns true if the block was a sequence of phi argument copies.
    bool has_phi_copies(BlockID block) const;

    /// Assigns the given phi argument copies to the given block.
    void set_phi_copies(BlockID block, std::vector<RegisterCopy> copies);

    /// Returns the phi argument copies for the given block.
    const std::vector<RegisterCopy>& get_phi_copies(BlockID block) const;

private:
    // Storage locations of ssa locals.
    IndexMap<std::optional<CompiledLocation>, IDMapper<LocalID>> locs_;

    // Spare storage locations for the passing of phi arguments. Only assigned
    // to blocks that pass phi arguments to successors.
    IndexMap<std::vector<RegisterCopy>, IDMapper<BlockID>> copies_;

    // Total number of storage locations used.
    u32 total_registers_ = 0;
};

/// Assigns a physical location to ssa locals in the given function.
/// Used when compiling a function from IR to bytecode.
/// Exposed for testing.
CompiledLocations allocate_locations(const Function& func);

/* [[[cog
    import unions
    import bytecode_gen
    unions.define_inlines(bytecode_gen.CompiledLocation)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto)
CompiledLocation::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case CompiledLocationType::Value:
        return vis.visit_value(self.value_, std::forward<Args>(args)...);
    case CompiledLocationType::Method:
        return vis.visit_method(self.method_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid CompiledLocation type.");
}
// [[[end]]]

} // namespace tiro

#endif // TIRO_BYTECODE_GEN_LOCATIONS_HPP
