#include "tiro/mir/closures.hpp"

#include "tiro/core/math.hpp"

namespace tiro::compiler::mir_transform {

void ClosureEnv::format(FormatStream& stream) const {
    stream.format("ClosureEnv(parent: {}, size: {})", parent_, size_);
}

void format(const ClosureEnvLocation& loc, FormatStream& stream) {
    stream.format("ClosureEnvLocation(env: {}, index: {})", loc.env, loc.index);
}

ClosureEnvCollection::ClosureEnvCollection() {}

ClosureEnvCollection::~ClosureEnvCollection() {}

ClosureEnvID ClosureEnvCollection::make(const ClosureEnv& env) {
    const u32 id_value = checked_cast<u32>(envs_.size());
    envs_.push_back(env);
    return ClosureEnvID(id_value);
}

VecPtr<ClosureEnv> ClosureEnvCollection::operator[](ClosureEnvID id) {
    check_id(id);
    return VecPtr(envs_, id.value());
}

VecPtr<const ClosureEnv> ClosureEnvCollection::
operator[](ClosureEnvID id) const {
    check_id(id);
    return VecPtr(envs_, id.value());
}

void ClosureEnvCollection::write_location(
    NotNull<Symbol*> symbol, const ClosureEnvLocation& loc) {
    TIRO_ASSERT(locs_.find(symbol) == locs_.end(),
        "Symbol is already associated with a location.");
    TIRO_ASSERT(loc.env, "The location must have a valid environment id.");
    TIRO_ASSERT((*this)[loc.env]->size() > loc.index,
        "The location's index is out of bounds for "
        "the given environment.");

    locs_.emplace(symbol, loc);
}

std::optional<ClosureEnvLocation>
ClosureEnvCollection::read_location(NotNull<Symbol*> symbol) const {
    if (auto pos = locs_.find(symbol); pos != locs_.end())
        return pos->second;
    return {};
}

void ClosureEnvCollection::check_id(ClosureEnvID id) const {
    TIRO_ASSERT(id, "ClosureEnvID is not valid.");
    TIRO_ASSERT(id.value() < envs_.size(),
        "ClosureEnvID's value is out of bounds (does the id belong to a "
        "different collection?).");
}

void dump_envs(const ClosureEnvCollection& envs, const StringTable& strings,
    FormatStream& stream) {
    stream.format("FunctionEnvironments:\n");

    {
        stream.format("  Environments:\n");

        const size_t env_count = envs.environment_count();
        const size_t max_index_length = fmt::formatted_size(
            "{}", env_count == 0 ? 0 : env_count - 1);

        size_t index = 0;
        for (const auto& env : envs.environments()) {
            stream.format("    {index:>{width}}: {value}\n",
                fmt::arg("index", index), fmt::arg("width", max_index_length),
                fmt::arg("value", env));
            ++index;
        }
    }

    {
        stream.format("  Locations:\n");
        for (const auto& loc : envs.locations()) {
            stream.format("    {}@{} -> {}\n", strings.dump(loc.first->name()),
                (void*) loc.first, loc.second);
        }
    }
}

} // namespace tiro::compiler::mir_transform
