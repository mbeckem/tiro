#ifndef TIRO_COMPILER_IR_FUNCTION_HPP
#define TIRO_COMPILER_IR_FUNCTION_HPP

#include "common/adt/function_ref.hpp"
#include "common/adt/not_null.hpp"
#include "common/adt/span.hpp"
#include "common/defs.hpp"
#include "common/entities/entity_storage.hpp"
#include "common/entities/entity_storage_accessors.hpp"
#include "common/enum_flags.hpp"
#include "common/format.hpp"
#include "common/hash.hpp"
#include "common/ranges/iter_tools.hpp"
#include "common/text/string_table.hpp"
#include "compiler/ir/block.hpp"
#include "compiler/ir/entities.hpp"
#include "compiler/ir/fwd.hpp"
#include "compiler/ir/inst.hpp"
#include "compiler/ir/param.hpp"
#include "compiler/ir/record.hpp"
#include "compiler/ir/terminator.hpp"
#include "compiler/ir/value.hpp"
#include "compiler/semantics/symbol_table.hpp"

#include <absl/container/inlined_vector.h>

namespace tiro::ir {

enum class FunctionType : u8 {
    /// Function is a plain function and can be called and exported as-is.
    Normal,

    /// Function requires a closure environment to be called.
    Closure
};

std::string_view to_string(FunctionType type);

class Function final {
public:
    explicit Function(InternedString name, FunctionType type, StringTable& strings);

    ~Function();

    Function(Function&&) noexcept = default;
    Function& operator=(Function&&) noexcept = default;

    Function(const Function&) = delete;
    Function& operator=(const Function&) = delete;

    InternedString name() const noexcept { return name_; }
    FunctionType type() const noexcept { return type_; }

    StringTable& strings() const noexcept { return *strings_; }

    BlockId make(Block&& block);
    ParamId make(const Param& param);
    InstId make(Inst&& inst);
    LocalListId make(LocalList&& value_list);
    RecordId make(Record&& record);

    BlockId entry() const;
    BlockId body() const;
    BlockId exit() const;

    size_t block_count() const { return blocks_.size(); }
    size_t param_count() const { return params_.size(); }
    size_t inst_count() const { return insts_.size(); }
    size_t local_list_count() const { return local_lists_.size(); }

    TIRO_ENTITY_STORAGE_ACCESSORS(Block, BlockId, blocks_)
    TIRO_ENTITY_STORAGE_ACCESSORS(Param, ParamId, params_)
    TIRO_ENTITY_STORAGE_ACCESSORS(Inst, InstId, insts_)
    TIRO_ENTITY_STORAGE_ACCESSORS(LocalList, LocalListId, local_lists_)
    TIRO_ENTITY_STORAGE_ACCESSORS(Record, RecordId, records_)

    auto block_ids() const { return blocks_.keys(); }

private:
    NotNull<StringTable*> strings_;
    InternedString name_;
    FunctionType type_;
    BlockId entry_;
    BlockId body_;
    BlockId exit_;

    EntityStorage<Block, BlockId> blocks_;
    EntityStorage<Param, ParamId> params_;
    EntityStorage<Inst, InstId> insts_;
    EntityStorage<LocalList, LocalListId> local_lists_;
    EntityStorage<Record, RecordId> records_;
};

void dump_function(const Function& func, FormatStream& stream);

/// Represents a list of local variables, e.g. the arguments to a function call
/// or the items of an array.
class LocalList final {
public:
    using Storage = absl::InlinedVector<InstId, 8>;

    LocalList();
    LocalList(std::initializer_list<InstId> locals);
    LocalList(Storage&& locals);
    ~LocalList();

    LocalList(LocalList&&) noexcept = default;
    LocalList& operator=(LocalList&&) = default;

    // Prevent accidental copying.
    LocalList(const LocalList&) = delete;
    LocalList& operator=(const LocalList&) = delete;

    auto begin() const { return locals_.begin(); }
    auto end() const { return locals_.end(); }

    size_t size() const { return locals_.size(); }
    bool empty() const { return locals_.empty(); }

    InstId operator[](size_t index) const {
        TIRO_DEBUG_ASSERT(index < locals_.size(), "Index out of bounds.");
        return locals_[index];
    }

    InstId get(size_t index) const { return (*this)[index]; }

    void set(size_t index, InstId value) {
        TIRO_DEBUG_ASSERT(index < locals_.size(), "Index out of bounds.");
        locals_[index] = value;
    }

    void remove(size_t index, size_t count) {
        TIRO_DEBUG_ASSERT(
            index <= locals_.size() && count <= locals_.size() - index, "Range out of bounds.");
        const auto pos = locals_.begin() + index;
        locals_.erase(pos, pos + count);
    }

    void append(InstId local) { locals_.push_back(local); }

private:
    Storage locals_;
};

namespace dump_helpers {

template<typename T>
struct Dump {
    const Function& parent;
    T value;
};

struct Definition {
    InstId inst;
};

template<typename T>
Dump<T> dump(const Function& parent, const T& value) {
    return Dump<T>{parent, value};
}

inline Dump<const Value&> dump(const Function& parent, const Value& value) {
    return {parent, value};
}

inline Dump<const Terminator&> dump(const Function& parent, const Terminator& value) {
    return {parent, value};
}

void format(const Dump<BlockId>& d, FormatStream& stream);
void format(const Dump<Definition>& d, FormatStream& stream);
void format(const Dump<const Terminator&>& d, FormatStream& stream);
void format(const Dump<LValue>& d, FormatStream& stream);
void format(const Dump<Phi>& d, FormatStream& stream);
void format(const Dump<Constant>& d, FormatStream& stream);
void format(const Dump<Aggregate>& d, FormatStream& stream);
void format(const Dump<const Value&>& d, FormatStream& stream);
void format(const Dump<InstId>& d, FormatStream& stream);
void format(const Dump<LocalListId>& d, FormatStream& stream);
void format(const Dump<RecordId>& d, FormatStream& stream);

}; // namespace dump_helpers

} // namespace tiro::ir

TIRO_ENABLE_FREE_TO_STRING(tiro::ir::FunctionType)

TIRO_ENABLE_FREE_FORMAT(tiro::ir::dump_helpers::Dump<tiro::ir::BlockId>)
TIRO_ENABLE_FREE_FORMAT(tiro::ir::dump_helpers::Dump<tiro::ir::dump_helpers::Definition>)
TIRO_ENABLE_FREE_FORMAT(tiro::ir::dump_helpers::Dump<const tiro::ir::Terminator&>)
TIRO_ENABLE_FREE_FORMAT(tiro::ir::dump_helpers::Dump<tiro::ir::LValue>)
TIRO_ENABLE_FREE_FORMAT(tiro::ir::dump_helpers::Dump<tiro::ir::Phi>)
TIRO_ENABLE_FREE_FORMAT(tiro::ir::dump_helpers::Dump<tiro::ir::Constant>)
TIRO_ENABLE_FREE_FORMAT(tiro::ir::dump_helpers::Dump<tiro::ir::Aggregate>)
TIRO_ENABLE_FREE_FORMAT(tiro::ir::dump_helpers::Dump<const tiro::ir::Value&>)
TIRO_ENABLE_FREE_FORMAT(tiro::ir::dump_helpers::Dump<tiro::ir::InstId>)
TIRO_ENABLE_FREE_FORMAT(tiro::ir::dump_helpers::Dump<tiro::ir::LocalListId>)
TIRO_ENABLE_FREE_FORMAT(tiro::ir::dump_helpers::Dump<tiro::ir::RecordId>)

#endif // TIRO_COMPILER_IR_FUNCTION_HPP
