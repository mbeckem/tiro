#ifndef TIRO_CODEGEN_STMT_CODEGEN_HPP
#define TIRO_CODEGEN_STMT_CODEGEN_HPP

#include "tiro/codegen/codegen.hpp"

namespace tiro::compiler {

/// This class is responsible for compiling statements to bytecode.
class StmtCodegen final {
public:
    // leave_value: if true, leave the value produced by an expression statement on the stack.
    explicit StmtCodegen(NotNull<Stmt*> stmt, FunctionCodegen& func);

    void generate();

    ModuleCodegen& module() { return func_.module(); }

public:
    void visit_empty_stmt(EmptyStmt*) {}
    void visit_assert_stmt(AssertStmt* s);
    void visit_while_stmt(WhileStmt* s);
    void visit_for_stmt(ForStmt* s);
    void visit_decl_stmt(DeclStmt* s);
    void visit_expr_stmt(ExprStmt* s);

private:
    Stmt* stmt_;
    FunctionCodegen& func_;
    CodeBuilder& builder_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace tiro::compiler

#endif // TIRO_CODEGEN_STMT_CODEGEN_HPP
