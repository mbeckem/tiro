#ifndef HAMMER_COMPILER_SIMPLIFIER_HPP
#define HAMMER_COMPILER_SIMPLIFIER_HPP

#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/string_table.hpp"
#include "hammer/compiler/syntax/ast.hpp"

namespace hammer::compiler {

/// The simplifier lowers the AST from a high level tree
/// that represents the parsed source code to a lower level tree
/// that is easier to compile.
///
/// TODO: Not implemented yet - does nothing.
///
/// The plan is to do at least constant evaluation and
/// simplification of loops here (a single "loop" node instead
/// of multiple loop variants).
class Simplifier final {
public:
    explicit Simplifier(StringTable& strings, Diagnostics& diag);
    ~Simplifier();

    Simplifier(const Simplifier&) = delete;
    Simplifier& operator=(const Simplifier&) = delete;

    void simplify();

private:
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_SIMPLIFIER_HPP
