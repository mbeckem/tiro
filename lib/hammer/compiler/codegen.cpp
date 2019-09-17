#include "hammer/compiler/codegen.hpp"

#include "hammer/ast/node_visit.hpp"
#include "hammer/ast/scope.hpp"
#include "hammer/compiler/analyzer.hpp"
#include "hammer/compiler/utils.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/core/overloaded.hpp"

namespace hammer {

static u32 next_u32(u32& counter, const char* msg) {
    HAMMER_CHECK(counter != std::numeric_limits<u32>::max(),
        "Counter overflow: {}.", msg);
    return counter++;
}

FunctionCodegen::FunctionCodegen(ast::FuncDecl& func, ModuleCodegen& module,
    StringTable& strings, Diagnostics& diag)
    : func_(func)
    , module_(module)
    , strings_(strings)
    , diag_(diag)
    , result_(std::make_unique<CompiledFunction>())
    , builder_(result_->code) {

    result_->name = func.name();
}

void FunctionCodegen::compile() {
    visit_scopes();
    compile_function_body(func_.body());
    builder_.finish();
}

void FunctionCodegen::visit_scopes() {
    HAMMER_ASSERT(func_.get_scope_kind() == ast::ScopeKind::ParameterScope,
        "Invalid function scope.");
    const size_t params = func_.param_count();
    for (size_t i = 0; i < params; ++i) {
        ast::ParamDecl* param = func_.get_param(i);
        HAMMER_ASSERT(
            decl_to_location_.count(param) == 0, "Parameter already visited.");

        VarLocation loc;
        loc.type = VarLocationType::Param;
        loc.param.index = next_u32(next_param_, "too many params");

        decl_to_location_.emplace(param, loc);
    }

    visit_scopes(func_.body());

    result_->params = next_param_;
    result_->locals = max_local_;
}

void FunctionCodegen::visit_scopes(ast::Node* node) {
    HAMMER_ASSERT_NOT_NULL(node);

    if (node->has_error())
        return;

    // Don't recurse into nested functions.
    // TODO also don't recurse into nested classes.
    if (isa<ast::FuncDecl>(node)) {
        return;
    }

    if (ast::Scope* sc = Analyzer::as_scope(*node)) {
        const u32 old_locals_counter = next_local_;

        for (ast::Decl& sym : sc->declarations()) {
            if (ast::VarDecl* var = try_cast<ast::VarDecl>(&sym)) {
                HAMMER_ASSERT(decl_to_location_.count(var) == 0,
                    "Local variable already visited.");
                HAMMER_CHECK(!var->captured(),
                    "Captured variables are not implemented yet.");

                VarLocation loc;
                loc.type = VarLocationType::Local;
                loc.local.index = next_u32(next_local_, "too many locals");
                decl_to_location_.emplace(var, loc);
            } else {
                HAMMER_ERROR("Unexpected declaration in function: {}.",
                    to_string(sym.kind()));
            }
        }

        for (ast::Node& child : node->children()) {
            visit_scopes(&child);
        }

        max_local_ = std::max(next_local_, max_local_);
        next_local_ = old_locals_counter;
    } else {
        for (ast::Node& child : node->children()) {
            visit_scopes(&child);
        }
    }
}

void FunctionCodegen::compile_function_body(ast::BlockExpr* body) {
    HAMMER_ASSERT_NOT_NULL(body);

    compile_expr(body);
    switch (body->type()) {
    case ast::ExprType::Value:
        builder_.ret();
        break;
    case ast::ExprType::Never:
        // Nothing, control flow doesn't get here.
        break;
    case ast::ExprType::None:
        builder_.load_null();
        builder_.ret();
        break;
    }
}

void FunctionCodegen::compile_expr(ast::Expr* expr) {
    HAMMER_ASSERT_NOT_NULL(expr);
    HAMMER_ASSERT(!expr->has_error(), "Invalid node in codegen.");

    ast::visit(*expr, [&](auto&& e) { compile_expr_impl(e); });
}

void FunctionCodegen::compile_expr_value(ast::Expr* expr) {
    HAMMER_ASSERT(expr && expr->can_use_as_value(),
        "Cannot use this expression in a value context.");
    return compile_expr(expr);
}

void FunctionCodegen::compile_expr_impl(ast::UnaryExpr& e) {
    switch (e.operation()) {
#define HAMMER_SIMPLE_UNARY(op, opcode) \
    case ast::UnaryOperator::op:        \
        compile_expr_value(e.inner());  \
        builder_.opcode();              \
        break;

        HAMMER_SIMPLE_UNARY(Plus, upos)
        HAMMER_SIMPLE_UNARY(Minus, uneg);
        HAMMER_SIMPLE_UNARY(BitwiseNot, bnot);
        HAMMER_SIMPLE_UNARY(LogicalNot, lnot);
#undef HAMMER_SIMPLE_UNARY
    }
}

void FunctionCodegen::compile_expr_impl(ast::BinaryExpr& e) {
    switch (e.operation()) {
    case ast::BinaryOperator::Assign:
        compile_assign_expr(&e);
        break;

    case ast::BinaryOperator::LogicalAnd:
        compile_logical_and(e.left_child(), e.right_child());
        break;
    case ast::BinaryOperator::LogicalOr:
        compile_logical_or(e.left_child(), e.right_child());
        break;

// Simple binary expression case: compile lhs and rhs, then apply operator.
#define HAMMER_SIMPLE_BINARY(op, opcode)     \
    case ast::BinaryOperator::op:            \
        compile_expr_value(e.left_child());  \
        compile_expr_value(e.right_child()); \
        builder_.opcode();                   \
        break;

        HAMMER_SIMPLE_BINARY(Plus, add)
        HAMMER_SIMPLE_BINARY(Minus, sub)
        HAMMER_SIMPLE_BINARY(Multiply, mul)
        HAMMER_SIMPLE_BINARY(Divide, div)
        HAMMER_SIMPLE_BINARY(Modulus, mod)
        HAMMER_SIMPLE_BINARY(Power, pow)

        HAMMER_SIMPLE_BINARY(Less, lt)
        HAMMER_SIMPLE_BINARY(LessEquals, lte)
        HAMMER_SIMPLE_BINARY(Greater, gt)
        HAMMER_SIMPLE_BINARY(GreaterEquals, gte)
        HAMMER_SIMPLE_BINARY(Equals, eq)
        HAMMER_SIMPLE_BINARY(NotEquals, neq)

        HAMMER_SIMPLE_BINARY(LeftShift, lsh)
        HAMMER_SIMPLE_BINARY(RightShift, rsh)
        HAMMER_SIMPLE_BINARY(BitwiseAnd, band)
        HAMMER_SIMPLE_BINARY(BitwiseOr, bor)
        HAMMER_SIMPLE_BINARY(BitwiseXor, bxor)
#undef HAMMER_SIMPLE_BINARY
    }
}

void FunctionCodegen::compile_expr_impl(ast::VarExpr& e) {
    HAMMER_ASSERT(e.decl(), "Must have a valid symbol reference.");

    VarLocation loc = get_location(e.decl());

    switch (loc.type) {
    case VarLocationType::Param:
        builder_.load_param(loc.param.index);
        break;
    case VarLocationType::Local:
        builder_.load_local(loc.local.index);
        break;
    case VarLocationType::Module:
        builder_.load_module(loc.module.index);
        break;
    }
}

void FunctionCodegen::compile_expr_impl(ast::DotExpr& e) {
    HAMMER_ASSERT(e.name().valid(), "Invalid member name.");

    // Pushes the object we're accessing.
    compile_expr_value(e.inner());

    std::unique_ptr symbol = std::make_unique<CompiledSymbol>(e.name());
    const u32 symbol_index = constant(std::move(symbol));

    // Loads the member of the object.
    builder_.load_member(symbol_index);
}

void FunctionCodegen::compile_expr_impl(ast::CallExpr& e) {
    compile_expr_value(e.func());

    const size_t args = e.arg_count();
    for (size_t i = 0; i < args; ++i) {
        ast::Expr* arg = e.get_arg(i);
        compile_expr_value(arg);
    }

    HAMMER_CHECK(
        args <= std::numeric_limits<u32>::max(), "Too many arguments.");
    builder_.call(args);
}

void FunctionCodegen::compile_expr_impl(ast::IndexExpr& e) {
    compile_expr_value(e.inner());
    compile_expr_value(e.index());
    builder_.load_index();
}

void FunctionCodegen::compile_expr_impl(ast::IfExpr& e) {
    LabelGroup group(builder_);
    const LabelID if_else = group.gen("if-else");
    const LabelID if_end = group.gen("if-end");

    if (!e.else_branch()) {
        HAMMER_ASSERT(
            !e.can_use_as_value(), "If expr cannot have a value with one arm.");

        compile_expr_value(e.condition());
        builder_.jmp_false_pop(if_end);

        compile_expr(e.then_branch());
        if (e.then_branch()->type() == ast::ExprType::Value)
            builder_.pop();

        builder_.define_label(if_end);
    } else {
        compile_expr_value(e.condition());
        builder_.jmp_false_pop(if_else);

        compile_expr(e.then_branch());
        if (e.then_branch()->type() == ast::ExprType::Value
            && e.type() != ast::ExprType::Value)
            builder_.pop();

        builder_.jmp(if_end);

        builder_.define_label(if_else);
        compile_expr(e.else_branch());
        if (e.else_branch()->type() == ast::ExprType::Value
            && e.type() != ast::ExprType::Value)
            builder_.pop();

        builder_.define_label(if_end);
    }
}

void FunctionCodegen::compile_expr_impl(ast::ReturnExpr& e) {
    if (e.inner()) {
        compile_expr_value(e.inner());
        if (e.inner()->type() == ast::ExprType::Value)
            builder_.ret();
    } else {
        builder_.load_null();
        builder_.ret();
    }
}

void FunctionCodegen::compile_expr_impl(ast::ContinueExpr&) {
    HAMMER_CHECK(current_loop_, "Not in a loop.");
    HAMMER_CHECK(current_loop_->continue_label,
        "Continue label not defined for this loop.");
    builder_.jmp(current_loop_->continue_label);
}

void FunctionCodegen::compile_expr_impl(ast::BreakExpr&) {
    HAMMER_CHECK(current_loop_, "Not in a loop.");
    HAMMER_CHECK(
        current_loop_->break_label, "Break label not defined for this loop.");
    builder_.jmp(current_loop_->break_label);
}

void FunctionCodegen::compile_expr_impl(ast::BlockExpr& e) {
    const size_t statements = e.stmt_count();

    if (e.can_use_as_value()) {
        HAMMER_CHECK(statements > 0,
            "A block expression that producses a value must have at least one "
            "statement.");

        auto* last = try_cast<ast::ExprStmt>(e.get_stmt(e.stmt_count() - 1));
        HAMMER_CHECK(last,
            "Last statement of expression block must be a expression statement "
            "in this block.");
        HAMMER_CHECK(
            last->used(), "Last statement must have the \"used\" flag set.");
    }

    for (size_t i = 0; i < statements; ++i) {
        compile_stmt(e.get_stmt(i));
    }
}

void FunctionCodegen::compile_expr_impl(ast::NullLiteral&) {
    builder_.load_null();
}

void FunctionCodegen::compile_expr_impl(ast::BooleanLiteral& e) {
    if (e.value()) {
        builder_.load_true();
    } else {
        builder_.load_false();
    }
}

void FunctionCodegen::compile_expr_impl(ast::IntegerLiteral& e) {
    // TODO more instructions (for smaller numbers that dont need 64 bit)
    // and / or use constant table.
    builder_.load_int(e.value());
}

void FunctionCodegen::compile_expr_impl(ast::FloatLiteral& e) {
    builder_.load_float(e.value());
}

void FunctionCodegen::compile_expr_impl(ast::StringLiteral& e) {
    HAMMER_ASSERT(e.value().valid(), "Invalid string constant.");

    std::unique_ptr string = std::make_unique<CompiledString>(e.value());
    const u32 constant_index = constant(std::move(string));

    builder_.load_const(constant_index);
}

void FunctionCodegen::compile_expr_impl([[maybe_unused]] ast::ArrayLiteral& e) {
    HAMMER_NOT_IMPLEMENTED();
    // FIXME
}

void FunctionCodegen::compile_expr_impl([[maybe_unused]] ast::TupleLiteral& e) {
    HAMMER_NOT_IMPLEMENTED();
    // FIXME
}

void FunctionCodegen::compile_expr_impl([[maybe_unused]] ast::MapLiteral& e) {
    HAMMER_NOT_IMPLEMENTED();
    // FIXME
}

void FunctionCodegen::compile_expr_impl([[maybe_unused]] ast::SetLiteral& e) {
    HAMMER_NOT_IMPLEMENTED();
    // FIXME
}

void FunctionCodegen::compile_expr_impl([[maybe_unused]] ast::FuncLiteral& e) {
    HAMMER_ERROR("Nested functions are not implemented yet.");
    // FIXME
}

void FunctionCodegen::compile_stmt(ast::Stmt* stmt) {
    HAMMER_ASSERT_NOT_NULL(stmt);
    HAMMER_ASSERT(!stmt->has_error(), "Invalid node in codegen.");

    ast::visit(*stmt, [&](auto&& n) { compile_stmt_impl(n); });
}

void FunctionCodegen::compile_stmt_impl(ast::WhileStmt& s) {
    LabelGroup group(builder_);
    const LabelID while_cond = group.gen("while-cond");
    const LabelID while_end = group.gen("while-end");

    builder_.define_label(while_cond);
    compile_expr_value(s.condition());
    builder_.jmp_false_pop(while_end);

    {
        LoopContext loop;
        loop.break_label = while_end;
        loop.continue_label = while_cond;
        ScopedReplace replace(current_loop_, &loop);

        compile_expr(s.body());
        if (s.body()->type() == ast::ExprType::Value)
            builder_.pop();

        builder_.jmp(while_cond);
    }

    builder_.define_label(while_end);
}

void FunctionCodegen::compile_stmt_impl(ast::ForStmt& s) {
    // TODO lower the for statement to a simple loop before generating code.

    LabelGroup group(builder_);
    const LabelID for_cond = group.gen("for-cond");
    const LabelID for_step = group.gen("for-step");
    const LabelID for_end = group.gen("for-end");

    if (s.decl()) {
        compile_stmt(s.decl());
    }

    builder_.define_label(for_cond);
    if (s.condition()) {
        compile_expr_value(s.condition());
        builder_.jmp_false_pop(for_end);
    } else {
        // Nothing, fall through to body. Equivalent to for (; true; )
    }

    {
        LoopContext loop;
        loop.break_label = for_end;
        loop.continue_label = for_step;
        ScopedReplace replace(current_loop_, &loop);

        HAMMER_ASSERT(s.body(), "For loop must have a body.");
        compile_expr(s.body());
        if (s.body()->type() == ast::ExprType::Value)
            builder_.pop();
    }

    builder_.define_label(for_step);
    if (s.step()) {
        compile_expr(s.step());
        if (s.step()->type() == ast::ExprType::Value)
            builder_.pop();
    }
    builder_.jmp(for_cond);

    builder_.define_label(for_end);
}

void FunctionCodegen::compile_stmt_impl(ast::DeclStmt& s) {
    ast::Expr* init = s.declaration()->initializer();
    if (init) {
        compile_decl_assign(s.declaration(), init, false);
    }
}

void FunctionCodegen::compile_stmt_impl(ast::ExprStmt& s) {
    compile_expr(s.expression());
    if (s.expression()->type() == ast::ExprType::Value && !s.used())
        builder_.pop();
}

void FunctionCodegen::compile_assign_expr(ast::BinaryExpr* assign) {
    HAMMER_ASSERT_NOT_NULL(assign);
    HAMMER_ASSERT(assign->operation() == ast::BinaryOperator::Assign,
        "Expression must be an assignment.");

    // TODO Optimize the result of assignments that are never used.
    // Either by tracking the usage of expressions (see the "push_value" parameter below)
    // or by having a better optimizating pass that can detect and remove dup / pop patterns.

    auto visitor = Overloaded{[&](ast::DotExpr& e) {
                                  compile_member_assign(
                                      &e, assign->right_child(), true);
                              },
        [&](ast::IndexExpr& e) {
            compile_index_assign(&e, assign->right_child(), true);
        },
        [&](ast::VarExpr& e) {
            compile_decl_assign(e.decl(), assign->right_child(), true);
        },
        [&](ast::Expr& e) {
            HAMMER_ERROR("Invalid left hand side of type {} in assignment.",
                to_string(e.kind()));
        }};
    ast::visit(*assign->left_child(), visitor);
}

void FunctionCodegen::compile_member_assign(
    ast::DotExpr* lhs, ast::Expr* rhs, bool push_value) {
    HAMMER_ASSERT_NOT_NULL(lhs);

    // Pushes the object who's member we're manipulating.
    compile_expr_value(lhs->inner());

    // Pushes the value for the assignment.
    compile_expr_value(rhs);

    if (push_value) {
        builder_.dup();
        builder_.rot_3();
    }

    std::unique_ptr symbol = std::make_unique<CompiledSymbol>(lhs->name());
    const u32 symbol_index = constant(std::move(symbol));

    // Performs the assignment.
    builder_.store_member(symbol_index);
}

void FunctionCodegen::compile_index_assign(
    ast::IndexExpr* lhs, ast::Expr* rhs, bool push_value) {
    HAMMER_ASSERT_NOT_NULL(lhs);

    // Pushes the object
    compile_expr_value(lhs->inner());

    // Pushes the index value
    compile_expr_value(lhs->index());

    // Pushes the value for the assignment.
    compile_expr_value(rhs);

    if (push_value) {
        builder_.dup();
        builder_.rot_4();
    }

    builder_.store_index();
}

void FunctionCodegen::compile_decl_assign(
    ast::Decl* lhs, ast::Expr* rhs, bool push_value) {
    HAMMER_ASSERT_NOT_NULL(lhs);

    compile_expr_value(rhs);
    if (push_value) {
        builder_.dup();
    }

    VarLocation loc = get_location(lhs);
    switch (loc.type) {
    case VarLocationType::Param:
        builder_.store_param(loc.param.index);
        break;
    case VarLocationType::Local:
        builder_.store_local(loc.local.index);
        break;
    case VarLocationType::Module:
        builder_.store_module(loc.module.index);
        break;
    }
}

void FunctionCodegen::compile_logical_and(ast::Expr* lhs, ast::Expr* rhs) {
    LabelGroup group(builder_);
    const LabelID and_end = group.gen("and-end");

    compile_expr_value(lhs);
    builder_.jmp_false(and_end);

    builder_.pop();
    compile_expr_value(rhs);
    builder_.define_label(and_end);
}

void FunctionCodegen::compile_logical_or(ast::Expr* lhs, ast::Expr* rhs) {
    LabelGroup group(builder_);
    const LabelID or_end = group.gen("or-end");

    compile_expr_value(lhs);
    builder_.jmp_true(or_end);

    builder_.pop();
    compile_expr_value(rhs);
    builder_.define_label(or_end);
}

VarLocation FunctionCodegen::get_location(ast::Decl* decl) const {
    HAMMER_ASSERT_NOT_NULL(decl);
    if (auto pos = decl_to_location_.find(decl);
        pos != decl_to_location_.end()) {
        return pos->second;
    }

    return module_.get_location(decl);
}

u32 FunctionCodegen::constant(std::unique_ptr<CompiledOutput> o) {
    CompiledOutput* o_ptr = o.get();
    if (auto pos = constant_to_index_.find(o_ptr);
        pos != constant_to_index_.end()) {
        return pos->second;
    }

    u32 constant_index = insert_constant(std::move(o));
    constant_to_index_.emplace(o_ptr, constant_index);
    return constant_index;
}

u32 FunctionCodegen::insert_constant(std::unique_ptr<CompiledOutput> o) {
    HAMMER_CHECK(result_->literals.size() <= std::numeric_limits<u32>::max(),
        "Too many constants.");
    u32 constant_index = result_->literals.size();
    result_->literals.push_back(std::move(o));
    return constant_index;
}

ModuleCodegen::ModuleCodegen(
    ast::File& file, StringTable& strings, Diagnostics& diag)
    : file_(file)
    , strings_(strings)
    , diag_(diag)
    , result_(std::make_unique<CompiledModule>()) {
    // FIXME strip extensions and stuff, compute full name.
    result_->name = file.file_name();
}

void ModuleCodegen::compile() {
    const size_t items = file_.item_count();

    std::vector<ast::ImportDecl*> imports;
    std::vector<ast::FuncDecl*> functions;

    for (size_t i = 0; i < items; ++i) {
        ast::Node* item = file_.get_item(i);

        if (ast::ImportDecl* decl = try_cast<ast::ImportDecl>(item)) {
            imports.push_back(decl);
            continue;
        }

        if (ast::FuncDecl* decl = try_cast<ast::FuncDecl>(item)) {
            functions.push_back(decl);
            continue;
        }

        HAMMER_ERROR("Invalid node of type {} at module level.",
            to_string(item->kind()));
    }

    auto insert_loc = [&](ast::Decl* decl, u32 index, bool constant) {
        HAMMER_ASSERT(!decl_to_location_.count(decl), "Decl already indexed.");
        VarLocation loc;
        loc.type = VarLocationType::Module;
        loc.module.index = index;
        loc.module.constant = constant;
        decl_to_location_.emplace(decl, loc);
    };

    u32 index = 0;
    for (auto* i : imports) {
        HAMMER_ASSERT(i->name(), "Invalid name.");
        result_->members.push_back(std::make_unique<CompiledImport>(i->name()));
        insert_loc(i, index, true);
        index++;
    }
    for (auto* f : functions) {
        insert_loc(f, index, true);
        index++;
    }

    for (auto* f : functions) {
        FunctionCodegen gen(*f, *this, strings_, diag_);
        gen.compile();
        result_->members.push_back(gen.take_result());
    }
}

VarLocation ModuleCodegen::get_location(ast::Decl* decl) const {
    HAMMER_ASSERT_NOT_NULL(decl);
    if (auto pos = decl_to_location_.find(decl);
        pos != decl_to_location_.end()) {
        return pos->second;
    }

    HAMMER_ERROR("Failed to find location.");
}

} // namespace hammer
