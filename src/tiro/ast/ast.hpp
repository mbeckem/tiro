#ifndef TIRO_AST_AST_HPP
#define TIRO_AST_AST_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/string_table.hpp"

#include <string_view>
#include <vector>

namespace tiro {

enum class AccessType : u8 {
    Normal,
    Optional, // Null propagation, e.g. `instance?.member`.
};

std::string_view to_string(AccessType access);

/* [[[cog
    from codegen.unions import define
    from codegen.ast import ExprType
    define(ExprType)
]]] */
enum class ASTExprType : u8 {
    Block,
    Unary,
    Binary,
    Var,
    PropertyAccess,
    ElementAccess,
    Call,
    If,
    Return,
    Break,
    Continue,
    StringSequence,
    InterpolatedString,
    Null,
    Boolean,
    Integer,
    Float,
    String,
    Symbol,
    Array,
    Tuple,
    Set,
    Map,
    Func,
};

std::string_view to_string(ASTExprType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import ExprData
    define(ExprData)
]]] */
/// Represents the contents of an expression in the abstract syntax tree.
class ASTExprData final {
public:
    struct Block final {
        std::vector<ASTPtr<ASTStmt>> stmts;

        explicit Block(const std::vector<ASTPtr<ASTStmt>>& stmts_)
            : stmts(stmts_) {}
    };

    struct Unary final {
        UnaryOperator operation;
        ASTPtr<ASTExpr> inner;

        Unary(const UnaryOperator& operation_, const ASTPtr<ASTExpr>& inner_)
            : operation(operation_)
            , inner(inner_) {}
    };

    struct Binary final {
        BinaryOperator operation;
        ASTPtr<ASTExpr> left;
        ASTPtr<ASTExpr> right;

        Binary(const BinaryOperator& operation_, const ASTPtr<ASTExpr>& left_,
            const ASTPtr<ASTExpr>& right_)
            : operation(operation_)
            , left(left_)
            , right(right_) {}
    };

    struct Var final {
        InternedString name;

        explicit Var(const InternedString& name_)
            : name(name_) {}
    };

    struct PropertyAccess final {
        ASTAccessType access_type;
        ASTPtr<ASTExpr> instance;
        ASTProperty property;

        PropertyAccess(const ASTAccessType& access_type_,
            const ASTPtr<ASTExpr>& instance_, const ASTProperty& property_)
            : access_type(access_type_)
            , instance(instance_)
            , property(property_) {}
    };

    struct ElementAccess final {
        ASTAccessType access_type;
        ASTPtr<ASTExpr> instance;
        u32 element;

        ElementAccess(const ASTAccessType& access_type_,
            const ASTPtr<ASTExpr>& instance_, const u32& element_)
            : access_type(access_type_)
            , instance(instance_)
            , element(element_) {}
    };

    struct Call final {
        ASTAccessType access_type;
        ASTPtr<ASTExpr> func;
        std::vector<ASTPtr<ASTExpr>> args;

        Call(const ASTAccessType& access_type_, const ASTPtr<ASTExpr>& func_,
            const std::vector<ASTPtr<ASTExpr>>& args_)
            : access_type(access_type_)
            , func(func_)
            , args(args_) {}
    };

    struct If final {
        ASTPtr<ASTExpr> cond;
        ASTPtr<ASTExpr> then_branch;
        ASTPtr<ASTExpr> else_branch;

        If(const ASTPtr<ASTExpr>& cond_, const ASTPtr<ASTExpr>& then_branch_,
            const ASTPtr<ASTExpr>& else_branch_)
            : cond(cond_)
            , then_branch(then_branch_)
            , else_branch(else_branch_) {}
    };

    struct Return final {
        ASTPtr<ASTExpr> value;

        explicit Return(const ASTPtr<ASTExpr>& value_)
            : value(value_) {}
    };

    struct Break final {};

    struct Continue final {};

    struct StringSequence final {
        std::vector<ASTPtr<ASTExpr>> strings;

        explicit StringSequence(const std::vector<ASTPtr<ASTExpr>>& strings_)
            : strings(strings_) {}
    };

    struct InterpolatedString final {
        std::vector<ASTPtr<ASTExpr>> strings;

        explicit InterpolatedString(
            const std::vector<ASTPtr<ASTExpr>>& strings_)
            : strings(strings_) {}
    };

    struct Null final {};

    struct Boolean final {
        bool value;

        explicit Boolean(const bool& value_)
            : value(value_) {}
    };

    struct Integer final {
        i64 value;

        explicit Integer(const i64& value_)
            : value(value_) {}
    };

    struct Float final {
        f64 value;

        explicit Float(const f64& value_)
            : value(value_) {}
    };

    struct String final {
        InternedString value;

        explicit String(const InternedString& value_)
            : value(value_) {}
    };

    struct Symbol final {
        InternedString value;

        explicit Symbol(const InternedString& value_)
            : value(value_) {}
    };

    struct Array final {
        std::vector<ASTPtr<ASTExpr>> items;

        explicit Array(const std::vector<ASTPtr<ASTExpr>>& items_)
            : items(items_) {}
    };

    struct Tuple final {
        std::vector<ASTPtr<ASTExpr>> items;

        explicit Tuple(const std::vector<ASTPtr<ASTExpr>>& items_)
            : items(items_) {}
    };

    struct Set final {
        std::vector<ASTPtr<ASTExpr>> items;

        explicit Set(const std::vector<ASTPtr<ASTExpr>>& items_)
            : items(items_) {}
    };

    struct Map final {
        std::vector<ASTPtr<ASTExpr>> keys;
        std::vector<ASTPtr<ASTExpr>> values;

        Map(const std::vector<ASTPtr<ASTExpr>>& keys_,
            const std::vector<ASTPtr<ASTExpr>>& values_)
            : keys(keys_)
            , values(values_) {}
    };

    struct Func final {
        ASTPtr<ASTDecl> decl;

        explicit Func(const ASTPtr<ASTDecl>& decl_)
            : decl(decl_) {}
    };

    static ASTExprData make_block(const std::vector<ASTPtr<ASTStmt>>& stmts);
    static ASTExprData
    make_unary(const UnaryOperator& operation, const ASTPtr<ASTExpr>& inner);
    static ASTExprData make_binary(const BinaryOperator& operation,
        const ASTPtr<ASTExpr>& left, const ASTPtr<ASTExpr>& right);
    static ASTExprData make_var(const InternedString& name);
    static ASTExprData make_property_access(const ASTAccessType& access_type,
        const ASTPtr<ASTExpr>& instance, const ASTProperty& property);
    static ASTExprData make_element_access(const ASTAccessType& access_type,
        const ASTPtr<ASTExpr>& instance, const u32& element);
    static ASTExprData make_call(const ASTAccessType& access_type,
        const ASTPtr<ASTExpr>& func, const std::vector<ASTPtr<ASTExpr>>& args);
    static ASTExprData make_if(const ASTPtr<ASTExpr>& cond,
        const ASTPtr<ASTExpr>& then_branch, const ASTPtr<ASTExpr>& else_branch);
    static ASTExprData make_return(const ASTPtr<ASTExpr>& value);
    static ASTExprData make_break();
    static ASTExprData make_continue();
    static ASTExprData
    make_string_sequence(const std::vector<ASTPtr<ASTExpr>>& strings);
    static ASTExprData
    make_interpolated_string(const std::vector<ASTPtr<ASTExpr>>& strings);
    static ASTExprData make_null();
    static ASTExprData make_boolean(const bool& value);
    static ASTExprData make_integer(const i64& value);
    static ASTExprData make_float(const f64& value);
    static ASTExprData make_string(const InternedString& value);
    static ASTExprData make_symbol(const InternedString& value);
    static ASTExprData make_array(const std::vector<ASTPtr<ASTExpr>>& items);
    static ASTExprData make_tuple(const std::vector<ASTPtr<ASTExpr>>& items);
    static ASTExprData make_set(const std::vector<ASTPtr<ASTExpr>>& items);
    static ASTExprData make_map(const std::vector<ASTPtr<ASTExpr>>& keys,
        const std::vector<ASTPtr<ASTExpr>>& values);
    static ASTExprData make_func(const ASTPtr<ASTDecl>& decl);

    ASTExprData(const Block& block);
    ASTExprData(const Unary& unary);
    ASTExprData(const Binary& binary);
    ASTExprData(const Var& var);
    ASTExprData(const PropertyAccess& property_access);
    ASTExprData(const ElementAccess& element_access);
    ASTExprData(const Call& call);
    ASTExprData(const If& i);
    ASTExprData(const Return& ret);
    ASTExprData(const Break& br);
    ASTExprData(const Continue& cont);
    ASTExprData(const StringSequence& string_sequence);
    ASTExprData(const InterpolatedString& interpolated_string);
    ASTExprData(const Null& null);
    ASTExprData(const Boolean& boolean);
    ASTExprData(const Integer& integer);
    ASTExprData(const Float& f);
    ASTExprData(const String& string);
    ASTExprData(const Symbol& symbol);
    ASTExprData(const Array& array);
    ASTExprData(const Tuple& tuple);
    ASTExprData(const Set& set);
    ASTExprData(const Map& map);
    ASTExprData(const Func& func);

    ~ASTExprData();

    ASTExprData(ASTExprData&& other) noexcept;
    ASTExprData& operator=(ASTExprData&& other) noexcept;

    ASTExprType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Block& as_block() const;
    const Unary& as_unary() const;
    const Binary& as_binary() const;
    const Var& as_var() const;
    const PropertyAccess& as_property_access() const;
    const ElementAccess& as_element_access() const;
    const Call& as_call() const;
    const If& as_if() const;
    const Return& as_return() const;
    const Break& as_break() const;
    const Continue& as_continue() const;
    const StringSequence& as_string_sequence() const;
    const InterpolatedString& as_interpolated_string() const;
    const Null& as_null() const;
    const Boolean& as_boolean() const;
    const Integer& as_integer() const;
    const Float& as_float() const;
    const String& as_string() const;
    const Symbol& as_symbol() const;
    const Array& as_array() const;
    const Tuple& as_tuple() const;
    const Set& as_set() const;
    const Map& as_map() const;
    const Func& as_func() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto)
    visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    void _destroy_value() noexcept;
    void _move_construct_value(ASTExprData& other) noexcept;
    void _move_assign_value(ASTExprData& other) noexcept;

    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    ASTExprType type_;
    union {
        Block block_;
        Unary unary_;
        Binary binary_;
        Var var_;
        PropertyAccess property_access_;
        ElementAccess element_access_;
        Call call_;
        If if_;
        Return return_;
        Break break_;
        Continue continue_;
        StringSequence string_sequence_;
        InterpolatedString interpolated_string_;
        Null null_;
        Boolean boolean_;
        Integer integer_;
        Float float_;
        String string_;
        Symbol symbol_;
        Array array_;
        Tuple tuple_;
        Set set_;
        Map map_;
        Func func_;
    };
};
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import StmtType
    define(StmtType)
]]] */
enum class ASTStmtType : u8 {
    Empty,
    Assert,
    Decl,
    While,
    For,
    Expr,
};

std::string_view to_string(ASTStmtType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import StmtData
    define(StmtData)
]]] */
/// Represents the contents of a statement in the abstract syntax tree.
class ASTStmtData final {
public:
    struct Empty final {};

    struct Assert final {
        ASTPtr<ASTExpr> cond;
        ASTPtr<ASTExpr> message;

        Assert(const ASTPtr<ASTExpr>& cond_, const ASTPtr<ASTExpr>& message_)
            : cond(cond_)
            , message(message_) {}
    };

    struct Decl final {
        std::vector<ASTPtr<ASTDecl>> decls;

        explicit Decl(const std::vector<ASTPtr<ASTDecl>>& decls_)
            : decls(decls_) {}
    };

    struct While final {
        ASTPtr<ASTExpr> cond;
        ASTPtr<ASTExpr> body;

        While(const ASTPtr<ASTExpr>& cond_, const ASTPtr<ASTExpr>& body_)
            : cond(cond_)
            , body(body_) {}
    };

    struct For final {
        ASTPtr<ASTStmt> decl;
        ASTPtr<ASTExpr> cond;
        ASTPtr<ASTExpr> step;
        ASTPtr<ASTExpr> body;

        For(const ASTPtr<ASTStmt>& decl_, const ASTPtr<ASTExpr>& cond_,
            const ASTPtr<ASTExpr>& step_, const ASTPtr<ASTExpr>& body_)
            : decl(decl_)
            , cond(cond_)
            , step(step_)
            , body(body_) {}
    };

    struct Expr final {
        ASTPtr<ASTExpr> expr;

        explicit Expr(const ASTPtr<ASTExpr>& expr_)
            : expr(expr_) {}
    };

    static ASTStmtData make_empty();
    static ASTStmtData
    make_assert(const ASTPtr<ASTExpr>& cond, const ASTPtr<ASTExpr>& message);
    static ASTStmtData make_decl(const std::vector<ASTPtr<ASTDecl>>& decls);
    static ASTStmtData
    make_while(const ASTPtr<ASTExpr>& cond, const ASTPtr<ASTExpr>& body);
    static ASTStmtData
    make_for(const ASTPtr<ASTStmt>& decl, const ASTPtr<ASTExpr>& cond,
        const ASTPtr<ASTExpr>& step, const ASTPtr<ASTExpr>& body);
    static ASTStmtData make_expr(const ASTPtr<ASTExpr>& expr);

    ASTStmtData(const Empty& empty);
    ASTStmtData(const Assert& assert);
    ASTStmtData(const Decl& decl);
    ASTStmtData(const While& w);
    ASTStmtData(const For& f);
    ASTStmtData(const Expr& expr);

    ~ASTStmtData();

    ASTStmtData(ASTStmtData&& other) noexcept;
    ASTStmtData& operator=(ASTStmtData&& other) noexcept;

    ASTStmtType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Empty& as_empty() const;
    const Assert& as_assert() const;
    const Decl& as_decl() const;
    const While& as_while() const;
    const For& as_for() const;
    const Expr& as_expr() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto)
    visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    void _destroy_value() noexcept;
    void _move_construct_value(ASTStmtData& other) noexcept;
    void _move_assign_value(ASTStmtData& other) noexcept;

    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    ASTStmtType type_;
    union {
        Empty empty_;
        Assert assert_;
        Decl decl_;
        While while_;
        For for_;
        Expr expr_;
    };
};
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import DeclType
    define(DeclType)
]]] */
enum class ASTDeclType : u8 {
    Func,
    Var,
    Tuple,
    Import,
};

std::string_view to_string(ASTDeclType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import DeclData
    define(DeclData)
]]] */
/// Represents the contents of a declaration in the abstract syntax tree.
class ASTDeclData final {
public:
    struct Func final {
        InternedString name;
        std::vector<ASTPtr<ASTDecl>> params;
        ASTPtr<ASTExpr> body;
        bool body_is_value;

        Func(const InternedString& name_,
            const std::vector<ASTPtr<ASTDecl>>& params_,
            const ASTPtr<ASTExpr>& body_, const bool& body_is_value_)
            : name(name_)
            , params(params_)
            , body(body_)
            , body_is_value(body_is_value_) {}
    };

    struct Var final {
        InternedString name;
        bool is_const;
        ASTPtr<ASTExpr> init;

        Var(const InternedString& name_, const bool& is_const_,
            const ASTPtr<ASTExpr>& init_)
            : name(name_)
            , is_const(is_const_)
            , init(init_) {}
    };

    struct Tuple final {
        std::vector<InternedString> names;
        bool is_const;
        ASTPtr<ASTExpr> init;

        Tuple(const std::vector<InternedString>& names_, const bool& is_const_,
            const ASTPtr<ASTExpr>& init_)
            : names(names_)
            , is_const(is_const_)
            , init(init_) {}
    };

    struct Import final {
        std::vector<InternedString> path;

        explicit Import(const std::vector<InternedString>& path_)
            : path(path_) {}
    };

    static ASTDeclData make_func(const InternedString& name,
        const std::vector<ASTPtr<ASTDecl>>& params, const ASTPtr<ASTExpr>& body,
        const bool& body_is_value);
    static ASTDeclData make_var(const InternedString& name,
        const bool& is_const, const ASTPtr<ASTExpr>& init);
    static ASTDeclData make_tuple(const std::vector<InternedString>& names,
        const bool& is_const, const ASTPtr<ASTExpr>& init);
    static ASTDeclData make_import(const std::vector<InternedString>& path);

    ASTDeclData(const Func& func);
    ASTDeclData(const Var& var);
    ASTDeclData(const Tuple& tuple);
    ASTDeclData(const Import& import);

    ~ASTDeclData();

    ASTDeclData(ASTDeclData&& other) noexcept;
    ASTDeclData& operator=(ASTDeclData&& other) noexcept;

    ASTDeclType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Func& as_func() const;
    const Var& as_var() const;
    const Tuple& as_tuple() const;
    const Import& as_import() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto)
    visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    void _destroy_value() noexcept;
    void _move_construct_value(ASTDeclData& other) noexcept;
    void _move_assign_value(ASTDeclData& other) noexcept;

    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    ASTDeclType type_;
    union {
        Func func_;
        Var var_;
        Tuple tuple_;
        Import import_;
    };
};
// [[[end]]]

/* [[[cog
    from codegen.unions import implement_inlines
    from codegen.ast import ExprData
    implement_inlines(ExprData)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto)
ASTExprData::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case ASTExprType::Block:
        return vis.visit_block(self.block_, std::forward<Args>(args)...);
    case ASTExprType::Unary:
        return vis.visit_unary(self.unary_, std::forward<Args>(args)...);
    case ASTExprType::Binary:
        return vis.visit_binary(self.binary_, std::forward<Args>(args)...);
    case ASTExprType::Var:
        return vis.visit_var(self.var_, std::forward<Args>(args)...);
    case ASTExprType::PropertyAccess:
        return vis.visit_property_access(
            self.property_access_, std::forward<Args>(args)...);
    case ASTExprType::ElementAccess:
        return vis.visit_element_access(
            self.element_access_, std::forward<Args>(args)...);
    case ASTExprType::Call:
        return vis.visit_call(self.call_, std::forward<Args>(args)...);
    case ASTExprType::If:
        return vis.visit_if(self.if_, std::forward<Args>(args)...);
    case ASTExprType::Return:
        return vis.visit_return(self.return_, std::forward<Args>(args)...);
    case ASTExprType::Break:
        return vis.visit_break(self.break_, std::forward<Args>(args)...);
    case ASTExprType::Continue:
        return vis.visit_continue(self.continue_, std::forward<Args>(args)...);
    case ASTExprType::StringSequence:
        return vis.visit_string_sequence(
            self.string_sequence_, std::forward<Args>(args)...);
    case ASTExprType::InterpolatedString:
        return vis.visit_interpolated_string(
            self.interpolated_string_, std::forward<Args>(args)...);
    case ASTExprType::Null:
        return vis.visit_null(self.null_, std::forward<Args>(args)...);
    case ASTExprType::Boolean:
        return vis.visit_boolean(self.boolean_, std::forward<Args>(args)...);
    case ASTExprType::Integer:
        return vis.visit_integer(self.integer_, std::forward<Args>(args)...);
    case ASTExprType::Float:
        return vis.visit_float(self.float_, std::forward<Args>(args)...);
    case ASTExprType::String:
        return vis.visit_string(self.string_, std::forward<Args>(args)...);
    case ASTExprType::Symbol:
        return vis.visit_symbol(self.symbol_, std::forward<Args>(args)...);
    case ASTExprType::Array:
        return vis.visit_array(self.array_, std::forward<Args>(args)...);
    case ASTExprType::Tuple:
        return vis.visit_tuple(self.tuple_, std::forward<Args>(args)...);
    case ASTExprType::Set:
        return vis.visit_set(self.set_, std::forward<Args>(args)...);
    case ASTExprType::Map:
        return vis.visit_map(self.map_, std::forward<Args>(args)...);
    case ASTExprType::Func:
        return vis.visit_func(self.func_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid ASTExprData type.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement_inlines
    from codegen.ast import StmtData
    implement_inlines(StmtData)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto)
ASTStmtData::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case ASTStmtType::Empty:
        return vis.visit_empty(self.empty_, std::forward<Args>(args)...);
    case ASTStmtType::Assert:
        return vis.visit_assert(self.assert_, std::forward<Args>(args)...);
    case ASTStmtType::Decl:
        return vis.visit_decl(self.decl_, std::forward<Args>(args)...);
    case ASTStmtType::While:
        return vis.visit_while(self.while_, std::forward<Args>(args)...);
    case ASTStmtType::For:
        return vis.visit_for(self.for_, std::forward<Args>(args)...);
    case ASTStmtType::Expr:
        return vis.visit_expr(self.expr_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid ASTStmtData type.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement_inlines
    from codegen.ast import DeclData
    implement_inlines(DeclData)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto)
ASTDeclData::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case ASTDeclType::Func:
        return vis.visit_func(self.func_, std::forward<Args>(args)...);
    case ASTDeclType::Var:
        return vis.visit_var(self.var_, std::forward<Args>(args)...);
    case ASTDeclType::Tuple:
        return vis.visit_tuple(self.tuple_, std::forward<Args>(args)...);
    case ASTDeclType::Import:
        return vis.visit_import(self.import_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid ASTDeclData type.");
}
// [[[end]]]

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::AccessType);

TIRO_ENABLE_FREE_TO_STRING(tiro::ASTPropertyType);
TIRO_ENABLE_FREE_TO_STRING(tiro::ASTExprType);
TIRO_ENABLE_FREE_TO_STRING(tiro::ASTStmtType);
TIRO_ENABLE_FREE_TO_STRING(tiro::ASTDeclType);

TIRO_ENABLE_MEMBER_FORMAT(tiro::ASTProperty);
TIRO_ENABLE_MEMBER_FORMAT(tiro::ASTExprData);
TIRO_ENABLE_MEMBER_FORMAT(tiro::ASTStmtData);
TIRO_ENABLE_MEMBER_FORMAT(tiro::ASTDeclData);

#endif // TIRO_AST_AST_HPP
