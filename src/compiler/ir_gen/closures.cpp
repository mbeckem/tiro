#include "compiler/ir_gen/closures.hpp"

#include "common/math.hpp"

namespace tiro::ir {

void ClosureEnv::format(FormatStream& stream) const {
    stream.format("ClosureEnv(parent: {}, size: {})", parent_, size_);
}

void format(const ClosureEnvLocation& loc, FormatStream& stream) {
    stream.format("ClosureEnvLocation(env: {}, index: {})", loc.env, loc.index);
}

ClosureEnvCollection::ClosureEnvCollection() {}

ClosureEnvCollection::~ClosureEnvCollection() {}

ClosureEnvId ClosureEnvCollection::make(const ClosureEnv& env) {
    return envs_.push_back(env);
}

NotNull<IndexMapPtr<ClosureEnv>> ClosureEnvCollection::operator[](ClosureEnvId id) {
    check_id(id);
    return TIRO_NN(envs_.ptr_to(id));
}

NotNull<IndexMapPtr<const ClosureEnv>> ClosureEnvCollection::operator[](ClosureEnvId id) const {
    check_id(id);
    return TIRO_NN(envs_.ptr_to(id));
}

void ClosureEnvCollection::write_location(SymbolId symbol, const ClosureEnvLocation& loc) {
    TIRO_DEBUG_ASSERT(
        locs_.find(symbol) == locs_.end(), "Symbol is already associated with a location.");
    TIRO_DEBUG_ASSERT(loc.env, "The location must have a valid environment id.");
    TIRO_DEBUG_ASSERT((*this)[loc.env]->size() > loc.index,
        "The location's index is out of bounds for "
        "the given environment.");

    locs_.emplace(symbol, loc);
}

std::optional<ClosureEnvLocation> ClosureEnvCollection::read_location(SymbolId symbol) const {
    if (auto pos = locs_.find(symbol); pos != locs_.end())
        return pos->second;
    return {};
}

void ClosureEnvCollection::check_id([[maybe_unused]] ClosureEnvId id) const {
    TIRO_DEBUG_ASSERT(id, "ClosureEnvId is not valid.");
    TIRO_DEBUG_ASSERT(id.value() < envs_.size(),
        "ClosureEnvId's value is out of bounds (does the id belong to a "
        "different collection?).");
}

void dump_envs(const ClosureEnvCollection& envs, const SymbolTable& symbols,
    const StringTable& strings, FormatStream& stream) {
    stream.format("FunctionEnvironments:\n");

    {
        stream.format("  Environments:\n");

        const size_t env_count = envs.environment_count();
        const size_t max_index_length = fmt::formatted_size(
            "{}", env_count == 0 ? 0 : env_count - 1);

        size_t index = 0;
        for (const auto& env : envs.environments()) {
            stream.format("    {index:>{width}}: {value}\n", fmt::arg("index", index),
                fmt::arg("width", max_index_length), fmt::arg("value", env));
            ++index;
        }
    }

    {
        stream.format("  Locations:\n");
        for (const auto& loc : envs.locations()) {
            auto symbol = symbols[loc.first];
            stream.format("    {}@{} -> {}\n", strings.dump(symbol->name()), loc.first, loc.second);
        }
    }
}

} // namespace tiro::ir
