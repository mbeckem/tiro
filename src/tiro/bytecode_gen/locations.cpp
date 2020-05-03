#include "tiro/bytecode_gen/locations.hpp"

namespace tiro {

/* [[[cog
    from codegen.unions import implement
    from codegen.bytecode_gen import BytecodeLocationType
    implement(BytecodeLocationType)
]]] */
std::string_view to_string(BytecodeLocationType type) {
    switch (type) {
    case BytecodeLocationType::Value:
        return "Value";
    case BytecodeLocationType::Method:
        return "Method";
    }
    TIRO_UNREACHABLE("Invalid BytecodeLocationType.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.bytecode_gen import BytecodeLocation
    implement(BytecodeLocation)
]]] */
BytecodeLocation BytecodeLocation::make_value(const Value& value) {
    return value;
}

BytecodeLocation BytecodeLocation::make_method(
    const BytecodeRegister& instance, const BytecodeRegister& function) {
    return {Method{instance, function}};
}

BytecodeLocation::BytecodeLocation(Value value)
    : type_(BytecodeLocationType::Value)
    , value_(std::move(value)) {}

BytecodeLocation::BytecodeLocation(Method method)
    : type_(BytecodeLocationType::Method)
    , method_(std::move(method)) {}

const BytecodeLocation::Value& BytecodeLocation::as_value() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeLocationType::Value,
        "Bad member access on BytecodeLocation: not a Value.");
    return value_;
}

const BytecodeLocation::Method& BytecodeLocation::as_method() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeLocationType::Method,
        "Bad member access on BytecodeLocation: not a Method.");
    return method_;
}

bool operator==(const BytecodeLocation& lhs, const BytecodeLocation& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const BytecodeLocation& rhs;

        bool
        visit_value([[maybe_unused]] const BytecodeLocation::Value& value) {
            [[maybe_unused]] const auto& other = rhs.as_value();
            return value == other;
        }

        bool
        visit_method([[maybe_unused]] const BytecodeLocation::Method& method) {
            [[maybe_unused]] const auto& other = rhs.as_method();
            return method.instance == other.instance
                   && method.function == other.function;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const BytecodeLocation& lhs, const BytecodeLocation& rhs) {
    return !(lhs == rhs);
}
// [[[end]]]

u32 physical_locals_count(const BytecodeLocation& loc) {
    switch (loc.type()) {
    case BytecodeLocationType::Value:
        return 1;
    case BytecodeLocationType::Method:
        return 2;
    }
    TIRO_UNREACHABLE("Invalid location type.");
}

void visit_physical_locals(
    const BytecodeLocation& loc, FunctionRef<void(BytecodeRegister)> callback) {
    switch (loc.type()) {
    case BytecodeLocationType::Value:
        callback(loc.as_value());
        return;
    case BytecodeLocationType::Method:
        callback(loc.as_method().function);
        callback(loc.as_method().instance);
        return;
    }
    TIRO_UNREACHABLE("Invalid location type.");
}

BytecodeLocations::BytecodeLocations() {}

BytecodeLocations::BytecodeLocations(
    size_t total_blocks, size_t total_ssa_locals) {
    copies_.resize(total_blocks);
    locs_.resize(total_ssa_locals);
}

bool BytecodeLocations::contains(LocalId ssa_local) const {
    return locs_[ssa_local].has_value();
}

void BytecodeLocations::set(LocalId ssa_local, const BytecodeLocation& loc) {
    TIRO_DEBUG_ASSERT(ssa_local, "SSA local must be valid.");
    locs_[ssa_local] = loc;
}

BytecodeLocation BytecodeLocations::get(LocalId ssa_local) const {
    TIRO_DEBUG_ASSERT(contains(ssa_local),
        "SSA local must have been assigned a physical location.");
    return *locs_[ssa_local];
}

std::optional<BytecodeLocation>
BytecodeLocations::try_get(LocalId ssa_local) const {
    if (locs_.in_bounds(ssa_local))
        return locs_[ssa_local];
    return {};
}

bool BytecodeLocations::has_phi_copies(BlockId block) const {
    return copies_.in_bounds(block) && !copies_[block].empty();
}

void BytecodeLocations::set_phi_copies(
    BlockId block, std::vector<RegisterCopy> copies) {
    TIRO_DEBUG_ASSERT(block, "Block must be valid.");
    copies_[block] = std::move(copies);
}

const std::vector<RegisterCopy>&
BytecodeLocations::get_phi_copies(BlockId block) const {
    TIRO_DEBUG_ASSERT(block, "Block must be valid.");
    return copies_[block];
}

} // namespace tiro
