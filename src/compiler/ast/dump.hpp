#ifndef TIRO_COMPILER_AST_DUMP_HPP
#define TIRO_COMPILER_AST_DUMP_HPP

#include "common/format.hpp"
#include "common/string_table.hpp"
#include "compiler/ast/fwd.hpp"

namespace tiro {

std::string dump(const AstNode* node, const StringTable& strings);

} // namespace tiro

#endif // TIRO_COMPILER_AST_DUMP_HPP
