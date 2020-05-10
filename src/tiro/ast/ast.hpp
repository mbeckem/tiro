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

TIRO_DEFINE_ID(AstId, u32);

class AstIds final {
public:
    AstIds();

    AstId generate();

private:
    u32 next_id_;
};

class AstNodeBase {
public:
    AstId id;
    SourceReference source;
    bool error = false;

    AstNodeBase(AstId id_, const SourceReference& source_)
        : id(id_)
        , source(source_) {}

    AstNodeBase(const AstNodeBase&) = default;
    AstNodeBase(AstNodeBase&&) noexcept = default;

    AstNodeBase& operator=(const AstNodeBase&) = default;
    AstNodeBase& operator=(AstNodeBase&&) noexcept = default;

protected:
    // Prevent delete on base class w/o virtual destructor
    ~AstNodeBase();
};

template<typename Data>
class AstNodeFromData : public AstNodeBase {
public:
    AstNodeFromData(AstId id, const SourceReference& source, const Data& data)
        : AstNodeBase(id, source)
        , Data(data) {}

    AstNodeFromData(AstId id, const SourceReference& source, Data&& data)
        : AstNodeBase(id, source)
        , Data(std::move(data)) {}
};

#define TIRO_AST_NODE(Name, Data)                     \
    class Name final : public AstNodeFromData<Data> { \
    public:                                           \
        using DataType = Data;                        \
        using AstNodeFromData::AstNodeFromData;       \
    };

template<typename T, typename... Args>
T make_node(AstId id, const SourceReference& source, Args&&... args) {
    return T(id, source, T::DataType(std::forward<Args>(args)...));
}

enum class AccessType : u8 {
    Normal,
    Optional, // Null propagation, e.g. `instance?.member`.
};

std::string_view to_string(AccessType access);

/// Represents the declaration of a function parameter.
class AstParamDecl final : public AstNodeBase {
public:
    AstParamDecl(AstId id, const SourceReference& source, InternedString name_)
        : AstNodeBase(id, source)
        , name(name_) {}

    InternedString name;
};

/// Represents the declaration of a function.
class AstFuncDecl final : public AstNodeBase {
public:
    AstFuncDecl(AstId id, const SourceReference& source, InternedString name_,
        std::vector<AstParamDecl> params_, AstPtr<AstExpr> body_,
        bool body_is_value_)
        : AstNodeBase(id, source)
        , name(name_)
        , params(std::move(params_))
        , body(std::move(body_))
        , body_is_value(body_is_value_) {}

    InternedString name;
    std::vector<AstParamDecl> params;
    AstPtr<AstExpr> body;
    bool body_is_value = false;
};

/* [[[cog
    from codegen.unions import define
    from codegen.ast import ItemData
    define(ItemData.tag)
]]] */
enum class AstItemType : u8 {
    Import,
    Func,
    Var,
};

std::string_view to_string(AstItemType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import ItemData
    define(ItemData)
]]] */
/// Represents the contents of a toplevel item.
class AstItemData {
public:
    struct Import final {
        InternedString name;
        std::vector<InternedString> path;

        Import(InternedString name_, std::vector<InternedString> path_)
            : name(std::move(name_))
            , path(std::move(path_)) {}
    };

    struct Func final {
        AstFuncDecl decl;

        explicit Func(AstFuncDecl decl_)
            : decl(std::move(decl_)) {}
    };

    struct Var final {
        std::vector<AstBinding> bindings;

        explicit Var(std::vector<AstBinding> bindings_)
            : bindings(std::move(bindings_)) {}
    };

    static AstItemData
    make_import(InternedString name, std::vector<InternedString> path);
    static AstItemData make_func(AstFuncDecl decl);
    static AstItemData make_var(std::vector<AstBinding> bindings);

    AstItemData(Import import);
    AstItemData(Func func);
    AstItemData(Var var);

    ~AstItemData();

    AstItemData(AstItemData&& other) noexcept;
    AstItemData& operator=(AstItemData&& other) noexcept;

    AstItemType type() const noexcept { return type_; }

    const Import& as_import() const;
    const Func& as_func() const;
    const Var& as_var() const;

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
    void _move_construct_value(AstItemData& other) noexcept;
    void _move_assign_value(AstItemData& other) noexcept;

    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    AstItemType type_;
    union {
        Import import_;
        Func func_;
        Var var_;
    };
};
// [[[end]]]

TIRO_AST_NODE(AstItem, AstItemData);

/* [[[cog
    from codegen.unions import define
    from codegen.ast import BindingData
    define(BindingData.tag)
]]] */
enum class AstBindingType : u8 {
    Var,
    Tuple,
};

std::string_view to_string(AstBindingType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import BindingData
    define(BindingData)
]]] */
/// Represents a binding of values to names.
class AstBindingData {
public:
    struct Var final {
        InternedString name;
        bool is_const;
        AstPtr<AstExpr> init;

        Var(InternedString name_, bool is_const_, AstPtr<AstExpr> init_)
            : name(std::move(name_))
            , is_const(std::move(is_const_))
            , init(std::move(init_)) {}
    };

    struct Tuple final {
        std::vector<InternedString> names;
        bool is_const;
        AstPtr<AstExpr> init;

        Tuple(std::vector<InternedString> names_, bool is_const_,
            AstPtr<AstExpr> init_)
            : names(std::move(names_))
            , is_const(std::move(is_const_))
            , init(std::move(init_)) {}
    };

    static AstBindingData
    make_var(InternedString name, bool is_const, AstPtr<AstExpr> init);
    static AstBindingData make_tuple(
        std::vector<InternedString> names, bool is_const, AstPtr<AstExpr> init);

    AstBindingData(Var var);
    AstBindingData(Tuple tuple);

    ~AstBindingData();

    AstBindingData(AstBindingData&& other) noexcept;
    AstBindingData& operator=(AstBindingData&& other) noexcept;

    AstBindingType type() const noexcept { return type_; }

    const Var& as_var() const;
    const Tuple& as_tuple() const;

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
    void _move_construct_value(AstBindingData& other) noexcept;
    void _move_assign_value(AstBindingData& other) noexcept;

    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    AstBindingType type_;
    union {
        Var var_;
        Tuple tuple_;
    };
};
// [[[end]]]

TIRO_AST_NODE(AstBinding, AstBindingData);

/* [[[cog
    from codegen.unions import define
    from codegen.ast import PropertyData
    define(PropertyData.tag)
]]] */
enum class AstPropertyType : u8 {
    Field,
    TupleField,
};

std::string_view to_string(AstPropertyType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import PropertyData
    define(PropertyData)
]]] */
/// Represents the name of a property.
class AstPropertyData {
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

    static AstPropertyData make_field(const InternedString& name);
    static AstPropertyData make_tuple_field(const u32& index);

    AstPropertyData(Field field);
    AstPropertyData(TupleField tuple_field);

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

TIRO_AST_NODE(AstProperty, AstPropertyData);

/* [[[cog
    from codegen.unions import define
    from codegen.ast import ExprData
    define(ExprData.tag)
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
class AstExprData {
public:
    struct Block final {
        std::vector<AstPtr<AstStmt>> stmts;

        explicit Block(std::vector<AstPtr<AstStmt>> stmts_)
            : stmts(std::move(stmts_)) {}
    };

    struct Unary final {
        UnaryOperator operation;
        AstPtr<AstExpr> inner;

        Unary(UnaryOperator operation_, AstPtr<AstExpr> inner_)
            : operation(std::move(operation_))
            , inner(std::move(inner_)) {}
    };

    struct Binary final {
        BinaryOperator operation;
        AstPtr<AstExpr> left;
        AstPtr<AstExpr> right;

        Binary(BinaryOperator operation_, AstPtr<AstExpr> left_,
            AstPtr<AstExpr> right_)
            : operation(std::move(operation_))
            , left(std::move(left_))
            , right(std::move(right_)) {}
    };

    struct Var final {
        InternedString name;

        explicit Var(InternedString name_)
            : name(std::move(name_)) {}
    };

    struct PropertyAccess final {
        AccessType access_type;
        AstPtr<AstExpr> instance;
        AstProperty property;

        PropertyAccess(AccessType access_type_, AstPtr<AstExpr> instance_,
            AstProperty property_)
            : access_type(std::move(access_type_))
            , instance(std::move(instance_))
            , property(std::move(property_)) {}
    };

    struct ElementAccess final {
        AccessType access_type;
        AstPtr<AstExpr> instance;
        AstPtr<AstExpr> element;

        ElementAccess(AccessType access_type_, AstPtr<AstExpr> instance_,
            AstPtr<AstExpr> element_)
            : access_type(std::move(access_type_))
            , instance(std::move(instance_))
            , element(std::move(element_)) {}
    };

    struct Call final {
        AccessType access_type;
        AstPtr<AstExpr> func;
        std::vector<AstPtr<AstExpr>> args;

        Call(AccessType access_type_, AstPtr<AstExpr> func_,
            std::vector<AstPtr<AstExpr>> args_)
            : access_type(std::move(access_type_))
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

        explicit Boolean(bool value_)
            : value(std::move(value_)) {}
    };

    struct Integer final {
        i64 value;

        explicit Integer(i64 value_)
            : value(std::move(value_)) {}
    };

    struct Float final {
        f64 value;

        explicit Float(f64 value_)
            : value(std::move(value_)) {}
    };

    struct String final {
        InternedString value;

        explicit String(InternedString value_)
            : value(std::move(value_)) {}
    };

    struct Symbol final {
        InternedString value;

        explicit Symbol(InternedString value_)
            : value(std::move(value_)) {}
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
        AstFuncDecl decl;

        explicit Func(AstFuncDecl decl_)
            : decl(std::move(decl_)) {}
    };

    static AstExprData make_block(std::vector<AstPtr<AstStmt>> stmts);
    static AstExprData
    make_unary(UnaryOperator operation, AstPtr<AstExpr> inner);
    static AstExprData make_binary(
        BinaryOperator operation, AstPtr<AstExpr> left, AstPtr<AstExpr> right);
    static AstExprData make_var(InternedString name);
    static AstExprData make_property_access(
        AccessType access_type, AstPtr<AstExpr> instance, AstProperty property);
    static AstExprData make_element_access(AccessType access_type,
        AstPtr<AstExpr> instance, AstPtr<AstExpr> element);
    static AstExprData make_call(AccessType access_type, AstPtr<AstExpr> func,
        std::vector<AstPtr<AstExpr>> args);
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
    static AstExprData make_boolean(bool value);
    static AstExprData make_integer(i64 value);
    static AstExprData make_float(f64 value);
    static AstExprData make_string(InternedString value);
    static AstExprData make_symbol(InternedString value);
    static AstExprData make_array(std::vector<AstPtr<AstExpr>> items);
    static AstExprData make_tuple(std::vector<AstPtr<AstExpr>> items);
    static AstExprData make_set(std::vector<AstPtr<AstExpr>> items);
    static AstExprData make_map(
        std::vector<AstPtr<AstExpr>> keys, std::vector<AstPtr<AstExpr>> values);
    static AstExprData make_func(AstFuncDecl decl);

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

TIRO_AST_NODE(AstExpr, AstExprData);

/* [[[cog
    from codegen.unions import define
    from codegen.ast import StmtData
    define(StmtData.tag)
]]] */
enum class AstStmtType : u8 {
    Empty,
    Item,
    Assert,
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
class AstStmtData {
public:
    struct Empty final {};

    struct Item final {
        AstPtr<AstItem> item;

        explicit Item(AstPtr<AstItem> item_)
            : item(std::move(item_)) {}
    };

    struct Assert final {
        AstPtr<AstExpr> cond;
        AstPtr<AstExpr> message;

        Assert(AstPtr<AstExpr> cond_, AstPtr<AstExpr> message_)
            : cond(std::move(cond_))
            , message(std::move(message_)) {}
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
    static AstStmtData make_item(AstPtr<AstItem> item);
    static AstStmtData
    make_assert(AstPtr<AstExpr> cond, AstPtr<AstExpr> message);
    static AstStmtData make_while(AstPtr<AstExpr> cond, AstPtr<AstExpr> body);
    static AstStmtData make_for(AstPtr<AstStmt> decl, AstPtr<AstExpr> cond,
        AstPtr<AstExpr> step, AstPtr<AstExpr> body);
    static AstStmtData make_expr(AstPtr<AstExpr> expr);

    AstStmtData(Empty empty);
    AstStmtData(Item item);
    AstStmtData(Assert assert);
    AstStmtData(While w);
    AstStmtData(For f);
    AstStmtData(Expr expr);

    ~AstStmtData();

    AstStmtData(AstStmtData&& other) noexcept;
    AstStmtData& operator=(AstStmtData&& other) noexcept;

    AstStmtType type() const noexcept { return type_; }

    const Empty& as_empty() const;
    const Item& as_item() const;
    const Assert& as_assert() const;
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
        Item item_;
        Assert assert_;
        While while_;
        For for_;
        Expr expr_;
    };
};
// [[[end]]]

TIRO_AST_NODE(AstStmt, AstStmtData);

class AstFile final : public AstNodeBase {
public:
    AstFile(
        AstId id, const SourceReference& source, std::vector<AstItem> items_)
        : AstNodeBase(id, source)
        , items(std::move(items_)) {}

    std::vector<AstItem> items;
};

/* [[[cog
    from codegen.unions import implement_inlines
    from codegen.ast import ItemData, BindingData, PropertyData, ExprData, StmtData
    implement_inlines(ItemData, BindingData, PropertyData, ExprData, StmtData)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto)
AstItemData::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case AstItemType::Import:
        return vis.visit_import(self.import_, std::forward<Args>(args)...);
    case AstItemType::Func:
        return vis.visit_func(self.func_, std::forward<Args>(args)...);
    case AstItemType::Var:
        return vis.visit_var(self.var_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid AstItemData type.");
}

template<typename Self, typename Visitor, typename... Args>
decltype(auto)
AstBindingData::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case AstBindingType::Var:
        return vis.visit_var(self.var_, std::forward<Args>(args)...);
    case AstBindingType::Tuple:
        return vis.visit_tuple(self.tuple_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid AstBindingData type.");
}

template<typename Self, typename Visitor, typename... Args>
decltype(auto)
AstPropertyData::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case AstPropertyType::Field:
        return vis.visit_field(self.field_, std::forward<Args>(args)...);
    case AstPropertyType::TupleField:
        return vis.visit_tuple_field(
            self.tuple_field_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid AstPropertyData type.");
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
    case AstStmtType::Item:
        return vis.visit_item(self.item_, std::forward<Args>(args)...);
    case AstStmtType::Assert:
        return vis.visit_assert(self.assert_, std::forward<Args>(args)...);
    case AstStmtType::While:
        return vis.visit_while(self.while_, std::forward<Args>(args)...);
    case AstStmtType::For:
        return vis.visit_for(self.for_, std::forward<Args>(args)...);
    case AstStmtType::Expr:
        return vis.visit_expr(self.expr_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid AstStmtData type.");
}
// [[[end]]]

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::AccessType);

TIRO_ENABLE_FREE_TO_STRING(tiro::AstItemType);
TIRO_ENABLE_FREE_TO_STRING(tiro::AstBindingType);
TIRO_ENABLE_FREE_TO_STRING(tiro::AstPropertyType);
TIRO_ENABLE_FREE_TO_STRING(tiro::AstExprType);
TIRO_ENABLE_FREE_TO_STRING(tiro::AstStmtType);

#endif // TIRO_AST_AST_HPP
