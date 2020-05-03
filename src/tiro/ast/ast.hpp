#ifndef TIRO_AST_AST_HPP
#define TIRO_AST_AST_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/ast/operators.hpp"
#include "tiro/ast/ptr.hpp"
#include "tiro/compiler/source_reference.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/id_type.hpp"
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
    from codegen.ast import PropertyType
    define(PropertyType)
]]] */
enum class AstPropertyType : u8 {
    Field,
    TupleField,
};

std::string_view to_string(AstPropertyType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import Property
    define(Property)
]]] */
/// Represents the name of a property.
class AstProperty final {
public:
    /// Represents an object field.
    struct Field final {
        InternedString name;

        explicit Field(const InternedString& name_)
            : name(name_) {}
    };

    /// Represents a numeric field within a tuple.
    struct TupleField final {
        u32 index;

        explicit TupleField(const u32& index_)
            : index(index_) {}
    };

    static AstProperty make_field(const InternedString& name);
    static AstProperty make_tuple_field(const u32& index);

    AstProperty(Field field);
    AstProperty(TupleField tuple_field);

    AstPropertyType type() const noexcept { return type_; }

    const Field& as_field() const;
    const TupleField& as_tuple_field() const;

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
    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    AstPropertyType type_;
    union {
        Field field_;
        TupleField tuple_field_;
    };
};
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import ExprType
    define(ExprType)
]]] */
enum class AstExprType : u8 {
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

std::string_view to_string(AstExprType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import ExprData
    define(ExprData)
]]] */
/// Represents the contents of an expression in the abstract syntax tree.
class AstExprData final {
public:
    struct Block final {
        std::vector<AstPtr<AstStmt>> stmts;

        explicit Block(std::vector<AstPtr<AstStmt>> stmts_)
            : stmts(std::move(stmts_)) {}
    };

    struct Unary final {
        UnaryOperator operation;
        AstPtr<AstExpr> inner;

        Unary(const UnaryOperator& operation_, AstPtr<AstExpr> inner_)
            : operation(operation_)
            , inner(std::move(inner_)) {}
    };

    struct Binary final {
        BinaryOperator operation;
        AstPtr<AstExpr> left;
        AstPtr<AstExpr> right;

        Binary(const BinaryOperator& operation_, AstPtr<AstExpr> left_,
            AstPtr<AstExpr> right_)
            : operation(operation_)
            , left(std::move(left_))
            , right(std::move(right_)) {}
    };

    struct Var final {
        InternedString name;

        explicit Var(const InternedString& name_)
            : name(name_) {}
    };

    struct PropertyAccess final {
        AccessType access_type;
        AstPtr<AstExpr> instance;
        AstProperty property;

        PropertyAccess(const AccessType& access_type_,
            AstPtr<AstExpr> instance_, const AstProperty& property_)
            : access_type(access_type_)
            , instance(std::move(instance_))
            , property(property_) {}
    };

    struct ElementAccess final {
        AccessType access_type;
        AstPtr<AstExpr> instance;
        u32 element;

        ElementAccess(const AccessType& access_type_, AstPtr<AstExpr> instance_,
            const u32& element_)
            : access_type(access_type_)
            , instance(std::move(instance_))
            , element(element_) {}
    };

    struct Call final {
        AccessType access_type;
        AstPtr<AstExpr> func;
        std::vector<AstPtr<AstExpr>> args;

        Call(const AccessType& access_type_, AstPtr<AstExpr> func_,
            std::vector<AstPtr<AstExpr>> args_)
            : access_type(access_type_)
            , func(std::move(func_))
            , args(std::move(args_)) {}
    };

    struct If final {
        AstPtr<AstExpr> cond;
        AstPtr<AstExpr> then_branch;
        AstPtr<AstExpr> else_branch;

        If(AstPtr<AstExpr> cond_, AstPtr<AstExpr> then_branch_,
            AstPtr<AstExpr> else_branch_)
            : cond(std::move(cond_))
            , then_branch(std::move(then_branch_))
            , else_branch(std::move(else_branch_)) {}
    };

    struct Return final {
        AstPtr<AstExpr> value;

        explicit Return(AstPtr<AstExpr> value_)
            : value(std::move(value_)) {}
    };

    struct Break final {};

    struct Continue final {};

    struct StringSequence final {
        std::vector<AstPtr<AstExpr>> strings;

        explicit StringSequence(std::vector<AstPtr<AstExpr>> strings_)
            : strings(std::move(strings_)) {}
    };

    struct InterpolatedString final {
        std::vector<AstPtr<AstExpr>> strings;

        explicit InterpolatedString(std::vector<AstPtr<AstExpr>> strings_)
            : strings(std::move(strings_)) {}
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
        std::vector<AstPtr<AstExpr>> items;

        explicit Array(std::vector<AstPtr<AstExpr>> items_)
            : items(std::move(items_)) {}
    };

    struct Tuple final {
        std::vector<AstPtr<AstExpr>> items;

        explicit Tuple(std::vector<AstPtr<AstExpr>> items_)
            : items(std::move(items_)) {}
    };

    struct Set final {
        std::vector<AstPtr<AstExpr>> items;

        explicit Set(std::vector<AstPtr<AstExpr>> items_)
            : items(std::move(items_)) {}
    };

    struct Map final {
        std::vector<AstPtr<AstExpr>> keys;
        std::vector<AstPtr<AstExpr>> values;

        Map(std::vector<AstPtr<AstExpr>> keys_,
            std::vector<AstPtr<AstExpr>> values_)
            : keys(std::move(keys_))
            , values(std::move(values_)) {}
    };

    struct Func final {
        AstPtr<AstDecl> decl;

        explicit Func(AstPtr<AstDecl> decl_)
            : decl(std::move(decl_)) {}
    };

    static AstExprData make_block(std::vector<AstPtr<AstStmt>> stmts);
    static AstExprData
    make_unary(const UnaryOperator& operation, AstPtr<AstExpr> inner);
    static AstExprData make_binary(const BinaryOperator& operation,
        AstPtr<AstExpr> left, AstPtr<AstExpr> right);
    static AstExprData make_var(const InternedString& name);
    static AstExprData make_property_access(const AccessType& access_type,
        AstPtr<AstExpr> instance, const AstProperty& property);
    static AstExprData make_element_access(const AccessType& access_type,
        AstPtr<AstExpr> instance, const u32& element);
    static AstExprData make_call(const AccessType& access_type,
        AstPtr<AstExpr> func, std::vector<AstPtr<AstExpr>> args);
    static AstExprData make_if(AstPtr<AstExpr> cond,
        AstPtr<AstExpr> then_branch, AstPtr<AstExpr> else_branch);
    static AstExprData make_return(AstPtr<AstExpr> value);
    static AstExprData make_break();
    static AstExprData make_continue();
    static AstExprData
    make_string_sequence(std::vector<AstPtr<AstExpr>> strings);
    static AstExprData
    make_interpolated_string(std::vector<AstPtr<AstExpr>> strings);
    static AstExprData make_null();
    static AstExprData make_boolean(const bool& value);
    static AstExprData make_integer(const i64& value);
    static AstExprData make_float(const f64& value);
    static AstExprData make_string(const InternedString& value);
    static AstExprData make_symbol(const InternedString& value);
    static AstExprData make_array(std::vector<AstPtr<AstExpr>> items);
    static AstExprData make_tuple(std::vector<AstPtr<AstExpr>> items);
    static AstExprData make_set(std::vector<AstPtr<AstExpr>> items);
    static AstExprData make_map(
        std::vector<AstPtr<AstExpr>> keys, std::vector<AstPtr<AstExpr>> values);
    static AstExprData make_func(AstPtr<AstDecl> decl);

    AstExprData(Block block);
    AstExprData(Unary unary);
    AstExprData(Binary binary);
    AstExprData(Var var);
    AstExprData(PropertyAccess property_access);
    AstExprData(ElementAccess element_access);
    AstExprData(Call call);
    AstExprData(If i);
    AstExprData(Return ret);
    AstExprData(Break br);
    AstExprData(Continue cont);
    AstExprData(StringSequence string_sequence);
    AstExprData(InterpolatedString interpolated_string);
    AstExprData(Null null);
    AstExprData(Boolean boolean);
    AstExprData(Integer integer);
    AstExprData(Float f);
    AstExprData(String string);
    AstExprData(Symbol symbol);
    AstExprData(Array array);
    AstExprData(Tuple tuple);
    AstExprData(Set set);
    AstExprData(Map map);
    AstExprData(Func func);

    ~AstExprData();

    AstExprData(AstExprData&& other) noexcept;
    AstExprData& operator=(AstExprData&& other) noexcept;

    AstExprType type() const noexcept { return type_; }

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
    void _move_construct_value(AstExprData& other) noexcept;
    void _move_assign_value(AstExprData& other) noexcept;

    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    AstExprType type_;
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
enum class AstStmtType : u8 {
    Empty,
    Assert,
    Decl,
    While,
    For,
    Expr,
};

std::string_view to_string(AstStmtType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import StmtData
    define(StmtData)
]]] */
/// Represents the contents of a statement in the abstract syntax tree.
class AstStmtData final {
public:
    struct Empty final {};

    struct Assert final {
        AstPtr<AstExpr> cond;
        AstPtr<AstExpr> message;

        Assert(AstPtr<AstExpr> cond_, AstPtr<AstExpr> message_)
            : cond(std::move(cond_))
            , message(std::move(message_)) {}
    };

    struct Decl final {
        std::vector<AstPtr<AstDecl>> decls;

        explicit Decl(std::vector<AstPtr<AstDecl>> decls_)
            : decls(std::move(decls_)) {}
    };

    struct While final {
        AstPtr<AstExpr> cond;
        AstPtr<AstExpr> body;

        While(AstPtr<AstExpr> cond_, AstPtr<AstExpr> body_)
            : cond(std::move(cond_))
            , body(std::move(body_)) {}
    };

    struct For final {
        AstPtr<AstStmt> decl;
        AstPtr<AstExpr> cond;
        AstPtr<AstExpr> step;
        AstPtr<AstExpr> body;

        For(AstPtr<AstStmt> decl_, AstPtr<AstExpr> cond_, AstPtr<AstExpr> step_,
            AstPtr<AstExpr> body_)
            : decl(std::move(decl_))
            , cond(std::move(cond_))
            , step(std::move(step_))
            , body(std::move(body_)) {}
    };

    struct Expr final {
        AstPtr<AstExpr> expr;

        explicit Expr(AstPtr<AstExpr> expr_)
            : expr(std::move(expr_)) {}
    };

    static AstStmtData make_empty();
    static AstStmtData
    make_assert(AstPtr<AstExpr> cond, AstPtr<AstExpr> message);
    static AstStmtData make_decl(std::vector<AstPtr<AstDecl>> decls);
    static AstStmtData make_while(AstPtr<AstExpr> cond, AstPtr<AstExpr> body);
    static AstStmtData make_for(AstPtr<AstStmt> decl, AstPtr<AstExpr> cond,
        AstPtr<AstExpr> step, AstPtr<AstExpr> body);
    static AstStmtData make_expr(AstPtr<AstExpr> expr);

    AstStmtData(Empty empty);
    AstStmtData(Assert assert);
    AstStmtData(Decl decl);
    AstStmtData(While w);
    AstStmtData(For f);
    AstStmtData(Expr expr);

    ~AstStmtData();

    AstStmtData(AstStmtData&& other) noexcept;
    AstStmtData& operator=(AstStmtData&& other) noexcept;

    AstStmtType type() const noexcept { return type_; }

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
    void _move_construct_value(AstStmtData& other) noexcept;
    void _move_assign_value(AstStmtData& other) noexcept;

    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    AstStmtType type_;
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
enum class AstDeclType : u8 {
    Func,
    Var,
    Tuple,
    Import,
};

std::string_view to_string(AstDeclType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import DeclData
    define(DeclData)
]]] */
/// Represents the contents of a declaration in the abstract syntax tree.
class AstDeclData final {
public:
    struct Func final {
        InternedString name;
        std::vector<AstPtr<AstDecl>> params;
        AstPtr<AstExpr> body;
        bool body_is_value;

        Func(const InternedString& name_, std::vector<AstPtr<AstDecl>> params_,
            AstPtr<AstExpr> body_, const bool& body_is_value_)
            : name(name_)
            , params(std::move(params_))
            , body(std::move(body_))
            , body_is_value(body_is_value_) {}
    };

    struct Var final {
        InternedString name;
        bool is_const;
        AstPtr<AstExpr> init;

        Var(const InternedString& name_, const bool& is_const_,
            AstPtr<AstExpr> init_)
            : name(name_)
            , is_const(is_const_)
            , init(std::move(init_)) {}
    };

    struct Tuple final {
        std::vector<InternedString> names;
        bool is_const;
        AstPtr<AstExpr> init;

        Tuple(std::vector<InternedString> names_, const bool& is_const_,
            AstPtr<AstExpr> init_)
            : names(std::move(names_))
            , is_const(is_const_)
            , init(std::move(init_)) {}
    };

    struct Import final {
        std::vector<InternedString> path;

        explicit Import(std::vector<InternedString> path_)
            : path(std::move(path_)) {}
    };

    static AstDeclData
    make_func(const InternedString& name, std::vector<AstPtr<AstDecl>> params,
        AstPtr<AstExpr> body, const bool& body_is_value);
    static AstDeclData make_var(
        const InternedString& name, const bool& is_const, AstPtr<AstExpr> init);
    static AstDeclData make_tuple(std::vector<InternedString> names,
        const bool& is_const, AstPtr<AstExpr> init);
    static AstDeclData make_import(std::vector<InternedString> path);

    AstDeclData(Func func);
    AstDeclData(Var var);
    AstDeclData(Tuple tuple);
    AstDeclData(Import import);

    ~AstDeclData();

    AstDeclData(AstDeclData&& other) noexcept;
    AstDeclData& operator=(AstDeclData&& other) noexcept;

    AstDeclType type() const noexcept { return type_; }

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
    void _move_construct_value(AstDeclData& other) noexcept;
    void _move_assign_value(AstDeclData& other) noexcept;

    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    AstDeclType type_;
    union {
        Func func_;
        Var var_;
        Tuple tuple_;
        Import import_;
    };
};
// [[[end]]]

TIRO_DEFINE_ID(AstId, u32);

template<typename Data>
struct NodeBase {
    AstId id;
    Data data;
    SourceReference source;
};

/// Represents an expression in the AST.
struct AstExpr final : NodeBase<AstExprData> {};

/// Represents a statement in the AST.
struct AstStmt final : NodeBase<AstStmtData> {};

/// Represents a declaration in the AST.
struct AstDecl final : NodeBase<AstDeclData> {};

/* [[[cog
    from codegen.unions import implement_inlines
    from codegen.ast import Property, ExprData, StmtData, DeclData
    implement_inlines(Property, ExprData, StmtData, DeclData)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto)
AstProperty::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case AstPropertyType::Field:
        return vis.visit_field(self.field_, std::forward<Args>(args)...);
    case AstPropertyType::TupleField:
        return vis.visit_tuple_field(
            self.tuple_field_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid AstProperty type.");
}

template<typename Self, typename Visitor, typename... Args>
decltype(auto)
AstExprData::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case AstExprType::Block:
        return vis.visit_block(self.block_, std::forward<Args>(args)...);
    case AstExprType::Unary:
        return vis.visit_unary(self.unary_, std::forward<Args>(args)...);
    case AstExprType::Binary:
        return vis.visit_binary(self.binary_, std::forward<Args>(args)...);
    case AstExprType::Var:
        return vis.visit_var(self.var_, std::forward<Args>(args)...);
    case AstExprType::PropertyAccess:
        return vis.visit_property_access(
            self.property_access_, std::forward<Args>(args)...);
    case AstExprType::ElementAccess:
        return vis.visit_element_access(
            self.element_access_, std::forward<Args>(args)...);
    case AstExprType::Call:
        return vis.visit_call(self.call_, std::forward<Args>(args)...);
    case AstExprType::If:
        return vis.visit_if(self.if_, std::forward<Args>(args)...);
    case AstExprType::Return:
        return vis.visit_return(self.return_, std::forward<Args>(args)...);
    case AstExprType::Break:
        return vis.visit_break(self.break_, std::forward<Args>(args)...);
    case AstExprType::Continue:
        return vis.visit_continue(self.continue_, std::forward<Args>(args)...);
    case AstExprType::StringSequence:
        return vis.visit_string_sequence(
            self.string_sequence_, std::forward<Args>(args)...);
    case AstExprType::InterpolatedString:
        return vis.visit_interpolated_string(
            self.interpolated_string_, std::forward<Args>(args)...);
    case AstExprType::Null:
        return vis.visit_null(self.null_, std::forward<Args>(args)...);
    case AstExprType::Boolean:
        return vis.visit_boolean(self.boolean_, std::forward<Args>(args)...);
    case AstExprType::Integer:
        return vis.visit_integer(self.integer_, std::forward<Args>(args)...);
    case AstExprType::Float:
        return vis.visit_float(self.float_, std::forward<Args>(args)...);
    case AstExprType::String:
        return vis.visit_string(self.string_, std::forward<Args>(args)...);
    case AstExprType::Symbol:
        return vis.visit_symbol(self.symbol_, std::forward<Args>(args)...);
    case AstExprType::Array:
        return vis.visit_array(self.array_, std::forward<Args>(args)...);
    case AstExprType::Tuple:
        return vis.visit_tuple(self.tuple_, std::forward<Args>(args)...);
    case AstExprType::Set:
        return vis.visit_set(self.set_, std::forward<Args>(args)...);
    case AstExprType::Map:
        return vis.visit_map(self.map_, std::forward<Args>(args)...);
    case AstExprType::Func:
        return vis.visit_func(self.func_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid AstExprData type.");
}

template<typename Self, typename Visitor, typename... Args>
decltype(auto)
AstStmtData::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case AstStmtType::Empty:
        return vis.visit_empty(self.empty_, std::forward<Args>(args)...);
    case AstStmtType::Assert:
        return vis.visit_assert(self.assert_, std::forward<Args>(args)...);
    case AstStmtType::Decl:
        return vis.visit_decl(self.decl_, std::forward<Args>(args)...);
    case AstStmtType::While:
        return vis.visit_while(self.while_, std::forward<Args>(args)...);
    case AstStmtType::For:
        return vis.visit_for(self.for_, std::forward<Args>(args)...);
    case AstStmtType::Expr:
        return vis.visit_expr(self.expr_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid AstStmtData type.");
}

template<typename Self, typename Visitor, typename... Args>
decltype(auto)
AstDeclData::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case AstDeclType::Func:
        return vis.visit_func(self.func_, std::forward<Args>(args)...);
    case AstDeclType::Var:
        return vis.visit_var(self.var_, std::forward<Args>(args)...);
    case AstDeclType::Tuple:
        return vis.visit_tuple(self.tuple_, std::forward<Args>(args)...);
    case AstDeclType::Import:
        return vis.visit_import(self.import_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid AstDeclData type.");
}
// [[[end]]]

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::AccessType);

TIRO_ENABLE_FREE_TO_STRING(tiro::AstPropertyType);
TIRO_ENABLE_FREE_TO_STRING(tiro::AstExprType);
TIRO_ENABLE_FREE_TO_STRING(tiro::AstStmtType);
TIRO_ENABLE_FREE_TO_STRING(tiro::AstDeclType);

TIRO_ENABLE_MEMBER_FORMAT(tiro::AstProperty);
TIRO_ENABLE_MEMBER_FORMAT(tiro::AstExprData);
TIRO_ENABLE_MEMBER_FORMAT(tiro::AstStmtData);
TIRO_ENABLE_MEMBER_FORMAT(tiro::AstDeclData);

#endif // TIRO_AST_AST_HPP
