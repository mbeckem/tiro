#include "hammer/compiler/codegen.hpp"

#include "hammer/ast/node_visit.hpp"
#include "hammer/ast/scope.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/core/error.hpp"
#include "hammer/core/overloaded.hpp"
#include "hammer/compiler/analyzer.hpp"

// TODO use this everywhere in this file where appropriate
#define HAMMER_ASSERT_VALUE(expr) \
    HAMMER_ASSERT((expr) && (expr)->has_value(), "Expression must have a value in this context.")

namespace hammer {

namespace {

// Replaces the value at "storage" with the new value and restores
// it to the old value in the destructor.
template<typename T>
struct ScopedReplace {
    ScopedReplace(T& storage, T new_value)
        : storage_(&storage)
        , old_value_(storage) {
        *storage_ = new_value;
    }

    ~ScopedReplace() { *storage_ = old_value_; }

    ScopedReplace(const ScopedReplace&) = delete;
    ScopedReplace& operator=(const ScopedReplace&) = delete;

private:
    T* storage_;
    T old_value_;
};

} // namespace

static u32 next_u32(u32& counter, const char* msg) {
    HAMMER_CHECK(counter != std::numeric_limits<u32>::max(), "Counter overflow: {}.", msg);
    return counter++;
}

FunctionCodegen::FunctionCodegen(ast::FuncDecl& func, ModuleCodegen& module, StringTable& strings,
                                 Diagnostics& diag)
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
        HAMMER_ASSERT(decl_to_location_.count(param) == 0, "Parameter already visited.");

        VarLocation loc;
        loc.type = VarLocationType::param;
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
                HAMMER_ASSERT(decl_to_location_.count(var) == 0, "Local variable already visited.");
                HAMMER_CHECK(!var->captured(), "Captured variables are not implemented yet.");

                VarLocation loc;
                loc.type = VarLocationType::local;
                loc.local.index = next_u32(next_local_, "too many locals");
                decl_to_location_.emplace(var, loc);
            } else {
                HAMMER_ERROR("Unexpected declaration in function: {}.", to_string(sym.kind()));
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
    if (body->has_value()) {
        builder_.ret();
    } else {
        builder_.load_null();
        builder_.ret();
    }
}

void FunctionCodegen::compile_expr(ast::Expr* expr) {
    HAMMER_ASSERT_NOT_NULL(expr);
    HAMMER_ASSERT(!expr->has_error(), "Invalid node in codegen.");

    ast::visit(*expr, [&](auto&& e) { compile_expr_impl(e); });
}

void FunctionCodegen::compile_expr_impl(ast::UnaryExpr& e) {
    HAMMER_ASSERT_VALUE(e.inner());

    switch (e.operation()) {
#define HAMMER_SIMPLE_UNARY(op, opcode) \
    case ast::UnaryOperator::op:        \
        compile_expr(e.inner());        \
        builder_.opcode();              \
        break;

        HAMMER_SIMPLE_UNARY(plus, upos)
        HAMMER_SIMPLE_UNARY(minus, uneg);
        HAMMER_SIMPLE_UNARY(bitwise_not, bnot);
        HAMMER_SIMPLE_UNARY(logical_not, lnot);
#undef HAMMER_SIMPLE_UNARY
    }
}

void FunctionCodegen::compile_expr_impl(ast::BinaryExpr& e) {
    HAMMER_ASSERT_VALUE(e.left_child());
    HAMMER_ASSERT_VALUE(e.right_child());

    switch (e.operation()) {
    case ast::BinaryOperator::assign:
        compile_assign_expr(&e);
        break;

    case ast::BinaryOperator::logical_and:
        compile_logical_and(e.left_child(), e.right_child());
        break;
    case ast::BinaryOperator::logical_or:
        compile_logical_or(e.left_child(), e.right_child());
        break;

// Simple binary expression case: compile lhs and rhs, then apply operator.
#define HAMMER_SIMPLE_BINARY(op, opcode) \
    case ast::BinaryOperator::op:        \
        compile_expr(e.left_child());    \
        compile_expr(e.right_child());   \
        builder_.opcode();               \
        break;

        HAMMER_SIMPLE_BINARY(plus, add)
        HAMMER_SIMPLE_BINARY(minus, sub)
        HAMMER_SIMPLE_BINARY(multiply, mul)
        HAMMER_SIMPLE_BINARY(divide, div)
        HAMMER_SIMPLE_BINARY(modulus, mod)
        HAMMER_SIMPLE_BINARY(power, pow)

        HAMMER_SIMPLE_BINARY(less, lt)
        HAMMER_SIMPLE_BINARY(less_eq, lte)
        HAMMER_SIMPLE_BINARY(greater, gt)
        HAMMER_SIMPLE_BINARY(greater_eq, gte)
        HAMMER_SIMPLE_BINARY(equals, eq)
        HAMMER_SIMPLE_BINARY(not_equals, neq)
#undef HAMMER_SIMPLE_BINARY

    case ast::BinaryOperator::left_shift:
    case ast::BinaryOperator::right_shift:
    case ast::BinaryOperator::bitwise_and:
    case ast::BinaryOperator::bitwise_or:
    case ast::BinaryOperator::bitwise_xor:
        // FIXME
        HAMMER_ERROR("Binary operator not implemented.");
    }
}

void FunctionCodegen::compile_expr_impl(ast::VarExpr& e) {
    HAMMER_ASSERT(e.decl(), "Must have a valid symbol reference.");

    VarLocation loc = get_location(e.decl());

    switch (loc.type) {
    case VarLocationType::param:
        builder_.load_param(loc.param.index);
        break;
    case VarLocationType::local:
        builder_.load_local(loc.local.index);
        break;
    case VarLocationType::module:
        builder_.load_module(loc.module.index);
        break;
    }
}

void FunctionCodegen::compile_expr_impl(ast::DotExpr& e) {
    HAMMER_ASSERT_VALUE(e.inner());
    HAMMER_ASSERT(e.name().valid(), "Invalid member name.");

    // Pushes the object we're accessing.
    compile_expr(e.inner());

    std::unique_ptr symbol = std::make_unique<CompiledSymbol>(e.name());
    const u32 symbol_index = constant(std::move(symbol));

    // Loads the member of the object.
    builder_.load_member(symbol_index);
}

void FunctionCodegen::compile_expr_impl(ast::CallExpr& e) {
    HAMMER_ASSERT_VALUE(e.func());
    compile_expr(e.func());

    const size_t args = e.arg_count();
    for (size_t i = 0; i < args; ++i) {
        ast::Expr* arg = e.get_arg(i);
        HAMMER_ASSERT_VALUE(arg);
        compile_expr(arg);
    }

    HAMMER_CHECK(args <= std::numeric_limits<u32>::max(), "Too many arguments.");
    builder_.call(args);
}

void FunctionCodegen::compile_expr_impl(ast::IndexExpr& e) {
    HAMMER_ASSERT_VALUE(e.inner());
    compile_expr(e.inner());

    HAMMER_ASSERT_VALUE(e.index());
    compile_expr(e.index());
    builder_.load_index();
}

void FunctionCodegen::compile_expr_impl(ast::IfExpr& e) {
    HAMMER_ASSERT_VALUE(e.condition());

    LabelGroup group(builder_);
    const LabelID if_else = group.gen("if-else");
    const LabelID if_end = group.gen("if-end");

    if (!e.else_branch()) {
        HAMMER_ASSERT(!e.has_value(), "If expr cannot have a value with one arm.");

        compile_expr(e.condition());
        builder_.jmp_false_pop(if_end);

        compile_expr(e.then_branch());
        if (e.then_branch()->has_value())
            builder_.pop();

        builder_.define_label(if_end);
    } else {
        compile_expr(e.condition());
        builder_.jmp_false_pop(if_else);

        compile_expr(e.then_branch());
        if (e.then_branch()->has_value() && !e.has_value())
            builder_.pop();

        builder_.jmp(if_end);

        builder_.define_label(if_else);
        compile_expr(e.else_branch());
        if (e.else_branch()->has_value() && !e.has_value())
            builder_.pop();

        builder_.define_label(if_end);
    }
}

void FunctionCodegen::compile_expr_impl(ast::ReturnExpr& e) {
    if (e.inner()) {
        HAMMER_ASSERT_VALUE(e.inner());
        compile_expr(e.inner());
    } else {
        builder_.load_null();
    }
    builder_.ret();
}

void FunctionCodegen::compile_expr_impl(ast::ContinueExpr&) {
    HAMMER_CHECK(current_loop_, "Not in a loop.");
    HAMMER_CHECK(current_loop_->continue_label, "Continue label not defined for this loop.");
    builder_.jmp(current_loop_->continue_label);
}

void FunctionCodegen::compile_expr_impl(ast::BreakExpr&) {
    HAMMER_CHECK(current_loop_, "Not in a loop.");
    HAMMER_CHECK(current_loop_->break_label, "Break label not defined for this loop.");
    builder_.jmp(current_loop_->break_label);
}

void FunctionCodegen::compile_expr_impl(ast::BlockExpr& e) {
    const size_t statements = e.stmt_count();

    if (e.has_value()) {
        HAMMER_CHECK(statements > 0,
                     "A block expression that producses a value must have at least one "
                     "statement.");

        auto* last = try_cast<ast::ExprStmt>(e.get_stmt(e.stmt_count() - 1));
        HAMMER_CHECK(last,
                     "Last statement of expression block must be a expression statement in "
                     "this block.");
        HAMMER_CHECK(last->used(), "Last statement must have the \"used\" flag set.");
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

void FunctionCodegen::compile_expr_impl(ast::ArrayLiteral& e) {
    unused(e);
    HAMMER_NOT_IMPLEMENTED();
    // FIXME
}

void FunctionCodegen::compile_expr_impl(ast::TupleLiteral& e) {
    unused(e);
    HAMMER_NOT_IMPLEMENTED();
    // FIXME
}

void FunctionCodegen::compile_expr_impl(ast::MapLiteral& e) {
    unused(e);
    HAMMER_NOT_IMPLEMENTED();
    // FIXME
}

void FunctionCodegen::compile_expr_impl(ast::SetLiteral& e) {
    unused(e);
    HAMMER_NOT_IMPLEMENTED();
    // FIXME
}

void FunctionCodegen::compile_expr_impl(ast::FuncLiteral& e) {
    unused(e);
    HAMMER_ERROR("Nested functions are not implemented yet.");
    // FIXME
}

void FunctionCodegen::compile_stmt(ast::Stmt* stmt) {
    HAMMER_ASSERT_NOT_NULL(stmt);
    HAMMER_ASSERT(!stmt->has_error(), "Invalid node in codegen.");

    ast::visit(*stmt, [&](auto&& n) { compile_stmt_impl(n); });
}

void FunctionCodegen::compile_stmt_impl(ast::WhileStmt& s) {
    HAMMER_ASSERT_VALUE(s.condition());

    LabelGroup group(builder_);
    const LabelID while_cond = group.gen("while-cond");
    const LabelID while_end = group.gen("while-end");

    builder_.define_label(while_cond);
    compile_expr(s.condition());
    builder_.jmp_false_pop(while_end);

    {
        LoopContext loop;
        loop.break_label = while_end;
        loop.continue_label = while_cond;
        ScopedReplace replace(current_loop_, &loop);

        compile_expr(s.body());
        if (s.body()->has_value())
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
        HAMMER_ASSERT_VALUE(s.condition());

        compile_expr(s.condition());
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
        if (s.body()->has_value())
            builder_.pop();
    }

    builder_.define_label(for_step);
    if (s.step()) {
        compile_expr(s.step());
        if (s.step()->has_value())
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
    if (s.expression()->has_value() && !s.used())
        builder_.pop();
}

void FunctionCodegen::compile_assign_expr(ast::BinaryExpr* assign) {
    HAMMER_ASSERT_NOT_NULL(assign);
    HAMMER_ASSERT(assign->operation() == ast::BinaryOperator::assign,
                  "Expression must be an assignment.");

    // TODO set param to false if the result of this expr is not used.

    auto visitor = Overloaded{
        [&](ast::DotExpr& e) { compile_member_assign(&e, assign->right_child(), true); },
        [&](ast::IndexExpr& e) { compile_index_assign(&e, assign->right_child(), true); },
        [&](ast::VarExpr& e) { compile_decl_assign(e.decl(), assign->right_child(), true); },
        [&](ast::Expr& e) {
            HAMMER_ERROR("Invalid left hand side of type {} in assignment.", to_string(e.kind()));
        }};
    ast::visit(*assign->left_child(), visitor);
}

void FunctionCodegen::compile_member_assign(ast::DotExpr* lhs, ast::Expr* rhs, bool push_value) {
    HAMMER_ASSERT_NOT_NULL(lhs);
    HAMMER_ASSERT_VALUE(rhs);
    HAMMER_ASSERT_VALUE(lhs->inner());

    // Pushes the object who's member we're manipulating.
    compile_expr(lhs->inner());

    // Pushes the value for the assignment.
    compile_expr(rhs);

    if (push_value) {
        builder_.dup();
        builder_.rot_3();
    }

    std::unique_ptr symbol = std::make_unique<CompiledSymbol>(lhs->name());
    const u32 symbol_index = constant(std::move(symbol));

    // Performs the assignment.
    builder_.store_member(symbol_index);
}

void FunctionCodegen::compile_index_assign(ast::IndexExpr* lhs, ast::Expr* rhs, bool push_value) {
    HAMMER_ASSERT_NOT_NULL(lhs);
    HAMMER_ASSERT_VALUE(rhs);
    HAMMER_ASSERT_VALUE(lhs->inner());
    HAMMER_ASSERT_VALUE(lhs->index());

    // Pushes the object
    compile_expr(lhs->inner());

    // Pushes the index value
    compile_expr(lhs->index());

    // Pushes the value for the assignment.
    compile_expr(rhs);

    if (push_value) {
        builder_.dup();
        builder_.rot_4();
    }

    builder_.store_index();
}

void FunctionCodegen::compile_decl_assign(ast::Decl* lhs, ast::Expr* rhs, bool push_value) {
    HAMMER_ASSERT_NOT_NULL(lhs);
    HAMMER_ASSERT_VALUE(rhs);

    compile_expr(rhs);

    if (push_value) {
        builder_.dup();
    }

    VarLocation loc = get_location(lhs);
    switch (loc.type) {
    case VarLocationType::param:
        builder_.store_param(loc.param.index);
        break;
    case VarLocationType::local:
        builder_.store_local(loc.local.index);
        break;
    case VarLocationType::module:
        builder_.store_module(loc.module.index);
        break;
    }
}

void FunctionCodegen::compile_logical_and(ast::Expr* lhs, ast::Expr* rhs) {
    HAMMER_ASSERT_VALUE(lhs);
    HAMMER_ASSERT_VALUE(rhs);

    LabelGroup group(builder_);
    const LabelID and_end = group.gen("and-end");

    compile_expr(lhs);
    builder_.jmp_false(and_end);

    builder_.pop();
    compile_expr(rhs);
    builder_.define_label(and_end);
}

void FunctionCodegen::compile_logical_or(ast::Expr* lhs, ast::Expr* rhs) {
    HAMMER_ASSERT_VALUE(lhs);
    HAMMER_ASSERT_VALUE(rhs);

    LabelGroup group(builder_);
    const LabelID or_end = group.gen("or-end");

    compile_expr(lhs);
    builder_.jmp_true(or_end);

    builder_.pop();
    compile_expr(rhs);
    builder_.define_label(or_end);
}

VarLocation FunctionCodegen::get_location(ast::Decl* decl) const {
    HAMMER_ASSERT_NOT_NULL(decl);
    if (auto pos = decl_to_location_.find(decl); pos != decl_to_location_.end()) {
        return pos->second;
    }

    return module_.get_location(decl);
}

u32 FunctionCodegen::constant(std::unique_ptr<CompiledOutput> o) {
    CompiledOutput* o_ptr = o.get();
    if (auto pos = constant_to_index_.find(o_ptr); pos != constant_to_index_.end()) {
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

ModuleCodegen::ModuleCodegen(ast::File& file, StringTable& strings, Diagnostics& diag)
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

        HAMMER_ERROR("Invalid node of type {} at module level.", to_string(item->kind()));
    }

    auto insert_loc = [&](ast::Decl* decl, u32 index, bool constant) {
        HAMMER_ASSERT(!decl_to_location_.count(decl), "Decl already indexed.");
        VarLocation loc;
        loc.type = VarLocationType::module;
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
    if (auto pos = decl_to_location_.find(decl); pos != decl_to_location_.end()) {
        return pos->second;
    }

    HAMMER_ERROR("Failed to find location.");
}

} // namespace hammer