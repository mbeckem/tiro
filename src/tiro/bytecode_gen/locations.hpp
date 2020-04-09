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
    using Value = CompiledLocalID;

    /// Represents a method value. Two locals are needed to represent a method:
    /// One for the object instance and one for the actual method function value.
    struct Method final {
        /// The 'this' argument of the method call.
        CompiledLocalID instance;

        /// The function value.
        CompiledLocalID function;

        Method(
            const CompiledLocalID& instance_, const CompiledLocalID& function_)
            : instance(instance_)
            , function(function_) {}
    };

    static CompiledLocation make_value(const Value& value);
    static CompiledLocation make_method(
        const CompiledLocalID& instance, const CompiledLocalID& function);

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
// [[[end]]]

u32 physical_locals_count(const CompiledLocation& loc);

void visit_physical_locals(
    const CompiledLocation& loc, FunctionRef<void(CompiledLocalID)> callback);

/// Maps virtual locals (from the ir layer) to physical locals (at the bytecode layer).
class CompiledLocations final {
public:
    CompiledLocations();
    explicit CompiledLocations(size_t total_ssa_locals);

    CompiledLocations(CompiledLocations&&) noexcept = default;
    CompiledLocations& operator=(CompiledLocations&&) noexcept = default;

    /// Returns the required number of physical local variable slots.
    u32 physical_locals() const { return physical_locals_; }

    /// Returns true if the given ssa_local was assigned a physical location.
    bool contains(LocalID ssa_local) const;

    /// Assigns the physical location to the given ssa_local.
    /// \pre ssa_local must not have been assigned a location yet.
    void set(LocalID ssa_local, const CompiledLocation& location);

    /// Returns the physical location of the given ssa_local.
    /// \pre ssa_local must have been assigned a location.
    CompiledLocation get(LocalID ssa_local) const;

    /// Returns the physical location of the given ssa local, or an empty
    /// optional if the ssa local has not been assigned a location.
    std::optional<CompiledLocation> try_get(LocalID ssa_local) const;

private:
    IndexMap<std::optional<CompiledLocation>, IDMapper<LocalID>> locs_;
    u32 physical_locals_ = 0;
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
