#ifndef TIRO_COMPILER_IR_INSTR_HPP
#define TIRO_COMPILER_IR_INSTR_HPP

#include "common/defs.hpp"
#include "common/format.hpp"
#include "common/text/string_table.hpp"
#include "compiler/ir/entities.hpp"
#include "compiler/ir/fwd.hpp"
#include "compiler/ir/value.hpp"

namespace tiro::ir {

/// Represents an IR instruction.
/// Instructions use SSA (Static Single Assignment) form: they are defined exactly once.
/// The Inst class contains metadata associated with the instruction combined with the
/// actual value, which describes the operation to perform.
class Inst final {
public:
    // TODO source ranges

    Inst(Value value);

    /// Only declared variables have a valid name.
    void name(InternedString name) { name_ = name; }
    InternedString name() const noexcept { return name_; }

    /// The value bound to this instruction.
    const Value& value() const noexcept { return value_; }
    Value& value() noexcept { return value_; }
    void value(Value&& value) { value_ = std::move(value); }

    void format(FormatStream& stream) const;

private:
    InternedString name_;
    Value value_;
};

} // namespace tiro::ir

TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::Inst)

#endif // TIRO_COMPILER_IR_INSTR_HPP
