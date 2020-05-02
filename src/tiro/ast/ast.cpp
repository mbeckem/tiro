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
    from codegen.ast import PropertyType
    implement(PropertyType)
]]] */
std::string_view to_string(ASTPropertyType type) {
    switch (type) {
    case ASTPropertyType::Field:
        return "Field";
    case ASTPropertyType::TupleField:
        return "TupleField";
    }
    TIRO_UNREACHABLE("Invalid ASTPropertyType.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ast import Property
    implement(Property)
]]] */
ASTProperty ASTProperty::make_field(const InternedString& name) {
    return {Field{name}};
}

ASTProperty ASTProperty::make_tuple_field(const u32& index) {
    return {TupleField{index}};
}

ASTProperty::ASTProperty(Field field)
    : type_(ASTPropertyType::Field)
    , field_(std::move(field)) {}

ASTProperty::ASTProperty(TupleField tuple_field)
    : type_(ASTPropertyType::TupleField)
    , tuple_field_(std::move(tuple_field)) {}

const ASTProperty::Field& ASTProperty::as_field() const {
    TIRO_DEBUG_ASSERT(type_ == ASTPropertyType::Field,
        "Bad member access on ASTProperty: not a Field.");
    return field_;
}

const ASTProperty::TupleField& ASTProperty::as_tuple_field() const {
    TIRO_DEBUG_ASSERT(type_ == ASTPropertyType::TupleField,
        "Bad member access on ASTProperty: not a TupleField.");
    return tuple_field_;
}

void ASTProperty::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_field([[maybe_unused]] const Field& field) {
            stream.format("Field(name: {})", field.name);
        }

        void visit_tuple_field([[maybe_unused]] const TupleField& tuple_field) {
            stream.format("TupleField(index: {})", tuple_field.index);
        }
    };
    visit(FormatVisitor{stream});
}

// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ast import ExprType
    implement(ExprType)
]]] */
std::string_view to_string(ASTExprType type) {
    switch (type) {
    case ASTExprType::Block:
        return "Block";
    case ASTExprType::Unary:
        return "Unary";
    case ASTExprType::Binary:
        return "Binary";
    case ASTExprType::Var:
        return "Var";
    case ASTExprType::PropertyAccess:
        return "PropertyAccess";
    case ASTExprType::ElementAccess:
        return "ElementAccess";
    case ASTExprType::Call:
        return "Call";
    case ASTExprType::If:
        return "If";
    case ASTExprType::Return:
        return "Return";
    case ASTExprType::Break:
        return "Break";
    case ASTExprType::Continue:
        return "Continue";
    case ASTExprType::StringSequence:
        return "StringSequence";
    case ASTExprType::InterpolatedString:
        return "InterpolatedString";
    case ASTExprType::Null:
        return "Null";
    case ASTExprType::Boolean:
        return "Boolean";
    case ASTExprType::Integer:
        return "Integer";
    case ASTExprType::Float:
        return "Float";
    case ASTExprType::String:
        return "String";
    case ASTExprType::Symbol:
        return "Symbol";
    case ASTExprType::Array:
        return "Array";
    case ASTExprType::Tuple:
        return "Tuple";
    case ASTExprType::Set:
        return "Set";
    case ASTExprType::Map:
        return "Map";
    case ASTExprType::Func:
        return "Func";
    }
    TIRO_UNREACHABLE("Invalid ASTExprType.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ast import ExprData
    implement(ExprData)
]]] */
ASTExprData ASTExprData::make_block(std::vector<ASTPtr<ASTStmt>> stmts) {
    return {Block{std::move(stmts)}};
}

ASTExprData
ASTExprData::make_unary(const UnaryOperator& operation, ASTPtr<ASTExpr> inner) {
    return {Unary{operation, std::move(inner)}};
}

ASTExprData ASTExprData::make_binary(const BinaryOperator& operation,
    ASTPtr<ASTExpr> left, ASTPtr<ASTExpr> right) {
    return {Binary{operation, std::move(left), std::move(right)}};
}

ASTExprData ASTExprData::make_var(const InternedString& name) {
    return {Var{name}};
}

ASTExprData ASTExprData::make_property_access(const AccessType& access_type,
    ASTPtr<ASTExpr> instance, const ASTProperty& property) {
    return {PropertyAccess{access_type, std::move(instance), property}};
}

ASTExprData ASTExprData::make_element_access(const AccessType& access_type,
    ASTPtr<ASTExpr> instance, const u32& element) {
    return {ElementAccess{access_type, std::move(instance), element}};
}

ASTExprData ASTExprData::make_call(const AccessType& access_type,
    ASTPtr<ASTExpr> func, std::vector<ASTPtr<ASTExpr>> args) {
    return {Call{access_type, std::move(func), std::move(args)}};
}

ASTExprData ASTExprData::make_if(ASTPtr<ASTExpr> cond,
    ASTPtr<ASTExpr> then_branch, ASTPtr<ASTExpr> else_branch) {
    return {
        If{std::move(cond), std::move(then_branch), std::move(else_branch)}};
}

ASTExprData ASTExprData::make_return(ASTPtr<ASTExpr> value) {
    return {Return{std::move(value)}};
}

ASTExprData ASTExprData::make_break() {
    return {Break{}};
}

ASTExprData ASTExprData::make_continue() {
    return {Continue{}};
}

ASTExprData
ASTExprData::make_string_sequence(std::vector<ASTPtr<ASTExpr>> strings) {
    return {StringSequence{std::move(strings)}};
}

ASTExprData
ASTExprData::make_interpolated_string(std::vector<ASTPtr<ASTExpr>> strings) {
    return {InterpolatedString{std::move(strings)}};
}

ASTExprData ASTExprData::make_null() {
    return {Null{}};
}

ASTExprData ASTExprData::make_boolean(const bool& value) {
    return {Boolean{value}};
}

ASTExprData ASTExprData::make_integer(const i64& value) {
    return {Integer{value}};
}

ASTExprData ASTExprData::make_float(const f64& value) {
    return {Float{value}};
}

ASTExprData ASTExprData::make_string(const InternedString& value) {
    return {String{value}};
}

ASTExprData ASTExprData::make_symbol(const InternedString& value) {
    return {Symbol{value}};
}

ASTExprData ASTExprData::make_array(std::vector<ASTPtr<ASTExpr>> items) {
    return {Array{std::move(items)}};
}

ASTExprData ASTExprData::make_tuple(std::vector<ASTPtr<ASTExpr>> items) {
    return {Tuple{std::move(items)}};
}

ASTExprData ASTExprData::make_set(std::vector<ASTPtr<ASTExpr>> items) {
    return {Set{std::move(items)}};
}

ASTExprData ASTExprData::make_map(
    std::vector<ASTPtr<ASTExpr>> keys, std::vector<ASTPtr<ASTExpr>> values) {
    return {Map{std::move(keys), std::move(values)}};
}

ASTExprData ASTExprData::make_func(ASTPtr<ASTDecl> decl) {
    return {Func{std::move(decl)}};
}

ASTExprData::ASTExprData(Block block)
    : type_(ASTExprType::Block)
    , block_(std::move(block)) {}

ASTExprData::ASTExprData(Unary unary)
    : type_(ASTExprType::Unary)
    , unary_(std::move(unary)) {}

ASTExprData::ASTExprData(Binary binary)
    : type_(ASTExprType::Binary)
    , binary_(std::move(binary)) {}

ASTExprData::ASTExprData(Var var)
    : type_(ASTExprType::Var)
    , var_(std::move(var)) {}

ASTExprData::ASTExprData(PropertyAccess property_access)
    : type_(ASTExprType::PropertyAccess)
    , property_access_(std::move(property_access)) {}

ASTExprData::ASTExprData(ElementAccess element_access)
    : type_(ASTExprType::ElementAccess)
    , element_access_(std::move(element_access)) {}

ASTExprData::ASTExprData(Call call)
    : type_(ASTExprType::Call)
    , call_(std::move(call)) {}

ASTExprData::ASTExprData(If i)
    : type_(ASTExprType::If)
    , if_(std::move(i)) {}

ASTExprData::ASTExprData(Return ret)
    : type_(ASTExprType::Return)
    , return_(std::move(ret)) {}

ASTExprData::ASTExprData(Break br)
    : type_(ASTExprType::Break)
    , break_(std::move(br)) {}

ASTExprData::ASTExprData(Continue cont)
    : type_(ASTExprType::Continue)
    , continue_(std::move(cont)) {}

ASTExprData::ASTExprData(StringSequence string_sequence)
    : type_(ASTExprType::StringSequence)
    , string_sequence_(std::move(string_sequence)) {}

ASTExprData::ASTExprData(InterpolatedString interpolated_string)
    : type_(ASTExprType::InterpolatedString)
    , interpolated_string_(std::move(interpolated_string)) {}

ASTExprData::ASTExprData(Null null)
    : type_(ASTExprType::Null)
    , null_(std::move(null)) {}

ASTExprData::ASTExprData(Boolean boolean)
    : type_(ASTExprType::Boolean)
    , boolean_(std::move(boolean)) {}

ASTExprData::ASTExprData(Integer integer)
    : type_(ASTExprType::Integer)
    , integer_(std::move(integer)) {}

ASTExprData::ASTExprData(Float f)
    : type_(ASTExprType::Float)
    , float_(std::move(f)) {}

ASTExprData::ASTExprData(String string)
    : type_(ASTExprType::String)
    , string_(std::move(string)) {}

ASTExprData::ASTExprData(Symbol symbol)
    : type_(ASTExprType::Symbol)
    , symbol_(std::move(symbol)) {}

ASTExprData::ASTExprData(Array array)
    : type_(ASTExprType::Array)
    , array_(std::move(array)) {}

ASTExprData::ASTExprData(Tuple tuple)
    : type_(ASTExprType::Tuple)
    , tuple_(std::move(tuple)) {}

ASTExprData::ASTExprData(Set set)
    : type_(ASTExprType::Set)
    , set_(std::move(set)) {}

ASTExprData::ASTExprData(Map map)
    : type_(ASTExprType::Map)
    , map_(std::move(map)) {}

ASTExprData::ASTExprData(Func func)
    : type_(ASTExprType::Func)
    , func_(std::move(func)) {}

ASTExprData::~ASTExprData() {
    _destroy_value();
}

static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Block> && std::is_nothrow_move_assignable_v<ASTExprData::Block>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Unary> && std::is_nothrow_move_assignable_v<ASTExprData::Unary>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Binary> && std::is_nothrow_move_assignable_v<ASTExprData::Binary>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Var> && std::is_nothrow_move_assignable_v<ASTExprData::Var>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            PropertyAccess> && std::is_nothrow_move_assignable_v<ASTExprData::PropertyAccess>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            ElementAccess> && std::is_nothrow_move_assignable_v<ASTExprData::ElementAccess>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Call> && std::is_nothrow_move_assignable_v<ASTExprData::Call>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<
        ASTExprData::If> && std::is_nothrow_move_assignable_v<ASTExprData::If>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Return> && std::is_nothrow_move_assignable_v<ASTExprData::Return>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Break> && std::is_nothrow_move_assignable_v<ASTExprData::Break>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Continue> && std::is_nothrow_move_assignable_v<ASTExprData::Continue>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            StringSequence> && std::is_nothrow_move_assignable_v<ASTExprData::StringSequence>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            InterpolatedString> && std::is_nothrow_move_assignable_v<ASTExprData::InterpolatedString>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Null> && std::is_nothrow_move_assignable_v<ASTExprData::Null>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Boolean> && std::is_nothrow_move_assignable_v<ASTExprData::Boolean>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Integer> && std::is_nothrow_move_assignable_v<ASTExprData::Integer>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Float> && std::is_nothrow_move_assignable_v<ASTExprData::Float>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            String> && std::is_nothrow_move_assignable_v<ASTExprData::String>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Symbol> && std::is_nothrow_move_assignable_v<ASTExprData::Symbol>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Array> && std::is_nothrow_move_assignable_v<ASTExprData::Array>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Tuple> && std::is_nothrow_move_assignable_v<ASTExprData::Tuple>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Set> && std::is_nothrow_move_assignable_v<ASTExprData::Set>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Map> && std::is_nothrow_move_assignable_v<ASTExprData::Map>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Func> && std::is_nothrow_move_assignable_v<ASTExprData::Func>,
    "Only nothrow movable types are supported in generated unions.");

ASTExprData::ASTExprData(ASTExprData&& other) noexcept
    : type_(other.type()) {
    _move_construct_value(other);
}

ASTExprData& ASTExprData::operator=(ASTExprData&& other) noexcept {
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

const ASTExprData::Block& ASTExprData::as_block() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Block,
        "Bad member access on ASTExprData: not a Block.");
    return block_;
}

const ASTExprData::Unary& ASTExprData::as_unary() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Unary,
        "Bad member access on ASTExprData: not a Unary.");
    return unary_;
}

const ASTExprData::Binary& ASTExprData::as_binary() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Binary,
        "Bad member access on ASTExprData: not a Binary.");
    return binary_;
}

const ASTExprData::Var& ASTExprData::as_var() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Var,
        "Bad member access on ASTExprData: not a Var.");
    return var_;
}

const ASTExprData::PropertyAccess& ASTExprData::as_property_access() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::PropertyAccess,
        "Bad member access on ASTExprData: not a PropertyAccess.");
    return property_access_;
}

const ASTExprData::ElementAccess& ASTExprData::as_element_access() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::ElementAccess,
        "Bad member access on ASTExprData: not a ElementAccess.");
    return element_access_;
}

const ASTExprData::Call& ASTExprData::as_call() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Call,
        "Bad member access on ASTExprData: not a Call.");
    return call_;
}

const ASTExprData::If& ASTExprData::as_if() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::If,
        "Bad member access on ASTExprData: not a If.");
    return if_;
}

const ASTExprData::Return& ASTExprData::as_return() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Return,
        "Bad member access on ASTExprData: not a Return.");
    return return_;
}

const ASTExprData::Break& ASTExprData::as_break() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Break,
        "Bad member access on ASTExprData: not a Break.");
    return break_;
}

const ASTExprData::Continue& ASTExprData::as_continue() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Continue,
        "Bad member access on ASTExprData: not a Continue.");
    return continue_;
}

const ASTExprData::StringSequence& ASTExprData::as_string_sequence() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::StringSequence,
        "Bad member access on ASTExprData: not a StringSequence.");
    return string_sequence_;
}

const ASTExprData::InterpolatedString&
ASTExprData::as_interpolated_string() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::InterpolatedString,
        "Bad member access on ASTExprData: not a InterpolatedString.");
    return interpolated_string_;
}

const ASTExprData::Null& ASTExprData::as_null() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Null,
        "Bad member access on ASTExprData: not a Null.");
    return null_;
}

const ASTExprData::Boolean& ASTExprData::as_boolean() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Boolean,
        "Bad member access on ASTExprData: not a Boolean.");
    return boolean_;
}

const ASTExprData::Integer& ASTExprData::as_integer() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Integer,
        "Bad member access on ASTExprData: not a Integer.");
    return integer_;
}

const ASTExprData::Float& ASTExprData::as_float() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Float,
        "Bad member access on ASTExprData: not a Float.");
    return float_;
}

const ASTExprData::String& ASTExprData::as_string() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::String,
        "Bad member access on ASTExprData: not a String.");
    return string_;
}

const ASTExprData::Symbol& ASTExprData::as_symbol() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Symbol,
        "Bad member access on ASTExprData: not a Symbol.");
    return symbol_;
}

const ASTExprData::Array& ASTExprData::as_array() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Array,
        "Bad member access on ASTExprData: not a Array.");
    return array_;
}

const ASTExprData::Tuple& ASTExprData::as_tuple() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Tuple,
        "Bad member access on ASTExprData: not a Tuple.");
    return tuple_;
}

const ASTExprData::Set& ASTExprData::as_set() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Set,
        "Bad member access on ASTExprData: not a Set.");
    return set_;
}

const ASTExprData::Map& ASTExprData::as_map() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Map,
        "Bad member access on ASTExprData: not a Map.");
    return map_;
}

const ASTExprData::Func& ASTExprData::as_func() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Func,
        "Bad member access on ASTExprData: not a Func.");
    return func_;
}

void ASTExprData::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_block([[maybe_unused]] const Block& block) {
            stream.format("Block(stmts: {})", block.stmts);
        }

        void visit_unary([[maybe_unused]] const Unary& unary) {
            stream.format("Unary(operation: {}, inner: {})", unary.operation,
                unary.inner);
        }

        void visit_binary([[maybe_unused]] const Binary& binary) {
            stream.format("Binary(operation: {}, left: {}, right: {})",
                binary.operation, binary.left, binary.right);
        }

        void visit_var([[maybe_unused]] const Var& var) {
            stream.format("Var(name: {})", var.name);
        }

        void visit_property_access(
            [[maybe_unused]] const PropertyAccess& property_access) {
            stream.format(
                "PropertyAccess(access_type: {}, instance: {}, property: {})",
                property_access.access_type, property_access.instance,
                property_access.property);
        }

        void visit_element_access(
            [[maybe_unused]] const ElementAccess& element_access) {
            stream.format(
                "ElementAccess(access_type: {}, instance: {}, element: {})",
                element_access.access_type, element_access.instance,
                element_access.element);
        }

        void visit_call([[maybe_unused]] const Call& call) {
            stream.format("Call(access_type: {}, func: {}, args: {})",
                call.access_type, call.func, call.args);
        }

        void visit_if([[maybe_unused]] const If& i) {
            stream.format("If(cond: {}, then_branch: {}, else_branch: {})",
                i.cond, i.then_branch, i.else_branch);
        }

        void visit_return([[maybe_unused]] const Return& ret) {
            stream.format("Return(value: {})", ret.value);
        }

        void visit_break([[maybe_unused]] const Break& br) {
            stream.format("Break");
        }

        void visit_continue([[maybe_unused]] const Continue& cont) {
            stream.format("Continue");
        }

        void visit_string_sequence(
            [[maybe_unused]] const StringSequence& string_sequence) {
            stream.format(
                "StringSequence(strings: {})", string_sequence.strings);
        }

        void visit_interpolated_string(
            [[maybe_unused]] const InterpolatedString& interpolated_string) {
            stream.format(
                "InterpolatedString(strings: {})", interpolated_string.strings);
        }

        void visit_null([[maybe_unused]] const Null& null) {
            stream.format("Null");
        }

        void visit_boolean([[maybe_unused]] const Boolean& boolean) {
            stream.format("Boolean(value: {})", boolean.value);
        }

        void visit_integer([[maybe_unused]] const Integer& integer) {
            stream.format("Integer(value: {})", integer.value);
        }

        void visit_float([[maybe_unused]] const Float& f) {
            stream.format("Float(value: {})", f.value);
        }

        void visit_string([[maybe_unused]] const String& string) {
            stream.format("String(value: {})", string.value);
        }

        void visit_symbol([[maybe_unused]] const Symbol& symbol) {
            stream.format("Symbol(value: {})", symbol.value);
        }

        void visit_array([[maybe_unused]] const Array& array) {
            stream.format("Array(items: {})", array.items);
        }

        void visit_tuple([[maybe_unused]] const Tuple& tuple) {
            stream.format("Tuple(items: {})", tuple.items);
        }

        void visit_set([[maybe_unused]] const Set& set) {
            stream.format("Set(items: {})", set.items);
        }

        void visit_map([[maybe_unused]] const Map& map) {
            stream.format("Map(keys: {}, values: {})", map.keys, map.values);
        }

        void visit_func([[maybe_unused]] const Func& func) {
            stream.format("Func(decl: {})", func.decl);
        }
    };
    visit(FormatVisitor{stream});
}

void ASTExprData::_destroy_value() noexcept {
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

void ASTExprData::_move_construct_value(ASTExprData& other) noexcept {
    struct ConstructVisitor {
        ASTExprData* self;

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

void ASTExprData::_move_assign_value(ASTExprData& other) noexcept {
    struct AssignVisitor {
        ASTExprData* self;

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

} // namespace tiro
