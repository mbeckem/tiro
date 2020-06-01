#ifndef TIRO_AST_DUMP_HPP
#define TIRO_AST_DUMP_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/string_table.hpp"

namespace tiro {

std::string dump(const AstNode* node, const StringTable& strings);

} // namespace tiro

#endif // TIRO_AST_DUMP_HPP
