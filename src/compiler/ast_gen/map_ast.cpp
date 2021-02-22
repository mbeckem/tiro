#include "compiler/ast_gen/map_ast.hpp"

namespace tiro::next {

AstPtr<AstNode> map_program(Span<ParserEvent> events, Diagnostics& diag);

}
