#include "compiler/ir/value.hpp"

#include "compiler/ir/function.hpp"

namespace tiro::ir {

std::string_view to_string(BinaryOpType type) {
    switch (type) {
#define TIRO_CASE(X, Y)   \
    case BinaryOpType::X: \
        return Y;

        TIRO_CASE(Plus, "+")
        TIRO_CASE(Minus, "-")
        TIRO_CASE(Multiply, "*")
        TIRO_CASE(Divide, "/")
        TIRO_CASE(Modulus, "mod")
        TIRO_CASE(Power, "pow")
        TIRO_CASE(LeftShift, "lsh")
        TIRO_CASE(RightShift, "rsh")
        TIRO_CASE(BitwiseAnd, "band")
        TIRO_CASE(BitwiseOr, "bor")
        TIRO_CASE(BitwiseXor, "bxor")
        TIRO_CASE(Less, "lt")
        TIRO_CASE(LessEquals, "lte")
        TIRO_CASE(Greater, "gt")
        TIRO_CASE(GreaterEquals, "gte")
        TIRO_CASE(Equals, "eq")
        TIRO_CASE(NotEquals, "neq")

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid binary operation type.");
}

std::string_view to_string(UnaryOpType type) {
    switch (type) {
#define TIRO_CASE(X, Y)  \
    case UnaryOpType::X: \
        return Y;

        TIRO_CASE(Plus, "+")
        TIRO_CASE(Minus, "-")
        TIRO_CASE(BitwiseNot, "bnot")
        TIRO_CASE(LogicalNot, "lnot")

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid unary operation type.");
}

std::string_view to_string(ContainerType type) {
    switch (type) {
#define TIRO_CASE(X)       \
    case ContainerType::X: \
        return #X;

        TIRO_CASE(Array)
        TIRO_CASE(Tuple)
        TIRO_CASE(Set)
        TIRO_CASE(Map)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid container type.");
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import LValue
    implement(LValue.tag)
]]] */
std::string_view to_string(LValueType type) {
    switch (type) {
    case LValueType::Param:
        return "Param";
    case LValueType::Closure:
        return "Closure";
    case LValueType::Module:
        return "Module";
    case LValueType::Field:
        return "Field";
    case LValueType::TupleField:
        return "TupleField";
    case LValueType::Index:
        return "Index";
    }
    TIRO_UNREACHABLE("Invalid LValueType.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import LValue
    implement(LValue)
]]] */
LValue LValue::make_param(const ParamId& target) {
    return {Param{target}};
}

LValue LValue::make_closure(const InstId& env, const u32& levels, const u32& index) {
    return {Closure{env, levels, index}};
}

LValue LValue::make_module(const ModuleMemberId& member) {
    return {Module{member}};
}

LValue LValue::make_field(const InstId& object, const InternedString& name) {
    return {Field{object, name}};
}

LValue LValue::make_tuple_field(const InstId& object, const u32& index) {
    return {TupleField{object, index}};
}

LValue LValue::make_index(const InstId& object, const InstId& index) {
    return {Index{object, index}};
}

LValue::LValue(Param param)
    : type_(LValueType::Param)
    , param_(std::move(param)) {}

LValue::LValue(Closure closure)
    : type_(LValueType::Closure)
    , closure_(std::move(closure)) {}

LValue::LValue(Module module)
    : type_(LValueType::Module)
    , module_(std::move(module)) {}

LValue::LValue(Field field)
    : type_(LValueType::Field)
    , field_(std::move(field)) {}

LValue::LValue(TupleField tuple_field)
    : type_(LValueType::TupleField)
    , tuple_field_(std::move(tuple_field)) {}

LValue::LValue(Index index)
    : type_(LValueType::Index)
    , index_(std::move(index)) {}

const LValue::Param& LValue::as_param() const {
    TIRO_DEBUG_ASSERT(type_ == LValueType::Param, "Bad member access on LValue: not a Param.");
    return param_;
}

const LValue::Closure& LValue::as_closure() const {
    TIRO_DEBUG_ASSERT(type_ == LValueType::Closure, "Bad member access on LValue: not a Closure.");
    return closure_;
}

const LValue::Module& LValue::as_module() const {
    TIRO_DEBUG_ASSERT(type_ == LValueType::Module, "Bad member access on LValue: not a Module.");
    return module_;
}

const LValue::Field& LValue::as_field() const {
    TIRO_DEBUG_ASSERT(type_ == LValueType::Field, "Bad member access on LValue: not a Field.");
    return field_;
}

const LValue::TupleField& LValue::as_tuple_field() const {
    TIRO_DEBUG_ASSERT(
        type_ == LValueType::TupleField, "Bad member access on LValue: not a TupleField.");
    return tuple_field_;
}

const LValue::Index& LValue::as_index() const {
    TIRO_DEBUG_ASSERT(type_ == LValueType::Index, "Bad member access on LValue: not a Index.");
    return index_;
}

void LValue::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_param([[maybe_unused]] const Param& param) {
            stream.format("Param(target: {})", param.target);
        }

        void visit_closure([[maybe_unused]] const Closure& closure) {
            stream.format("Closure(env: {}, levels: {}, index: {})", closure.env, closure.levels,
                closure.index);
        }

        void visit_module([[maybe_unused]] const Module& module) {
            stream.format("Module(member: {})", module.member);
        }

        void visit_field([[maybe_unused]] const Field& field) {
            stream.format("Field(object: {}, name: {})", field.object, field.name);
        }

        void visit_tuple_field([[maybe_unused]] const TupleField& tuple_field) {
            stream.format(
                "TupleField(object: {}, index: {})", tuple_field.object, tuple_field.index);
        }

        void visit_index([[maybe_unused]] const Index& index) {
            stream.format("Index(object: {}, index: {})", index.object, index.index);
        }
    };
    visit(FormatVisitor{stream});
}

// [[[end]]]

Phi::Phi() {}

Phi::Phi(LocalListId operands)
    : operands_(operands) {}

Phi::Phi(Function& func, std::initializer_list<InstId> locals)
    : operands_(func.make(LocalList(locals))) {}

void Phi::append_operand(Function& func, InstId operand) {
    if (!operands_) {
        operands_ = func.make(LocalList{operand});
        return;
    }

    auto& list = func[operands_];
    list.append(operand);
}

InstId Phi::operand(const Function& func, size_t index) const {
    TIRO_DEBUG_ASSERT(operands_, "Phi has no operands.");
    auto& list = func[operands_];
    return list.get(index);
}

void Phi::operand(Function& func, size_t index, InstId local) {
    TIRO_DEBUG_ASSERT(operands_, "Phi has no operands.");
    auto& list = func[operands_];
    return list.set(index, local);
}

size_t Phi::operand_count(const Function& func) const {
    TIRO_DEBUG_ASSERT(operands_, "Phi has no operands.");
    return func[operands_].size();
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Constant
    implement(Constant.tag)
]]] */
std::string_view to_string(ConstantType type) {
    switch (type) {
    case ConstantType::Integer:
        return "Integer";
    case ConstantType::Float:
        return "Float";
    case ConstantType::String:
        return "String";
    case ConstantType::Symbol:
        return "Symbol";
    case ConstantType::Null:
        return "Null";
    case ConstantType::True:
        return "True";
    case ConstantType::False:
        return "False";
    }
    TIRO_UNREACHABLE("Invalid ConstantType.");
}
// [[[end]]]

void FloatConstant::format(FormatStream& stream) const {
    stream.format("Float({})", value);
}

void FloatConstant::hash(Hasher& h) const {
    if (std::isnan(value)) {
        h.append(6.5670192717080285e+99); // Some random value
    } else {
        h.append(value);
    }
}

bool operator==(const FloatConstant& lhs, const FloatConstant& rhs) {
    if (std::isnan(lhs.value) && std::isnan(rhs.value))
        return true;
    return lhs.value == rhs.value;
}

bool operator!=(const FloatConstant& lhs, const FloatConstant& rhs) {
    return !(lhs == rhs);
}

bool operator<(const FloatConstant& lhs, const FloatConstant& rhs) {
    return lhs.value < rhs.value;
}

bool operator>(const FloatConstant& lhs, const FloatConstant& rhs) {
    return rhs < lhs;
}

bool operator<=(const FloatConstant& lhs, const FloatConstant& rhs) {
    return lhs < rhs || lhs == rhs;
}

bool operator>=(const FloatConstant& lhs, const FloatConstant& rhs) {
    return rhs <= lhs;
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Constant
    implement(Constant)
]]] */
Constant Constant::make_integer(const i64& value) {
    return {Integer{value}};
}

Constant Constant::make_float(const Float& f) {
    return f;
}

Constant Constant::make_string(const InternedString& value) {
    return {String{value}};
}

Constant Constant::make_symbol(const InternedString& value) {
    return {Symbol{value}};
}

Constant Constant::make_null() {
    return {Null{}};
}

Constant Constant::make_true() {
    return {True{}};
}

Constant Constant::make_false() {
    return {False{}};
}

Constant::Constant(Integer integer)
    : type_(ConstantType::Integer)
    , integer_(std::move(integer)) {}

Constant::Constant(Float f)
    : type_(ConstantType::Float)
    , float_(std::move(f)) {}

Constant::Constant(String string)
    : type_(ConstantType::String)
    , string_(std::move(string)) {}

Constant::Constant(Symbol symbol)
    : type_(ConstantType::Symbol)
    , symbol_(std::move(symbol)) {}

Constant::Constant(Null null)
    : type_(ConstantType::Null)
    , null_(std::move(null)) {}

Constant::Constant(True t)
    : type_(ConstantType::True)
    , true_(std::move(t)) {}

Constant::Constant(False f)
    : type_(ConstantType::False)
    , false_(std::move(f)) {}

const Constant::Integer& Constant::as_integer() const {
    TIRO_DEBUG_ASSERT(
        type_ == ConstantType::Integer, "Bad member access on Constant: not a Integer.");
    return integer_;
}

const Constant::Float& Constant::as_float() const {
    TIRO_DEBUG_ASSERT(type_ == ConstantType::Float, "Bad member access on Constant: not a Float.");
    return float_;
}

const Constant::String& Constant::as_string() const {
    TIRO_DEBUG_ASSERT(
        type_ == ConstantType::String, "Bad member access on Constant: not a String.");
    return string_;
}

const Constant::Symbol& Constant::as_symbol() const {
    TIRO_DEBUG_ASSERT(
        type_ == ConstantType::Symbol, "Bad member access on Constant: not a Symbol.");
    return symbol_;
}

const Constant::Null& Constant::as_null() const {
    TIRO_DEBUG_ASSERT(type_ == ConstantType::Null, "Bad member access on Constant: not a Null.");
    return null_;
}

const Constant::True& Constant::as_true() const {
    TIRO_DEBUG_ASSERT(type_ == ConstantType::True, "Bad member access on Constant: not a True.");
    return true_;
}

const Constant::False& Constant::as_false() const {
    TIRO_DEBUG_ASSERT(type_ == ConstantType::False, "Bad member access on Constant: not a False.");
    return false_;
}

void Constant::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_integer([[maybe_unused]] const Integer& integer) {
            stream.format("Integer(value: {})", integer.value);
        }

        void visit_float([[maybe_unused]] const Float& f) { stream.format("{}", f); }

        void visit_string([[maybe_unused]] const String& string) {
            stream.format("String(value: {})", string.value);
        }

        void visit_symbol([[maybe_unused]] const Symbol& symbol) {
            stream.format("Symbol(value: {})", symbol.value);
        }

        void visit_null([[maybe_unused]] const Null& null) { stream.format("Null"); }

        void visit_true([[maybe_unused]] const True& t) { stream.format("True"); }

        void visit_false([[maybe_unused]] const False& f) { stream.format("False"); }
    };
    visit(FormatVisitor{stream});
}

void Constant::hash(Hasher& h) const {
    h.append(type());

    struct HashVisitor {
        Hasher& h;

        void visit_integer([[maybe_unused]] const Integer& integer) { h.append(integer.value); }

        void visit_float([[maybe_unused]] const Float& f) { h.append(f); }

        void visit_string([[maybe_unused]] const String& string) { h.append(string.value); }

        void visit_symbol([[maybe_unused]] const Symbol& symbol) { h.append(symbol.value); }

        void visit_null([[maybe_unused]] const Null& null) { return; }

        void visit_true([[maybe_unused]] const True& t) { return; }

        void visit_false([[maybe_unused]] const False& f) { return; }
    };
    return visit(HashVisitor{h});
}

bool operator==(const Constant& lhs, const Constant& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const Constant& rhs;

        bool visit_integer([[maybe_unused]] const Constant::Integer& integer) {
            [[maybe_unused]] const auto& other = rhs.as_integer();
            return integer.value == other.value;
        }

        bool visit_float([[maybe_unused]] const Constant::Float& f) {
            [[maybe_unused]] const auto& other = rhs.as_float();
            return f == other;
        }

        bool visit_string([[maybe_unused]] const Constant::String& string) {
            [[maybe_unused]] const auto& other = rhs.as_string();
            return string.value == other.value;
        }

        bool visit_symbol([[maybe_unused]] const Constant::Symbol& symbol) {
            [[maybe_unused]] const auto& other = rhs.as_symbol();
            return symbol.value == other.value;
        }

        bool visit_null([[maybe_unused]] const Constant::Null& null) {
            [[maybe_unused]] const auto& other = rhs.as_null();
            return true;
        }

        bool visit_true([[maybe_unused]] const Constant::True& t) {
            [[maybe_unused]] const auto& other = rhs.as_true();
            return true;
        }

        bool visit_false([[maybe_unused]] const Constant::False& f) {
            [[maybe_unused]] const auto& other = rhs.as_false();
            return true;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const Constant& lhs, const Constant& rhs) {
    return !(lhs == rhs);
}
// [[[end]]]

bool is_same(const Constant& lhs, const Constant& rhs) {
    if (lhs.type() == ConstantType::Float && rhs.type() == ConstantType::Float) {

        if (std::isnan(lhs.as_float().value) && std::isnan(lhs.as_float().value))
            return true;
    }

    return lhs == rhs;
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Aggregate
    implement(Aggregate.tag)
]]] */
std::string_view to_string(AggregateType type) {
    switch (type) {
    case AggregateType::Method:
        return "Method";
    case AggregateType::IteratorNext:
        return "IteratorNext";
    }
    TIRO_UNREACHABLE("Invalid AggregateType.");
}
/// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Aggregate
    implement(Aggregate)
]]] */
Aggregate Aggregate::make_method(const InstId& instance, const InternedString& function) {
    return {Method{instance, function}};
}

Aggregate Aggregate::make_iterator_next(const InstId& iterator) {
    return {IteratorNext{iterator}};
}

Aggregate::Aggregate(Method method)
    : type_(AggregateType::Method)
    , method_(std::move(method)) {}

Aggregate::Aggregate(IteratorNext iterator_next)
    : type_(AggregateType::IteratorNext)
    , iterator_next_(std::move(iterator_next)) {}

const Aggregate::Method& Aggregate::as_method() const {
    TIRO_DEBUG_ASSERT(
        type_ == AggregateType::Method, "Bad member access on Aggregate: not a Method.");
    return method_;
}

const Aggregate::IteratorNext& Aggregate::as_iterator_next() const {
    TIRO_DEBUG_ASSERT(type_ == AggregateType::IteratorNext,
        "Bad member access on Aggregate: not a IteratorNext.");
    return iterator_next_;
}

void Aggregate::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_method([[maybe_unused]] const Method& method) {
            stream.format("Method(instance: {}, function: {})", method.instance, method.function);
        }

        void visit_iterator_next([[maybe_unused]] const IteratorNext& iterator_next) {
            stream.format("IteratorNext(iterator: {})", iterator_next.iterator);
        }
    };
    visit(FormatVisitor{stream});
}

void Aggregate::hash(Hasher& h) const {
    h.append(type());

    struct HashVisitor {
        Hasher& h;

        void visit_method([[maybe_unused]] const Method& method) {
            h.append(method.instance).append(method.function);
        }

        void visit_iterator_next([[maybe_unused]] const IteratorNext& iterator_next) {
            h.append(iterator_next.iterator);
        }
    };
    return visit(HashVisitor{h});
}

bool operator==(const Aggregate& lhs, const Aggregate& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const Aggregate& rhs;

        bool visit_method([[maybe_unused]] const Aggregate::Method& method) {
            [[maybe_unused]] const auto& other = rhs.as_method();
            return method.instance == other.instance && method.function == other.function;
        }

        bool visit_iterator_next([[maybe_unused]] const Aggregate::IteratorNext& iterator_next) {
            [[maybe_unused]] const auto& other = rhs.as_iterator_next();
            return iterator_next.iterator == other.iterator;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const Aggregate& lhs, const Aggregate& rhs) {
    return !(lhs == rhs);
}
/// [[[end]]]

AggregateType aggregate_type(AggregateMember member) {
    switch (member) {
    case AggregateMember::MethodInstance:
    case AggregateMember::MethodFunction:
        return AggregateType::Method;
    case AggregateMember::IteratorNextValid:
    case AggregateMember::IteratorNextValue:
        return AggregateType::IteratorNext;
    }

    TIRO_UNREACHABLE("Invalid aggregate member.");
}

std::string_view to_string(AggregateMember member) {
    switch (member) {
#define TIRO_CASE(T)         \
    case AggregateMember::T: \
        return #T;

        TIRO_CASE(MethodInstance)
        TIRO_CASE(MethodFunction)
        TIRO_CASE(IteratorNextValid)
        TIRO_CASE(IteratorNextValue)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid aggregate member.");
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Value
    implement(Value.tag)
]]] */
std::string_view to_string(ValueType type) {
    switch (type) {
    case ValueType::Read:
        return "Read";
    case ValueType::Write:
        return "Write";
    case ValueType::Alias:
        return "Alias";
    case ValueType::Phi:
        return "Phi";
    case ValueType::ObserveAssign:
        return "ObserveAssign";
    case ValueType::PublishAssign:
        return "PublishAssign";
    case ValueType::Constant:
        return "Constant";
    case ValueType::OuterEnvironment:
        return "OuterEnvironment";
    case ValueType::BinaryOp:
        return "BinaryOp";
    case ValueType::UnaryOp:
        return "UnaryOp";
    case ValueType::Call:
        return "Call";
    case ValueType::Aggregate:
        return "Aggregate";
    case ValueType::GetAggregateMember:
        return "GetAggregateMember";
    case ValueType::MethodCall:
        return "MethodCall";
    case ValueType::MakeEnvironment:
        return "MakeEnvironment";
    case ValueType::MakeClosure:
        return "MakeClosure";
    case ValueType::MakeIterator:
        return "MakeIterator";
    case ValueType::Record:
        return "Record";
    case ValueType::Container:
        return "Container";
    case ValueType::Format:
        return "Format";
    case ValueType::Error:
        return "Error";
    case ValueType::Nop:
        return "Nop";
    }
    TIRO_UNREACHABLE("Invalid ValueType.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Value
    implement(Value)
]]] */
Value Value::make_read(const LValue& target) {
    return {Read{target}};
}

Value Value::make_write(const LValue& target, const InstId& value) {
    return {Write{target, value}};
}

Value Value::make_alias(const InstId& target) {
    return {Alias{target}};
}

Value Value::make_phi(const Phi& phi) {
    return phi;
}

Value Value::make_observe_assign(const SymbolId& symbol, const LocalListId& operands) {
    return {ObserveAssign{symbol, operands}};
}

Value Value::make_publish_assign(const SymbolId& symbol, const InstId& value) {
    return {PublishAssign{symbol, value}};
}

Value Value::make_constant(const Constant& constant) {
    return constant;
}

Value Value::make_outer_environment() {
    return {OuterEnvironment{}};
}

Value Value::make_binary_op(const BinaryOpType& op, const InstId& left, const InstId& right) {
    return {BinaryOp{op, left, right}};
}

Value Value::make_unary_op(const UnaryOpType& op, const InstId& operand) {
    return {UnaryOp{op, operand}};
}

Value Value::make_call(const InstId& func, const LocalListId& args) {
    return {Call{func, args}};
}

Value Value::make_aggregate(const Aggregate& aggregate) {
    return aggregate;
}

Value Value::make_get_aggregate_member(const InstId& aggregate, const AggregateMember& member) {
    return {GetAggregateMember{aggregate, member}};
}

Value Value::make_method_call(const InstId& method, const LocalListId& args) {
    return {MethodCall{method, args}};
}

Value Value::make_make_environment(const InstId& parent, const u32& size) {
    return {MakeEnvironment{parent, size}};
}

Value Value::make_make_closure(const InstId& env, const ModuleMemberId& func) {
    return {MakeClosure{env, func}};
}

Value Value::make_make_iterator(const InstId& container) {
    return {MakeIterator{container}};
}

Value Value::make_record(const RecordId& value) {
    return {Record{value}};
}

Value Value::make_container(const ContainerType& container, const LocalListId& args) {
    return {Container{container, args}};
}

Value Value::make_format(const LocalListId& args) {
    return {Format{args}};
}

Value Value::make_error() {
    return {Error{}};
}

Value Value::make_nop() {
    return {Nop{}};
}

Value::Value(Read read)
    : type_(ValueType::Read)
    , read_(std::move(read)) {}

Value::Value(Write write)
    : type_(ValueType::Write)
    , write_(std::move(write)) {}

Value::Value(Alias alias)
    : type_(ValueType::Alias)
    , alias_(std::move(alias)) {}

Value::Value(Phi phi)
    : type_(ValueType::Phi)
    , phi_(std::move(phi)) {}

Value::Value(ObserveAssign observe_assign)
    : type_(ValueType::ObserveAssign)
    , observe_assign_(std::move(observe_assign)) {}

Value::Value(PublishAssign publish_assign)
    : type_(ValueType::PublishAssign)
    , publish_assign_(std::move(publish_assign)) {}

Value::Value(Constant constant)
    : type_(ValueType::Constant)
    , constant_(std::move(constant)) {}

Value::Value(OuterEnvironment outer_environment)
    : type_(ValueType::OuterEnvironment)
    , outer_environment_(std::move(outer_environment)) {}

Value::Value(BinaryOp binary_op)
    : type_(ValueType::BinaryOp)
    , binary_op_(std::move(binary_op)) {}

Value::Value(UnaryOp unary_op)
    : type_(ValueType::UnaryOp)
    , unary_op_(std::move(unary_op)) {}

Value::Value(Call call)
    : type_(ValueType::Call)
    , call_(std::move(call)) {}

Value::Value(Aggregate aggregate)
    : type_(ValueType::Aggregate)
    , aggregate_(std::move(aggregate)) {}

Value::Value(GetAggregateMember get_aggregate_member)
    : type_(ValueType::GetAggregateMember)
    , get_aggregate_member_(std::move(get_aggregate_member)) {}

Value::Value(MethodCall method_call)
    : type_(ValueType::MethodCall)
    , method_call_(std::move(method_call)) {}

Value::Value(MakeEnvironment make_environment)
    : type_(ValueType::MakeEnvironment)
    , make_environment_(std::move(make_environment)) {}

Value::Value(MakeClosure make_closure)
    : type_(ValueType::MakeClosure)
    , make_closure_(std::move(make_closure)) {}

Value::Value(MakeIterator make_iterator)
    : type_(ValueType::MakeIterator)
    , make_iterator_(std::move(make_iterator)) {}

Value::Value(Record record)
    : type_(ValueType::Record)
    , record_(std::move(record)) {}

Value::Value(Container container)
    : type_(ValueType::Container)
    , container_(std::move(container)) {}

Value::Value(Format format)
    : type_(ValueType::Format)
    , format_(std::move(format)) {}

Value::Value(Error error)
    : type_(ValueType::Error)
    , error_(std::move(error)) {}

Value::Value(Nop nop)
    : type_(ValueType::Nop)
    , nop_(std::move(nop)) {}

Value::~Value() {
    _destroy_value();
}

static_assert(std::is_nothrow_move_constructible_v<
                  Value::Read> && std::is_nothrow_move_assignable_v<Value::Read>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::Write> && std::is_nothrow_move_assignable_v<Value::Write>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::Alias> && std::is_nothrow_move_assignable_v<Value::Alias>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::Phi> && std::is_nothrow_move_assignable_v<Value::Phi>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::ObserveAssign> && std::is_nothrow_move_assignable_v<Value::ObserveAssign>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::PublishAssign> && std::is_nothrow_move_assignable_v<Value::PublishAssign>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::Constant> && std::is_nothrow_move_assignable_v<Value::Constant>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<
        Value::OuterEnvironment> && std::is_nothrow_move_assignable_v<Value::OuterEnvironment>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::BinaryOp> && std::is_nothrow_move_assignable_v<Value::BinaryOp>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::UnaryOp> && std::is_nothrow_move_assignable_v<Value::UnaryOp>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::Call> && std::is_nothrow_move_assignable_v<Value::Call>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::Aggregate> && std::is_nothrow_move_assignable_v<Value::Aggregate>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<
        Value::GetAggregateMember> && std::is_nothrow_move_assignable_v<Value::GetAggregateMember>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::MethodCall> && std::is_nothrow_move_assignable_v<Value::MethodCall>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<
        Value::MakeEnvironment> && std::is_nothrow_move_assignable_v<Value::MakeEnvironment>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::MakeClosure> && std::is_nothrow_move_assignable_v<Value::MakeClosure>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::MakeIterator> && std::is_nothrow_move_assignable_v<Value::MakeIterator>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::Record> && std::is_nothrow_move_assignable_v<Value::Record>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::Container> && std::is_nothrow_move_assignable_v<Value::Container>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::Format> && std::is_nothrow_move_assignable_v<Value::Format>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::Error> && std::is_nothrow_move_assignable_v<Value::Error>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Value::Nop> && std::is_nothrow_move_assignable_v<Value::Nop>,
    "Only nothrow movable types are supported in generated unions.");

Value::Value(Value&& other) noexcept
    : type_(other.type()) {
    _move_construct_value(other);
}

Value& Value::operator=(Value&& other) noexcept {
    TIRO_DEBUG_ASSERT(this != &other, "Self move assignment is invalid.");
    if (type() == other.type()) {
        _move_assign_value(other);
    } else {
        _destroy_value();
        _move_construct_value(other);
        type_ = other.type();
    }
    return *this;
}

const Value::Read& Value::as_read() const {
    TIRO_DEBUG_ASSERT(type_ == ValueType::Read, "Bad member access on Value: not a Read.");
    return read_;
}

Value::Read& Value::as_read() {
    return const_cast<Read&>(const_cast<const Value*>(this)->as_read());
}

const Value::Write& Value::as_write() const {
    TIRO_DEBUG_ASSERT(type_ == ValueType::Write, "Bad member access on Value: not a Write.");
    return write_;
}

Value::Write& Value::as_write() {
    return const_cast<Write&>(const_cast<const Value*>(this)->as_write());
}

const Value::Alias& Value::as_alias() const {
    TIRO_DEBUG_ASSERT(type_ == ValueType::Alias, "Bad member access on Value: not a Alias.");
    return alias_;
}

Value::Alias& Value::as_alias() {
    return const_cast<Alias&>(const_cast<const Value*>(this)->as_alias());
}

const Value::Phi& Value::as_phi() const {
    TIRO_DEBUG_ASSERT(type_ == ValueType::Phi, "Bad member access on Value: not a Phi.");
    return phi_;
}

Value::Phi& Value::as_phi() {
    return const_cast<Phi&>(const_cast<const Value*>(this)->as_phi());
}

const Value::ObserveAssign& Value::as_observe_assign() const {
    TIRO_DEBUG_ASSERT(
        type_ == ValueType::ObserveAssign, "Bad member access on Value: not a ObserveAssign.");
    return observe_assign_;
}

Value::ObserveAssign& Value::as_observe_assign() {
    return const_cast<ObserveAssign&>(const_cast<const Value*>(this)->as_observe_assign());
}

const Value::PublishAssign& Value::as_publish_assign() const {
    TIRO_DEBUG_ASSERT(
        type_ == ValueType::PublishAssign, "Bad member access on Value: not a PublishAssign.");
    return publish_assign_;
}

Value::PublishAssign& Value::as_publish_assign() {
    return const_cast<PublishAssign&>(const_cast<const Value*>(this)->as_publish_assign());
}

const Value::Constant& Value::as_constant() const {
    TIRO_DEBUG_ASSERT(type_ == ValueType::Constant, "Bad member access on Value: not a Constant.");
    return constant_;
}

Value::Constant& Value::as_constant() {
    return const_cast<Constant&>(const_cast<const Value*>(this)->as_constant());
}

const Value::OuterEnvironment& Value::as_outer_environment() const {
    TIRO_DEBUG_ASSERT(type_ == ValueType::OuterEnvironment,
        "Bad member access on Value: not a OuterEnvironment.");
    return outer_environment_;
}

Value::OuterEnvironment& Value::as_outer_environment() {
    return const_cast<OuterEnvironment&>(const_cast<const Value*>(this)->as_outer_environment());
}

const Value::BinaryOp& Value::as_binary_op() const {
    TIRO_DEBUG_ASSERT(type_ == ValueType::BinaryOp, "Bad member access on Value: not a BinaryOp.");
    return binary_op_;
}

Value::BinaryOp& Value::as_binary_op() {
    return const_cast<BinaryOp&>(const_cast<const Value*>(this)->as_binary_op());
}

const Value::UnaryOp& Value::as_unary_op() const {
    TIRO_DEBUG_ASSERT(type_ == ValueType::UnaryOp, "Bad member access on Value: not a UnaryOp.");
    return unary_op_;
}

Value::UnaryOp& Value::as_unary_op() {
    return const_cast<UnaryOp&>(const_cast<const Value*>(this)->as_unary_op());
}

const Value::Call& Value::as_call() const {
    TIRO_DEBUG_ASSERT(type_ == ValueType::Call, "Bad member access on Value: not a Call.");
    return call_;
}

Value::Call& Value::as_call() {
    return const_cast<Call&>(const_cast<const Value*>(this)->as_call());
}

const Value::Aggregate& Value::as_aggregate() const {
    TIRO_DEBUG_ASSERT(
        type_ == ValueType::Aggregate, "Bad member access on Value: not a Aggregate.");
    return aggregate_;
}

Value::Aggregate& Value::as_aggregate() {
    return const_cast<Aggregate&>(const_cast<const Value*>(this)->as_aggregate());
}

const Value::GetAggregateMember& Value::as_get_aggregate_member() const {
    TIRO_DEBUG_ASSERT(type_ == ValueType::GetAggregateMember,
        "Bad member access on Value: not a GetAggregateMember.");
    return get_aggregate_member_;
}

Value::GetAggregateMember& Value::as_get_aggregate_member() {
    return const_cast<GetAggregateMember&>(
        const_cast<const Value*>(this)->as_get_aggregate_member());
}

const Value::MethodCall& Value::as_method_call() const {
    TIRO_DEBUG_ASSERT(
        type_ == ValueType::MethodCall, "Bad member access on Value: not a MethodCall.");
    return method_call_;
}

Value::MethodCall& Value::as_method_call() {
    return const_cast<MethodCall&>(const_cast<const Value*>(this)->as_method_call());
}

const Value::MakeEnvironment& Value::as_make_environment() const {
    TIRO_DEBUG_ASSERT(
        type_ == ValueType::MakeEnvironment, "Bad member access on Value: not a MakeEnvironment.");
    return make_environment_;
}

Value::MakeEnvironment& Value::as_make_environment() {
    return const_cast<MakeEnvironment&>(const_cast<const Value*>(this)->as_make_environment());
}

const Value::MakeClosure& Value::as_make_closure() const {
    TIRO_DEBUG_ASSERT(
        type_ == ValueType::MakeClosure, "Bad member access on Value: not a MakeClosure.");
    return make_closure_;
}

Value::MakeClosure& Value::as_make_closure() {
    return const_cast<MakeClosure&>(const_cast<const Value*>(this)->as_make_closure());
}

const Value::MakeIterator& Value::as_make_iterator() const {
    TIRO_DEBUG_ASSERT(
        type_ == ValueType::MakeIterator, "Bad member access on Value: not a MakeIterator.");
    return make_iterator_;
}

Value::MakeIterator& Value::as_make_iterator() {
    return const_cast<MakeIterator&>(const_cast<const Value*>(this)->as_make_iterator());
}

const Value::Record& Value::as_record() const {
    TIRO_DEBUG_ASSERT(type_ == ValueType::Record, "Bad member access on Value: not a Record.");
    return record_;
}

Value::Record& Value::as_record() {
    return const_cast<Record&>(const_cast<const Value*>(this)->as_record());
}

const Value::Container& Value::as_container() const {
    TIRO_DEBUG_ASSERT(
        type_ == ValueType::Container, "Bad member access on Value: not a Container.");
    return container_;
}

Value::Container& Value::as_container() {
    return const_cast<Container&>(const_cast<const Value*>(this)->as_container());
}

const Value::Format& Value::as_format() const {
    TIRO_DEBUG_ASSERT(type_ == ValueType::Format, "Bad member access on Value: not a Format.");
    return format_;
}

Value::Format& Value::as_format() {
    return const_cast<Format&>(const_cast<const Value*>(this)->as_format());
}

const Value::Error& Value::as_error() const {
    TIRO_DEBUG_ASSERT(type_ == ValueType::Error, "Bad member access on Value: not a Error.");
    return error_;
}

Value::Error& Value::as_error() {
    return const_cast<Error&>(const_cast<const Value*>(this)->as_error());
}

const Value::Nop& Value::as_nop() const {
    TIRO_DEBUG_ASSERT(type_ == ValueType::Nop, "Bad member access on Value: not a Nop.");
    return nop_;
}

Value::Nop& Value::as_nop() {
    return const_cast<Nop&>(const_cast<const Value*>(this)->as_nop());
}

void Value::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_read([[maybe_unused]] const Read& read) {
            stream.format("Read(target: {})", read.target);
        }

        void visit_write([[maybe_unused]] const Write& write) {
            stream.format("Write(target: {}, value: {})", write.target, write.value);
        }

        void visit_alias([[maybe_unused]] const Alias& alias) {
            stream.format("Alias(target: {})", alias.target);
        }

        void visit_phi([[maybe_unused]] const Phi& phi) { stream.format("{}", phi); }

        void visit_observe_assign([[maybe_unused]] const ObserveAssign& observe_assign) {
            stream.format("ObserveAssign(symbol: {}, operands: {})", observe_assign.symbol,
                observe_assign.operands);
        }

        void visit_publish_assign([[maybe_unused]] const PublishAssign& publish_assign) {
            stream.format("PublishAssign(symbol: {}, value: {})", publish_assign.symbol,
                publish_assign.value);
        }

        void visit_constant([[maybe_unused]] const Constant& constant) {
            stream.format("{}", constant);
        }

        void visit_outer_environment([[maybe_unused]] const OuterEnvironment& outer_environment) {
            stream.format("OuterEnvironment");
        }

        void visit_binary_op([[maybe_unused]] const BinaryOp& binary_op) {
            stream.format("BinaryOp(op: {}, left: {}, right: {})", binary_op.op, binary_op.left,
                binary_op.right);
        }

        void visit_unary_op([[maybe_unused]] const UnaryOp& unary_op) {
            stream.format("UnaryOp(op: {}, operand: {})", unary_op.op, unary_op.operand);
        }

        void visit_call([[maybe_unused]] const Call& call) {
            stream.format("Call(func: {}, args: {})", call.func, call.args);
        }

        void visit_aggregate([[maybe_unused]] const Aggregate& aggregate) {
            stream.format("{}", aggregate);
        }

        void visit_get_aggregate_member(
            [[maybe_unused]] const GetAggregateMember& get_aggregate_member) {
            stream.format("GetAggregateMember(aggregate: {}, member: {})",
                get_aggregate_member.aggregate, get_aggregate_member.member);
        }

        void visit_method_call([[maybe_unused]] const MethodCall& method_call) {
            stream.format("MethodCall(method: {}, args: {})", method_call.method, method_call.args);
        }

        void visit_make_environment([[maybe_unused]] const MakeEnvironment& make_environment) {
            stream.format("MakeEnvironment(parent: {}, size: {})", make_environment.parent,
                make_environment.size);
        }

        void visit_make_closure([[maybe_unused]] const MakeClosure& make_closure) {
            stream.format("MakeClosure(env: {}, func: {})", make_closure.env, make_closure.func);
        }

        void visit_make_iterator([[maybe_unused]] const MakeIterator& make_iterator) {
            stream.format("MakeIterator(container: {})", make_iterator.container);
        }

        void visit_record([[maybe_unused]] const Record& record) {
            stream.format("Record(value: {})", record.value);
        }

        void visit_container([[maybe_unused]] const Container& container) {
            stream.format(
                "Container(container: {}, args: {})", container.container, container.args);
        }

        void visit_format([[maybe_unused]] const Format& format) {
            stream.format("Format(args: {})", format.args);
        }

        void visit_error([[maybe_unused]] const Error& error) { stream.format("Error"); }

        void visit_nop([[maybe_unused]] const Nop& nop) { stream.format("Nop"); }
    };
    visit(FormatVisitor{stream});
}

void Value::_destroy_value() noexcept {
    struct DestroyVisitor {
        void visit_read(Read& read) { read.~Read(); }

        void visit_write(Write& write) { write.~Write(); }

        void visit_alias(Alias& alias) { alias.~Alias(); }

        void visit_phi(Phi& phi) { phi.~Phi(); }

        void visit_observe_assign(ObserveAssign& observe_assign) {
            observe_assign.~ObserveAssign();
        }

        void visit_publish_assign(PublishAssign& publish_assign) {
            publish_assign.~PublishAssign();
        }

        void visit_constant(Constant& constant) { constant.~Constant(); }

        void visit_outer_environment(OuterEnvironment& outer_environment) {
            outer_environment.~OuterEnvironment();
        }

        void visit_binary_op(BinaryOp& binary_op) { binary_op.~BinaryOp(); }

        void visit_unary_op(UnaryOp& unary_op) { unary_op.~UnaryOp(); }

        void visit_call(Call& call) { call.~Call(); }

        void visit_aggregate(Aggregate& aggregate) { aggregate.~Aggregate(); }

        void visit_get_aggregate_member(GetAggregateMember& get_aggregate_member) {
            get_aggregate_member.~GetAggregateMember();
        }

        void visit_method_call(MethodCall& method_call) { method_call.~MethodCall(); }

        void visit_make_environment(MakeEnvironment& make_environment) {
            make_environment.~MakeEnvironment();
        }

        void visit_make_closure(MakeClosure& make_closure) { make_closure.~MakeClosure(); }

        void visit_make_iterator(MakeIterator& make_iterator) { make_iterator.~MakeIterator(); }

        void visit_record(Record& record) { record.~Record(); }

        void visit_container(Container& container) { container.~Container(); }

        void visit_format(Format& format) { format.~Format(); }

        void visit_error(Error& error) { error.~Error(); }

        void visit_nop(Nop& nop) { nop.~Nop(); }
    };
    visit(DestroyVisitor{});
}

void Value::_move_construct_value(Value& other) noexcept {
    struct ConstructVisitor {
        Value* self;

        void visit_read(Read& read) { new (&self->read_) Read(std::move(read)); }

        void visit_write(Write& write) { new (&self->write_) Write(std::move(write)); }

        void visit_alias(Alias& alias) { new (&self->alias_) Alias(std::move(alias)); }

        void visit_phi(Phi& phi) { new (&self->phi_) Phi(std::move(phi)); }

        void visit_observe_assign(ObserveAssign& observe_assign) {
            new (&self->observe_assign_) ObserveAssign(std::move(observe_assign));
        }

        void visit_publish_assign(PublishAssign& publish_assign) {
            new (&self->publish_assign_) PublishAssign(std::move(publish_assign));
        }

        void visit_constant(Constant& constant) {
            new (&self->constant_) Constant(std::move(constant));
        }

        void visit_outer_environment(OuterEnvironment& outer_environment) {
            new (&self->outer_environment_) OuterEnvironment(std::move(outer_environment));
        }

        void visit_binary_op(BinaryOp& binary_op) {
            new (&self->binary_op_) BinaryOp(std::move(binary_op));
        }

        void visit_unary_op(UnaryOp& unary_op) {
            new (&self->unary_op_) UnaryOp(std::move(unary_op));
        }

        void visit_call(Call& call) { new (&self->call_) Call(std::move(call)); }

        void visit_aggregate(Aggregate& aggregate) {
            new (&self->aggregate_) Aggregate(std::move(aggregate));
        }

        void visit_get_aggregate_member(GetAggregateMember& get_aggregate_member) {
            new (&self->get_aggregate_member_) GetAggregateMember(std::move(get_aggregate_member));
        }

        void visit_method_call(MethodCall& method_call) {
            new (&self->method_call_) MethodCall(std::move(method_call));
        }

        void visit_make_environment(MakeEnvironment& make_environment) {
            new (&self->make_environment_) MakeEnvironment(std::move(make_environment));
        }

        void visit_make_closure(MakeClosure& make_closure) {
            new (&self->make_closure_) MakeClosure(std::move(make_closure));
        }

        void visit_make_iterator(MakeIterator& make_iterator) {
            new (&self->make_iterator_) MakeIterator(std::move(make_iterator));
        }

        void visit_record(Record& record) { new (&self->record_) Record(std::move(record)); }

        void visit_container(Container& container) {
            new (&self->container_) Container(std::move(container));
        }

        void visit_format(Format& format) { new (&self->format_) Format(std::move(format)); }

        void visit_error(Error& error) { new (&self->error_) Error(std::move(error)); }

        void visit_nop(Nop& nop) { new (&self->nop_) Nop(std::move(nop)); }
    };
    other.visit(ConstructVisitor{this});
}

void Value::_move_assign_value(Value& other) noexcept {
    struct AssignVisitor {
        Value* self;

        void visit_read(Read& read) { self->read_ = std::move(read); }

        void visit_write(Write& write) { self->write_ = std::move(write); }

        void visit_alias(Alias& alias) { self->alias_ = std::move(alias); }

        void visit_phi(Phi& phi) { self->phi_ = std::move(phi); }

        void visit_observe_assign(ObserveAssign& observe_assign) {
            self->observe_assign_ = std::move(observe_assign);
        }

        void visit_publish_assign(PublishAssign& publish_assign) {
            self->publish_assign_ = std::move(publish_assign);
        }

        void visit_constant(Constant& constant) { self->constant_ = std::move(constant); }

        void visit_outer_environment(OuterEnvironment& outer_environment) {
            self->outer_environment_ = std::move(outer_environment);
        }

        void visit_binary_op(BinaryOp& binary_op) { self->binary_op_ = std::move(binary_op); }

        void visit_unary_op(UnaryOp& unary_op) { self->unary_op_ = std::move(unary_op); }

        void visit_call(Call& call) { self->call_ = std::move(call); }

        void visit_aggregate(Aggregate& aggregate) { self->aggregate_ = std::move(aggregate); }

        void visit_get_aggregate_member(GetAggregateMember& get_aggregate_member) {
            self->get_aggregate_member_ = std::move(get_aggregate_member);
        }

        void visit_method_call(MethodCall& method_call) {
            self->method_call_ = std::move(method_call);
        }

        void visit_make_environment(MakeEnvironment& make_environment) {
            self->make_environment_ = std::move(make_environment);
        }

        void visit_make_closure(MakeClosure& make_closure) {
            self->make_closure_ = std::move(make_closure);
        }

        void visit_make_iterator(MakeIterator& make_iterator) {
            self->make_iterator_ = std::move(make_iterator);
        }

        void visit_record(Record& record) { self->record_ = std::move(record); }

        void visit_container(Container& container) { self->container_ = std::move(container); }

        void visit_format(Format& format) { self->format_ = std::move(format); }

        void visit_error(Error& error) { self->error_ = std::move(error); }

        void visit_nop(Nop& nop) { self->nop_ = std::move(nop); }
    };
    other.visit(AssignVisitor{this});
}

// [[[end]]]

bool is_phi_define(const Function& func, InstId inst_id) {
    if (!inst_id)
        return false;

    const auto& inst = func[inst_id];
    return inst.value().type() == ValueType::Phi;
}

} // namespace tiro::ir
