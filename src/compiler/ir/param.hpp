#ifndef TIRO_COMPILER_IR_PARAM_HPP
#define TIRO_COMPILER_IR_PARAM_HPP

#include "common/defs.hpp"
#include "common/format.hpp"
#include "common/text/string_table.hpp"
#include "compiler/ir/entities.hpp"
#include "compiler/ir/fwd.hpp"

namespace tiro::ir {

/// Represents a parameter to the function. Parameters appear in the same order
/// as in the source code.
class Param final {
public:
    // TODO source location

    explicit Param(InternedString name);

    InternedString name() const;

    void format(FormatStream& stream) const;

private:
    InternedString name_;
};

} // namespace tiro::ir

TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::Param)

#endif // TIRO_COMPILER_IR_PARAM_HPP
