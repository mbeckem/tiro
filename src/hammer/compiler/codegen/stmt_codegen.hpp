#ifndef HAMMER_COMPILER_CODEGEN_STMT_CODEGEN_HPP
#define HAMMER_COMPILER_CODEGEN_STMT_CODEGEN_HPP

#include "hammer/compiler/codegen/codegen.hpp"

namespace hammer {

/**
 * This class is responsible for compiling statements to bytecode.
 */
class StmtCodegen final {
public:
    StmtCodegen(ast::Stmt& stmt, FunctionCodegen& func);

    void generate();

private:
    void gen_impl(ast::EmptyStmt&) {}
    void gen_impl(ast::AssertStmt& s);
    void gen_impl(ast::WhileStmt& s);
    void gen_impl(ast::ForStmt& s);
    void gen_impl(ast::DeclStmt& s);
    void gen_impl(ast::ExprStmt& s);

private:
    ast::Stmt& stmt_;
    FunctionCodegen& func_;
    CodeBuilder& builder_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace hammer

#endif // HAMMER_COMPILER_CODEGEN_STMT_CODEGEN_HPP
