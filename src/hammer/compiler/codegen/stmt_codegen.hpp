#ifndef HAMMER_COMPILER_CODEGEN_STMT_CODEGEN_HPP
#define HAMMER_COMPILER_CODEGEN_STMT_CODEGEN_HPP

#include "hammer/compiler/codegen/codegen.hpp"

namespace hammer::compiler {

/// This class is responsible for compiling statements to bytecode.
class StmtCodegen final {
public:
    // leave_value: if true, leave the value produced by an expression statement on the stack.
    explicit StmtCodegen(const NodePtr<Stmt>& stmt, FunctionCodegen& func);

    void generate();

    ModuleCodegen& module() { return func_.module(); }

public:
    void visit_empty_stmt(const NodePtr<EmptyStmt>&) {}
    void visit_assert_stmt(const NodePtr<AssertStmt>& s);
    void visit_while_stmt(const NodePtr<WhileStmt>& s);
    void visit_for_stmt(const NodePtr<ForStmt>& s);
    void visit_decl_stmt(const NodePtr<DeclStmt>& s);
    void visit_expr_stmt(const NodePtr<ExprStmt>& s);

private:
    const NodePtr<Stmt>& stmt_;
    FunctionCodegen& func_;
    CodeBuilder& builder_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_CODEGEN_STMT_CODEGEN_HPP
