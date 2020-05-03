#include "tiro/ast/ast.hpp"

#include <new>

namespace tiro {

std::string_view to_string(AccessType access) {
    switch (access) {
    case AccessType::Normal:
        return "Normal";
    case AccessType::Optional:
        return "Optional";
    }
    TIRO_UNREACHABLE("Invalid access type.");
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ast import Property
    implement(Property.tag, Property)
]]] */
std::string_view to_string(AstPropertyType type) {
    switch (type) {
    case AstPropertyType::Field:
        return "Field";
    case AstPropertyType::TupleField:
        return "TupleField";
    }
    TIRO_UNREACHABLE("Invalid AstPropertyType.");
}

AstProperty AstProperty::make_field(const InternedString& name) {
    return {Field{name}};
}

AstProperty AstProperty::make_tuple_field(const u32& index) {
    return {TupleField{index}};
}

AstProperty::AstProperty(Field field)
    : type_(AstPropertyType::Field)
    , field_(std::move(field)) {}

AstProperty::AstProperty(TupleField tuple_field)
    : type_(AstPropertyType::TupleField)
    , tuple_field_(std::move(tuple_field)) {}

const AstProperty::Field& AstProperty::as_field() const {
    TIRO_DEBUG_ASSERT(type_ == AstPropertyType::Field,
        "Bad member access on AstProperty: not a Field.");
    return field_;
}

const AstProperty::TupleField& AstProperty::as_tuple_field() const {
    TIRO_DEBUG_ASSERT(type_ == AstPropertyType::TupleField,
        "Bad member access on AstProperty: not a TupleField.");
    return tuple_field_;
}

// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ast import ExprData
    implement(ExprData.tag, ExprData)
]]] */
std::string_view to_string(AstExprType type) {
    switch (type) {
    case AstExprType::Block:
        return "Block";
    case AstExprType::Unary:
        return "Unary";
    case AstExprType::Binary:
        return "Binary";
    case AstExprType::Var:
        return "Var";
    case AstExprType::PropertyAccess:
        return "PropertyAccess";
    case AstExprType::ElementAccess:
        return "ElementAccess";
    case AstExprType::Call:
        return "Call";
    case AstExprType::If:
        return "If";
    case AstExprType::Return:
        return "Return";
    case AstExprType::Break:
        return "Break";
    case AstExprType::Continue:
        return "Continue";
    case AstExprType::StringSequence:
        return "StringSequence";
    case AstExprType::InterpolatedString:
        return "InterpolatedString";
    case AstExprType::Null:
        return "Null";
    case AstExprType::Boolean:
        return "Boolean";
    case AstExprType::Integer:
        return "Integer";
    case AstExprType::Float:
        return "Float";
    case AstExprType::String:
        return "String";
    case AstExprType::Symbol:
        return "Symbol";
    case AstExprType::Array:
        return "Array";
    case AstExprType::Tuple:
        return "Tuple";
    case AstExprType::Set:
        return "Set";
    case AstExprType::Map:
        return "Map";
    case AstExprType::Func:
        return "Func";
    }
    TIRO_UNREACHABLE("Invalid AstExprType.");
}

AstExprData AstExprData::make_block(std::vector<AstPtr<AstStmt>> stmts) {
    return {Block{std::move(stmts)}};
}

AstExprData
AstExprData::make_unary(const UnaryOperator& operation, AstPtr<AstExpr> inner) {
    return {Unary{operation, std::move(inner)}};
}

AstExprData AstExprData::make_binary(const BinaryOperator& operation,
    AstPtr<AstExpr> left, AstPtr<AstExpr> right) {
    return {Binary{operation, std::move(left), std::move(right)}};
}

AstExprData AstExprData::make_var(const InternedString& name) {
    return {Var{name}};
}

AstExprData AstExprData::make_property_access(const AccessType& access_type,
    AstPtr<AstExpr> instance, const AstProperty& property) {
    return {PropertyAccess{access_type, std::move(instance), property}};
}

AstExprData AstExprData::make_element_access(const AccessType& access_type,
    AstPtr<AstExpr> instance, const u32& element) {
    return {ElementAccess{access_type, std::move(instance), element}};
}

AstExprData AstExprData::make_call(const AccessType& access_type,
    AstPtr<AstExpr> func, std::vector<AstPtr<AstExpr>> args) {
    return {Call{access_type, std::move(func), std::move(args)}};
}

AstExprData AstExprData::make_if(AstPtr<AstExpr> cond,
    AstPtr<AstExpr> then_branch, AstPtr<AstExpr> else_branch) {
    return {
        If{std::move(cond), std::move(then_branch), std::move(else_branch)}};
}

AstExprData AstExprData::make_return(AstPtr<AstExpr> value) {
    return {Return{std::move(value)}};
}

AstExprData AstExprData::make_break() {
    return {Break{}};
}

AstExprData AstExprData::make_continue() {
    return {Continue{}};
}

AstExprData
AstExprData::make_string_sequence(std::vector<AstPtr<AstExpr>> strings) {
    return {StringSequence{std::move(strings)}};
}

AstExprData
AstExprData::make_interpolated_string(std::vector<AstPtr<AstExpr>> strings) {
    return {InterpolatedString{std::move(strings)}};
}

AstExprData AstExprData::make_null() {
    return {Null{}};
}

AstExprData AstExprData::make_boolean(const bool& value) {
    return {Boolean{value}};
}

AstExprData AstExprData::make_integer(const i64& value) {
    return {Integer{value}};
}

AstExprData AstExprData::make_float(const f64& value) {
    return {Float{value}};
}

AstExprData AstExprData::make_string(const InternedString& value) {
    return {String{value}};
}

AstExprData AstExprData::make_symbol(const InternedString& value) {
    return {Symbol{value}};
}

AstExprData AstExprData::make_array(std::vector<AstPtr<AstExpr>> items) {
    return {Array{std::move(items)}};
}

AstExprData AstExprData::make_tuple(std::vector<AstPtr<AstExpr>> items) {
    return {Tuple{std::move(items)}};
}

AstExprData AstExprData::make_set(std::vector<AstPtr<AstExpr>> items) {
    return {Set{std::move(items)}};
}

AstExprData AstExprData::make_map(
    std::vector<AstPtr<AstExpr>> keys, std::vector<AstPtr<AstExpr>> values) {
    return {Map{std::move(keys), std::move(values)}};
}

AstExprData AstExprData::make_func(AstPtr<AstDecl> decl) {
    return {Func{std::move(decl)}};
}

AstExprData::AstExprData(Block block)
    : type_(AstExprType::Block)
    , block_(std::move(block)) {}

AstExprData::AstExprData(Unary unary)
    : type_(AstExprType::Unary)
    , unary_(std::move(unary)) {}

AstExprData::AstExprData(Binary binary)
    : type_(AstExprType::Binary)
    , binary_(std::move(binary)) {}

AstExprData::AstExprData(Var var)
    : type_(AstExprType::Var)
    , var_(std::move(var)) {}

AstExprData::AstExprData(PropertyAccess property_access)
    : type_(AstExprType::PropertyAccess)
    , property_access_(std::move(property_access)) {}

AstExprData::AstExprData(ElementAccess element_access)
    : type_(AstExprType::ElementAccess)
    , element_access_(std::move(element_access)) {}

AstExprData::AstExprData(Call call)
    : type_(AstExprType::Call)
    , call_(std::move(call)) {}

AstExprData::AstExprData(If i)
    : type_(AstExprType::If)
    , if_(std::move(i)) {}

AstExprData::AstExprData(Return ret)
    : type_(AstExprType::Return)
    , return_(std::move(ret)) {}

AstExprData::AstExprData(Break br)
    : type_(AstExprType::Break)
    , break_(std::move(br)) {}

AstExprData::AstExprData(Continue cont)
    : type_(AstExprType::Continue)
    , continue_(std::move(cont)) {}

AstExprData::AstExprData(StringSequence string_sequence)
    : type_(AstExprType::StringSequence)
    , string_sequence_(std::move(string_sequence)) {}

AstExprData::AstExprData(InterpolatedString interpolated_string)
    : type_(AstExprType::InterpolatedString)
    , interpolated_string_(std::move(interpolated_string)) {}

AstExprData::AstExprData(Null null)
    : type_(AstExprType::Null)
    , null_(std::move(null)) {}

AstExprData::AstExprData(Boolean boolean)
    : type_(AstExprType::Boolean)
    , boolean_(std::move(boolean)) {}

AstExprData::AstExprData(Integer integer)
    : type_(AstExprType::Integer)
    , integer_(std::move(integer)) {}

AstExprData::AstExprData(Float f)
    : type_(AstExprType::Float)
    , float_(std::move(f)) {}

AstExprData::AstExprData(String string)
    : type_(AstExprType::String)
    , string_(std::move(string)) {}

AstExprData::AstExprData(Symbol symbol)
    : type_(AstExprType::Symbol)
    , symbol_(std::move(symbol)) {}

AstExprData::AstExprData(Array array)
    : type_(AstExprType::Array)
    , array_(std::move(array)) {}

AstExprData::AstExprData(Tuple tuple)
    : type_(AstExprType::Tuple)
    , tuple_(std::move(tuple)) {}

AstExprData::AstExprData(Set set)
    : type_(AstExprType::Set)
    , set_(std::move(set)) {}

AstExprData::AstExprData(Map map)
    : type_(AstExprType::Map)
    , map_(std::move(map)) {}

AstExprData::AstExprData(Func func)
    : type_(AstExprType::Func)
    , func_(std::move(func)) {}

AstExprData::~AstExprData() {
    _destroy_value();
}

static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Block> && std::is_nothrow_move_assignable_v<AstExprData::Block>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Unary> && std::is_nothrow_move_assignable_v<AstExprData::Unary>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Binary> && std::is_nothrow_move_assignable_v<AstExprData::Binary>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Var> && std::is_nothrow_move_assignable_v<AstExprData::Var>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            PropertyAccess> && std::is_nothrow_move_assignable_v<AstExprData::PropertyAccess>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            ElementAccess> && std::is_nothrow_move_assignable_v<AstExprData::ElementAccess>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Call> && std::is_nothrow_move_assignable_v<AstExprData::Call>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<
        AstExprData::If> && std::is_nothrow_move_assignable_v<AstExprData::If>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Return> && std::is_nothrow_move_assignable_v<AstExprData::Return>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Break> && std::is_nothrow_move_assignable_v<AstExprData::Break>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Continue> && std::is_nothrow_move_assignable_v<AstExprData::Continue>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            StringSequence> && std::is_nothrow_move_assignable_v<AstExprData::StringSequence>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            InterpolatedString> && std::is_nothrow_move_assignable_v<AstExprData::InterpolatedString>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Null> && std::is_nothrow_move_assignable_v<AstExprData::Null>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Boolean> && std::is_nothrow_move_assignable_v<AstExprData::Boolean>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Integer> && std::is_nothrow_move_assignable_v<AstExprData::Integer>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Float> && std::is_nothrow_move_assignable_v<AstExprData::Float>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            String> && std::is_nothrow_move_assignable_v<AstExprData::String>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Symbol> && std::is_nothrow_move_assignable_v<AstExprData::Symbol>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Array> && std::is_nothrow_move_assignable_v<AstExprData::Array>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Tuple> && std::is_nothrow_move_assignable_v<AstExprData::Tuple>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Set> && std::is_nothrow_move_assignable_v<AstExprData::Set>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Map> && std::is_nothrow_move_assignable_v<AstExprData::Map>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstExprData::
            Func> && std::is_nothrow_move_assignable_v<AstExprData::Func>,
    "Only nothrow movable types are supported in generated unions.");

AstExprData::AstExprData(AstExprData&& other) noexcept
    : type_(other.type()) {
    _move_construct_value(other);
}

AstExprData& AstExprData::operator=(AstExprData&& other) noexcept {
    TIRO_DEBUG_ASSERT(this != &other, "Self move assignement is invalid.");
    if (type() == other.type()) {
        _move_assign_value(other);
    } else {
        _destroy_value();
        _move_construct_value(other);
        type_ = other.type();
    }
    return *this;
}

const AstExprData::Block& AstExprData::as_block() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Block,
        "Bad member access on AstExprData: not a Block.");
    return block_;
}

const AstExprData::Unary& AstExprData::as_unary() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Unary,
        "Bad member access on AstExprData: not a Unary.");
    return unary_;
}

const AstExprData::Binary& AstExprData::as_binary() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Binary,
        "Bad member access on AstExprData: not a Binary.");
    return binary_;
}

const AstExprData::Var& AstExprData::as_var() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Var,
        "Bad member access on AstExprData: not a Var.");
    return var_;
}

const AstExprData::PropertyAccess& AstExprData::as_property_access() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::PropertyAccess,
        "Bad member access on AstExprData: not a PropertyAccess.");
    return property_access_;
}

const AstExprData::ElementAccess& AstExprData::as_element_access() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::ElementAccess,
        "Bad member access on AstExprData: not a ElementAccess.");
    return element_access_;
}

const AstExprData::Call& AstExprData::as_call() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Call,
        "Bad member access on AstExprData: not a Call.");
    return call_;
}

const AstExprData::If& AstExprData::as_if() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::If,
        "Bad member access on AstExprData: not a If.");
    return if_;
}

const AstExprData::Return& AstExprData::as_return() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Return,
        "Bad member access on AstExprData: not a Return.");
    return return_;
}

const AstExprData::Break& AstExprData::as_break() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Break,
        "Bad member access on AstExprData: not a Break.");
    return break_;
}

const AstExprData::Continue& AstExprData::as_continue() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Continue,
        "Bad member access on AstExprData: not a Continue.");
    return continue_;
}

const AstExprData::StringSequence& AstExprData::as_string_sequence() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::StringSequence,
        "Bad member access on AstExprData: not a StringSequence.");
    return string_sequence_;
}

const AstExprData::InterpolatedString&
AstExprData::as_interpolated_string() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::InterpolatedString,
        "Bad member access on AstExprData: not a InterpolatedString.");
    return interpolated_string_;
}

const AstExprData::Null& AstExprData::as_null() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Null,
        "Bad member access on AstExprData: not a Null.");
    return null_;
}

const AstExprData::Boolean& AstExprData::as_boolean() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Boolean,
        "Bad member access on AstExprData: not a Boolean.");
    return boolean_;
}

const AstExprData::Integer& AstExprData::as_integer() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Integer,
        "Bad member access on AstExprData: not a Integer.");
    return integer_;
}

const AstExprData::Float& AstExprData::as_float() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Float,
        "Bad member access on AstExprData: not a Float.");
    return float_;
}

const AstExprData::String& AstExprData::as_string() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::String,
        "Bad member access on AstExprData: not a String.");
    return string_;
}

const AstExprData::Symbol& AstExprData::as_symbol() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Symbol,
        "Bad member access on AstExprData: not a Symbol.");
    return symbol_;
}

const AstExprData::Array& AstExprData::as_array() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Array,
        "Bad member access on AstExprData: not a Array.");
    return array_;
}

const AstExprData::Tuple& AstExprData::as_tuple() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Tuple,
        "Bad member access on AstExprData: not a Tuple.");
    return tuple_;
}

const AstExprData::Set& AstExprData::as_set() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Set,
        "Bad member access on AstExprData: not a Set.");
    return set_;
}

const AstExprData::Map& AstExprData::as_map() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Map,
        "Bad member access on AstExprData: not a Map.");
    return map_;
}

const AstExprData::Func& AstExprData::as_func() const {
    TIRO_DEBUG_ASSERT(type_ == AstExprType::Func,
        "Bad member access on AstExprData: not a Func.");
    return func_;
}

void AstExprData::_destroy_value() noexcept {
    struct DestroyVisitor {
        void visit_block(Block& block) { block.~Block(); }

        void visit_unary(Unary& unary) { unary.~Unary(); }

        void visit_binary(Binary& binary) { binary.~Binary(); }

        void visit_var(Var& var) { var.~Var(); }

        void visit_property_access(PropertyAccess& property_access) {
            property_access.~PropertyAccess();
        }

        void visit_element_access(ElementAccess& element_access) {
            element_access.~ElementAccess();
        }

        void visit_call(Call& call) { call.~Call(); }

        void visit_if(If& i) { i.~If(); }

        void visit_return(Return& ret) { ret.~Return(); }

        void visit_break(Break& br) { br.~Break(); }

        void visit_continue(Continue& cont) { cont.~Continue(); }

        void visit_string_sequence(StringSequence& string_sequence) {
            string_sequence.~StringSequence();
        }

        void
        visit_interpolated_string(InterpolatedString& interpolated_string) {
            interpolated_string.~InterpolatedString();
        }

        void visit_null(Null& null) { null.~Null(); }

        void visit_boolean(Boolean& boolean) { boolean.~Boolean(); }

        void visit_integer(Integer& integer) { integer.~Integer(); }

        void visit_float(Float& f) { f.~Float(); }

        void visit_string(String& string) { string.~String(); }

        void visit_symbol(Symbol& symbol) { symbol.~Symbol(); }

        void visit_array(Array& array) { array.~Array(); }

        void visit_tuple(Tuple& tuple) { tuple.~Tuple(); }

        void visit_set(Set& set) { set.~Set(); }

        void visit_map(Map& map) { map.~Map(); }

        void visit_func(Func& func) { func.~Func(); }
    };
    visit(DestroyVisitor{});
}

void AstExprData::_move_construct_value(AstExprData& other) noexcept {
    struct ConstructVisitor {
        AstExprData* self;

        void visit_block(Block& block) {
            new (&self->block_) Block(std::move(block));
        }

        void visit_unary(Unary& unary) {
            new (&self->unary_) Unary(std::move(unary));
        }

        void visit_binary(Binary& binary) {
            new (&self->binary_) Binary(std::move(binary));
        }

        void visit_var(Var& var) { new (&self->var_) Var(std::move(var)); }

        void visit_property_access(PropertyAccess& property_access) {
            new (&self->property_access_)
                PropertyAccess(std::move(property_access));
        }

        void visit_element_access(ElementAccess& element_access) {
            new (&self->element_access_)
                ElementAccess(std::move(element_access));
        }

        void visit_call(Call& call) {
            new (&self->call_) Call(std::move(call));
        }

        void visit_if(If& i) { new (&self->if_) If(std::move(i)); }

        void visit_return(Return& ret) {
            new (&self->return_) Return(std::move(ret));
        }

        void visit_break(Break& br) {
            new (&self->break_) Break(std::move(br));
        }

        void visit_continue(Continue& cont) {
            new (&self->continue_) Continue(std::move(cont));
        }

        void visit_string_sequence(StringSequence& string_sequence) {
            new (&self->string_sequence_)
                StringSequence(std::move(string_sequence));
        }

        void
        visit_interpolated_string(InterpolatedString& interpolated_string) {
            new (&self->interpolated_string_)
                InterpolatedString(std::move(interpolated_string));
        }

        void visit_null(Null& null) {
            new (&self->null_) Null(std::move(null));
        }

        void visit_boolean(Boolean& boolean) {
            new (&self->boolean_) Boolean(std::move(boolean));
        }

        void visit_integer(Integer& integer) {
            new (&self->integer_) Integer(std::move(integer));
        }

        void visit_float(Float& f) { new (&self->float_) Float(std::move(f)); }

        void visit_string(String& string) {
            new (&self->string_) String(std::move(string));
        }

        void visit_symbol(Symbol& symbol) {
            new (&self->symbol_) Symbol(std::move(symbol));
        }

        void visit_array(Array& array) {
            new (&self->array_) Array(std::move(array));
        }

        void visit_tuple(Tuple& tuple) {
            new (&self->tuple_) Tuple(std::move(tuple));
        }

        void visit_set(Set& set) { new (&self->set_) Set(std::move(set)); }

        void visit_map(Map& map) { new (&self->map_) Map(std::move(map)); }

        void visit_func(Func& func) {
            new (&self->func_) Func(std::move(func));
        }
    };
    other.visit(ConstructVisitor{this});
}

void AstExprData::_move_assign_value(AstExprData& other) noexcept {
    struct AssignVisitor {
        AstExprData* self;

        void visit_block(Block& block) { self->block_ = std::move(block); }

        void visit_unary(Unary& unary) { self->unary_ = std::move(unary); }

        void visit_binary(Binary& binary) { self->binary_ = std::move(binary); }

        void visit_var(Var& var) { self->var_ = std::move(var); }

        void visit_property_access(PropertyAccess& property_access) {
            self->property_access_ = std::move(property_access);
        }

        void visit_element_access(ElementAccess& element_access) {
            self->element_access_ = std::move(element_access);
        }

        void visit_call(Call& call) { self->call_ = std::move(call); }

        void visit_if(If& i) { self->if_ = std::move(i); }

        void visit_return(Return& ret) { self->return_ = std::move(ret); }

        void visit_break(Break& br) { self->break_ = std::move(br); }

        void visit_continue(Continue& cont) {
            self->continue_ = std::move(cont);
        }

        void visit_string_sequence(StringSequence& string_sequence) {
            self->string_sequence_ = std::move(string_sequence);
        }

        void
        visit_interpolated_string(InterpolatedString& interpolated_string) {
            self->interpolated_string_ = std::move(interpolated_string);
        }

        void visit_null(Null& null) { self->null_ = std::move(null); }

        void visit_boolean(Boolean& boolean) {
            self->boolean_ = std::move(boolean);
        }

        void visit_integer(Integer& integer) {
            self->integer_ = std::move(integer);
        }

        void visit_float(Float& f) { self->float_ = std::move(f); }

        void visit_string(String& string) { self->string_ = std::move(string); }

        void visit_symbol(Symbol& symbol) { self->symbol_ = std::move(symbol); }

        void visit_array(Array& array) { self->array_ = std::move(array); }

        void visit_tuple(Tuple& tuple) { self->tuple_ = std::move(tuple); }

        void visit_set(Set& set) { self->set_ = std::move(set); }

        void visit_map(Map& map) { self->map_ = std::move(map); }

        void visit_func(Func& func) { self->func_ = std::move(func); }
    };
    other.visit(AssignVisitor{this});
}

// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ast import StmtData
    implement(StmtData.tag, StmtData)
]]] */
std::string_view to_string(AstStmtType type) {
    switch (type) {
    case AstStmtType::Empty:
        return "Empty";
    case AstStmtType::Assert:
        return "Assert";
    case AstStmtType::Decl:
        return "Decl";
    case AstStmtType::While:
        return "While";
    case AstStmtType::For:
        return "For";
    case AstStmtType::Expr:
        return "Expr";
    }
    TIRO_UNREACHABLE("Invalid AstStmtType.");
}

AstStmtData AstStmtData::make_empty() {
    return {Empty{}};
}

AstStmtData
AstStmtData::make_assert(AstPtr<AstExpr> cond, AstPtr<AstExpr> message) {
    return {Assert{std::move(cond), std::move(message)}};
}

AstStmtData AstStmtData::make_decl(std::vector<AstPtr<AstDecl>> decls) {
    return {Decl{std::move(decls)}};
}

AstStmtData
AstStmtData::make_while(AstPtr<AstExpr> cond, AstPtr<AstExpr> body) {
    return {While{std::move(cond), std::move(body)}};
}

AstStmtData AstStmtData::make_for(AstPtr<AstStmt> decl, AstPtr<AstExpr> cond,
    AstPtr<AstExpr> step, AstPtr<AstExpr> body) {
    return {For{
        std::move(decl), std::move(cond), std::move(step), std::move(body)}};
}

AstStmtData AstStmtData::make_expr(AstPtr<AstExpr> expr) {
    return {Expr{std::move(expr)}};
}

AstStmtData::AstStmtData(Empty empty)
    : type_(AstStmtType::Empty)
    , empty_(std::move(empty)) {}

AstStmtData::AstStmtData(Assert assert)
    : type_(AstStmtType::Assert)
    , assert_(std::move(assert)) {}

AstStmtData::AstStmtData(Decl decl)
    : type_(AstStmtType::Decl)
    , decl_(std::move(decl)) {}

AstStmtData::AstStmtData(While w)
    : type_(AstStmtType::While)
    , while_(std::move(w)) {}

AstStmtData::AstStmtData(For f)
    : type_(AstStmtType::For)
    , for_(std::move(f)) {}

AstStmtData::AstStmtData(Expr expr)
    : type_(AstStmtType::Expr)
    , expr_(std::move(expr)) {}

AstStmtData::~AstStmtData() {
    _destroy_value();
}

static_assert(
    std::is_nothrow_move_constructible_v<AstStmtData::
            Empty> && std::is_nothrow_move_assignable_v<AstStmtData::Empty>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstStmtData::
            Assert> && std::is_nothrow_move_assignable_v<AstStmtData::Assert>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstStmtData::
            Decl> && std::is_nothrow_move_assignable_v<AstStmtData::Decl>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstStmtData::
            While> && std::is_nothrow_move_assignable_v<AstStmtData::While>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstStmtData::
            For> && std::is_nothrow_move_assignable_v<AstStmtData::For>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstStmtData::
            Expr> && std::is_nothrow_move_assignable_v<AstStmtData::Expr>,
    "Only nothrow movable types are supported in generated unions.");

AstStmtData::AstStmtData(AstStmtData&& other) noexcept
    : type_(other.type()) {
    _move_construct_value(other);
}

AstStmtData& AstStmtData::operator=(AstStmtData&& other) noexcept {
    TIRO_DEBUG_ASSERT(this != &other, "Self move assignement is invalid.");
    if (type() == other.type()) {
        _move_assign_value(other);
    } else {
        _destroy_value();
        _move_construct_value(other);
        type_ = other.type();
    }
    return *this;
}

const AstStmtData::Empty& AstStmtData::as_empty() const {
    TIRO_DEBUG_ASSERT(type_ == AstStmtType::Empty,
        "Bad member access on AstStmtData: not a Empty.");
    return empty_;
}

const AstStmtData::Assert& AstStmtData::as_assert() const {
    TIRO_DEBUG_ASSERT(type_ == AstStmtType::Assert,
        "Bad member access on AstStmtData: not a Assert.");
    return assert_;
}

const AstStmtData::Decl& AstStmtData::as_decl() const {
    TIRO_DEBUG_ASSERT(type_ == AstStmtType::Decl,
        "Bad member access on AstStmtData: not a Decl.");
    return decl_;
}

const AstStmtData::While& AstStmtData::as_while() const {
    TIRO_DEBUG_ASSERT(type_ == AstStmtType::While,
        "Bad member access on AstStmtData: not a While.");
    return while_;
}

const AstStmtData::For& AstStmtData::as_for() const {
    TIRO_DEBUG_ASSERT(type_ == AstStmtType::For,
        "Bad member access on AstStmtData: not a For.");
    return for_;
}

const AstStmtData::Expr& AstStmtData::as_expr() const {
    TIRO_DEBUG_ASSERT(type_ == AstStmtType::Expr,
        "Bad member access on AstStmtData: not a Expr.");
    return expr_;
}

void AstStmtData::_destroy_value() noexcept {
    struct DestroyVisitor {
        void visit_empty(Empty& empty) { empty.~Empty(); }

        void visit_assert(Assert& assert) { assert.~Assert(); }

        void visit_decl(Decl& decl) { decl.~Decl(); }

        void visit_while(While& w) { w.~While(); }

        void visit_for(For& f) { f.~For(); }

        void visit_expr(Expr& expr) { expr.~Expr(); }
    };
    visit(DestroyVisitor{});
}

void AstStmtData::_move_construct_value(AstStmtData& other) noexcept {
    struct ConstructVisitor {
        AstStmtData* self;

        void visit_empty(Empty& empty) {
            new (&self->empty_) Empty(std::move(empty));
        }

        void visit_assert(Assert& assert) {
            new (&self->assert_) Assert(std::move(assert));
        }

        void visit_decl(Decl& decl) {
            new (&self->decl_) Decl(std::move(decl));
        }

        void visit_while(While& w) { new (&self->while_) While(std::move(w)); }

        void visit_for(For& f) { new (&self->for_) For(std::move(f)); }

        void visit_expr(Expr& expr) {
            new (&self->expr_) Expr(std::move(expr));
        }
    };
    other.visit(ConstructVisitor{this});
}

void AstStmtData::_move_assign_value(AstStmtData& other) noexcept {
    struct AssignVisitor {
        AstStmtData* self;

        void visit_empty(Empty& empty) { self->empty_ = std::move(empty); }

        void visit_assert(Assert& assert) { self->assert_ = std::move(assert); }

        void visit_decl(Decl& decl) { self->decl_ = std::move(decl); }

        void visit_while(While& w) { self->while_ = std::move(w); }

        void visit_for(For& f) { self->for_ = std::move(f); }

        void visit_expr(Expr& expr) { self->expr_ = std::move(expr); }
    };
    other.visit(AssignVisitor{this});
}

// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ast import DeclData
    implement(DeclData.tag, DeclData)
]]] */
std::string_view to_string(AstDeclType type) {
    switch (type) {
    case AstDeclType::Func:
        return "Func";
    case AstDeclType::Var:
        return "Var";
    case AstDeclType::Tuple:
        return "Tuple";
    case AstDeclType::Import:
        return "Import";
    }
    TIRO_UNREACHABLE("Invalid AstDeclType.");
}

AstDeclData AstDeclData::make_func(const InternedString& name,
    std::vector<AstPtr<AstDecl>> params, AstPtr<AstExpr> body,
    const bool& body_is_value) {
    return {Func{name, std::move(params), std::move(body), body_is_value}};
}

AstDeclData AstDeclData::make_var(
    const InternedString& name, const bool& is_const, AstPtr<AstExpr> init) {
    return {Var{name, is_const, std::move(init)}};
}

AstDeclData AstDeclData::make_tuple(std::vector<InternedString> names,
    const bool& is_const, AstPtr<AstExpr> init) {
    return {Tuple{std::move(names), is_const, std::move(init)}};
}

AstDeclData AstDeclData::make_import(std::vector<InternedString> path) {
    return {Import{std::move(path)}};
}

AstDeclData::AstDeclData(Func func)
    : type_(AstDeclType::Func)
    , func_(std::move(func)) {}

AstDeclData::AstDeclData(Var var)
    : type_(AstDeclType::Var)
    , var_(std::move(var)) {}

AstDeclData::AstDeclData(Tuple tuple)
    : type_(AstDeclType::Tuple)
    , tuple_(std::move(tuple)) {}

AstDeclData::AstDeclData(Import import)
    : type_(AstDeclType::Import)
    , import_(std::move(import)) {}

AstDeclData::~AstDeclData() {
    _destroy_value();
}

static_assert(
    std::is_nothrow_move_constructible_v<AstDeclData::
            Func> && std::is_nothrow_move_assignable_v<AstDeclData::Func>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstDeclData::
            Var> && std::is_nothrow_move_assignable_v<AstDeclData::Var>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstDeclData::
            Tuple> && std::is_nothrow_move_assignable_v<AstDeclData::Tuple>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<AstDeclData::
            Import> && std::is_nothrow_move_assignable_v<AstDeclData::Import>,
    "Only nothrow movable types are supported in generated unions.");

AstDeclData::AstDeclData(AstDeclData&& other) noexcept
    : type_(other.type()) {
    _move_construct_value(other);
}

AstDeclData& AstDeclData::operator=(AstDeclData&& other) noexcept {
    TIRO_DEBUG_ASSERT(this != &other, "Self move assignement is invalid.");
    if (type() == other.type()) {
        _move_assign_value(other);
    } else {
        _destroy_value();
        _move_construct_value(other);
        type_ = other.type();
    }
    return *this;
}

const AstDeclData::Func& AstDeclData::as_func() const {
    TIRO_DEBUG_ASSERT(type_ == AstDeclType::Func,
        "Bad member access on AstDeclData: not a Func.");
    return func_;
}

const AstDeclData::Var& AstDeclData::as_var() const {
    TIRO_DEBUG_ASSERT(type_ == AstDeclType::Var,
        "Bad member access on AstDeclData: not a Var.");
    return var_;
}

const AstDeclData::Tuple& AstDeclData::as_tuple() const {
    TIRO_DEBUG_ASSERT(type_ == AstDeclType::Tuple,
        "Bad member access on AstDeclData: not a Tuple.");
    return tuple_;
}

const AstDeclData::Import& AstDeclData::as_import() const {
    TIRO_DEBUG_ASSERT(type_ == AstDeclType::Import,
        "Bad member access on AstDeclData: not a Import.");
    return import_;
}

void AstDeclData::_destroy_value() noexcept {
    struct DestroyVisitor {
        void visit_func(Func& func) { func.~Func(); }

        void visit_var(Var& var) { var.~Var(); }

        void visit_tuple(Tuple& tuple) { tuple.~Tuple(); }

        void visit_import(Import& import) { import.~Import(); }
    };
    visit(DestroyVisitor{});
}

void AstDeclData::_move_construct_value(AstDeclData& other) noexcept {
    struct ConstructVisitor {
        AstDeclData* self;

        void visit_func(Func& func) {
            new (&self->func_) Func(std::move(func));
        }

        void visit_var(Var& var) { new (&self->var_) Var(std::move(var)); }

        void visit_tuple(Tuple& tuple) {
            new (&self->tuple_) Tuple(std::move(tuple));
        }

        void visit_import(Import& import) {
            new (&self->import_) Import(std::move(import));
        }
    };
    other.visit(ConstructVisitor{this});
}

void AstDeclData::_move_assign_value(AstDeclData& other) noexcept {
    struct AssignVisitor {
        AstDeclData* self;

        void visit_func(Func& func) { self->func_ = std::move(func); }

        void visit_var(Var& var) { self->var_ = std::move(var); }

        void visit_tuple(Tuple& tuple) { self->tuple_ = std::move(tuple); }

        void visit_import(Import& import) { self->import_ = std::move(import); }
    };
    other.visit(AssignVisitor{this});
}

// [[[end]]]

} // namespace tiro

TIRO_IMPLEMENT_AST_DELETER(tiro::AstExpr)
TIRO_IMPLEMENT_AST_DELETER(tiro::AstStmt)
TIRO_IMPLEMENT_AST_DELETER(tiro::AstDecl)
