#ifndef TIRO_COMPILER_AST_GEN_MAP_AST_HPP
#define TIRO_COMPILER_AST_GEN_MAP_AST_HPP

#include "common/adt/span.hpp"
#include "compiler/ast/fwd.hpp"
#include "compiler/fwd.hpp"
#include "compiler/syntax/fwd.hpp"

namespace tiro::next {

AstPtr<AstNode> map_program(Span<ParserEvent> events, Diagnostics& diag);

} // namespace tiro::next

#endif // TIRO_COMPILER_AST_GEN_MAP_AST_HPP
