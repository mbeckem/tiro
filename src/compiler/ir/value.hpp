#ifndef TIRO_COMPILER_IR_VALUE_HPP
#define TIRO_COMPILER_IR_VALUE_HPP

#include "common/defs.hpp"
#include "common/text/string_table.hpp"
#include "compiler/ir/entities.hpp"
#include "compiler/ir/fwd.hpp"
#include "compiler/semantics/symbol_table.hpp"

namespace tiro::ir {

/// Represents the type of a binary operation.
enum class BinaryOpType : u8 {
    Plus,
    Minus,
    Multiply,
    Divide,
    Modulus,
    Power,
    LeftShift,
    RightShift,
    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,
    Less,
    LessEquals,
    Greater,
    GreaterEquals,
    Equals,
    NotEquals,
};

std::string_view to_string(BinaryOpType type);

/// Represents the type of a unary operation.
enum class UnaryOpType : u8 { Plus, Minus, BitwiseNot, LogicalNot };

std::string_view to_string(UnaryOpType type);

/// Repesents the type of a created container.
enum class ContainerType : u8 { Array, Tuple, Set, Map };

std::string_view to_string(ContainerType type);

/* [[[cog
    from codegen.unions import define
    from codegen.ir import LValue
    define(LValue.tag)
]]] */
enum class LValueType : u8 {
    Param,
    Closure,
    Module,
    Field,
    TupleField,
    Index,
};

std::string_view to_string(LValueType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ir import LValue
    define(LValue)
]]] */
/// LValues can appear as the left hand side of an assignment.
/// They are associated with a mutable storage location.
/// LValues do not use SSA form since they may reference memory shared
/// with other parts of the program.
class LValue final {
public:
    /// Reference to a function argument.
    struct Param final {
        /// Argument index in parameter list.
        ParamId target;

        explicit Param(const ParamId& target_)
            : target(target_) {}
    };

    /// Reference to a variable captured from an outer scope.
    struct Closure final {
        /// The environment to search. Either a local variable or the function's outer environment.
        InstId env;

        /// Levels to "go up" the environment hierarchy. 0 is the closure environment itself.
        u32 levels;

        /// Index into the environment.
        u32 index;

        Closure(const InstId& env_, const u32& levels_, const u32& index_)
            : env(env_)
            , levels(levels_)
            , index(index_) {}
    };

    /// Reference to a variable at module scope.
    struct Module final {
        /// Id of the module level variable.
        ModuleMemberId member;

        explicit Module(const ModuleMemberId& member_)
            : member(member_) {}
    };

    /// Reference to the field of an object (i.e. `object.foo`).
    struct Field final {
        /// Dereferenced object.
        InstId object;

        /// Field name to access.
        InternedString name;

        Field(const InstId& object_, const InternedString& name_)
            : object(object_)
            , name(name_) {}
    };

    /// Reference to a tuple field of a tuple (i.e. `tuple.3`).
    struct TupleField final {
        /// Dereferenced tuple object.
        InstId object;

        /// Index of the tuple member.
        u32 index;

        TupleField(const InstId& object_, const u32& index_)
            : object(object_)
            , index(index_) {}
    };

    /// Reference to an index of an array (or a map), i.e. `thing[foo]`.
    struct Index final {
        /// Dereferenced arraylike object.
        InstId object;

        /// Index into the array.
        InstId index;

        Index(const InstId& object_, const InstId& index_)
            : object(object_)
            , index(index_) {}
    };

    static LValue make_param(const ParamId& target);
    static LValue make_closure(const InstId& env, const u32& levels, const u32& index);
    static LValue make_module(const ModuleMemberId& member);
    static LValue make_field(const InstId& object, const InternedString& name);
    static LValue make_tuple_field(const InstId& object, const u32& index);
    static LValue make_index(const InstId& object, const InstId& index);

    LValue(Param param);
    LValue(Closure closure);
    LValue(Module module);
    LValue(Field field);
    LValue(TupleField tuple_field);
    LValue(Index index);

    LValueType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Param& as_param() const;
    const Closure& as_closure() const;
    const Module& as_module() const;
    const Field& as_field() const;
    const TupleField& as_tuple_field() const;
    const Index& as_index() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto) visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    LValueType type_;
    union {
        Param param_;
        Closure closure_;
        Module module_;
        Field field_;
        TupleField tuple_field_;
        Index index_;
    };
};
// [[[end]]]

/// Represents a phi node. Phi nodes are used at control flow join points to record the
/// fact that an SSA value may have one of multiple possible values, depending on the code path
/// used to reach the value.
class Phi final {
public:
    Phi();

    explicit Phi(LocalListId operands);

    Phi(Function& func, std::initializer_list<InstId> locals);

    LocalListId operands() const { return operands_; }
    void operands(LocalListId list_id) { operands_ = list_id; }

    void append_operand(Function& func, InstId operand);

    InstId operand(const Function& func, size_t index) const;
    void operand(Function& func, size_t index, InstId operand);
    size_t operand_count(const Function& func) const;

    // TODO
    void format(FormatStream& stream) const {
        (void) stream;
        TIRO_UNREACHABLE("DO NOT CALL");
    }

private:
    LocalListId operands_;
};

/* [[[cog
    from codegen.unions import define
    from codegen.ir import Constant
    define(Constant.tag)
]]] */
enum class ConstantType : u8 {
    Integer,
    Float,
    String,
    Symbol,
    Null,
    True,
    False,
};

std::string_view to_string(ConstantType type);
// [[[end]]]

/// Represents a floating point constant.
/// The important difference between this and the plain floating point type is
/// that this class treats "nan" as equal to itself.
/// This enables us to store floating point constants in containers (e.g. for value numbering).
struct FloatConstant final {
    f64 value;

    FloatConstant(f64 value_)
        : value(value_) {}

    operator f64() const noexcept { return value; }

    void format(FormatStream& stream) const;
    void hash(Hasher& h) const;
};

bool operator==(const FloatConstant& lhs, const FloatConstant& rhs);
bool operator!=(const FloatConstant& lhs, const FloatConstant& rhs);
bool operator<(const FloatConstant& lhs, const FloatConstant& rhs);
bool operator>(const FloatConstant& lhs, const FloatConstant& rhs);
bool operator<=(const FloatConstant& lhs, const FloatConstant& rhs);
bool operator>=(const FloatConstant& lhs, const FloatConstant& rhs);

/* [[[cog
    from codegen.unions import define
    from codegen.ir import Constant
    define(Constant)
]]] */
/// Represents a compile time constant.
class Constant final {
public:
    struct Integer final {
        i64 value;

        explicit Integer(const i64& value_)
            : value(value_) {}
    };

    using Float = FloatConstant;

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

    struct Null final {};

    struct True final {};

    struct False final {};

    static Constant make_integer(const i64& value);
    static Constant make_float(const Float& f);
    static Constant make_string(const InternedString& value);
    static Constant make_symbol(const InternedString& value);
    static Constant make_null();
    static Constant make_true();
    static Constant make_false();

    Constant(Integer integer);
    Constant(Float f);
    Constant(String string);
    Constant(Symbol symbol);
    Constant(Null null);
    Constant(True t);
    Constant(False f);

    ConstantType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    void hash(Hasher& h) const;

    const Integer& as_integer() const;
    const Float& as_float() const;
    const String& as_string() const;
    const Symbol& as_symbol() const;
    const Null& as_null() const;
    const True& as_true() const;
    const False& as_false() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto) visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    ConstantType type_;
    union {
        Integer integer_;
        Float float_;
        String string_;
        Symbol symbol_;
        Null null_;
        True true_;
        False false_;
    };
};

bool operator==(const Constant& lhs, const Constant& rhs);
bool operator!=(const Constant& lhs, const Constant& rhs);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ir import Aggregate
    define(Aggregate.tag)
]]] */
enum class AggregateType : u8 {
    Method,
    IteratorNext,
};

std::string_view to_string(AggregateType type);
/// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ir import Aggregate
    define(Aggregate)
]]] */
/// Represents the compile time type of an aggregate value.
/// Aggregate values are an aggregate of other values, which (at this time)
/// only exist as virtual entities at IR level.
/// The main use case right now is to group member instances and method pointers
/// or efficient method calls.
class Aggregate final {
public:
    /// Represents a method invocation (returns instance, method pointer).
    struct Method final {
        InstId instance;
        InternedString function;

        Method(const InstId& instance_, const InternedString& function_)
            : instance(instance_)
            , function(function_) {}
    };

    /// Represents the result of advancing an iterator (returns valid, value).
    struct IteratorNext final {
        InstId iterator;

        explicit IteratorNext(const InstId& iterator_)
            : iterator(iterator_) {}
    };

    static Aggregate make_method(const InstId& instance, const InternedString& function);
    static Aggregate make_iterator_next(const InstId& iterator);

    Aggregate(Method method);
    Aggregate(IteratorNext iterator_next);

    AggregateType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    void hash(Hasher& h) const;

    const Method& as_method() const;
    const IteratorNext& as_iterator_next() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto) visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    AggregateType type_;
    union {
        Method method_;
        IteratorNext iterator_next_;
    };
};

bool operator==(const Aggregate& lhs, const Aggregate& rhs);
bool operator!=(const Aggregate& lhs, const Aggregate& rhs);
/// [[[end]]]

/// Identifies the member of an aggregate. For this initial implementation
/// all members share a common namespace. Functions using aggregates must
/// check that the member id and the actual aggregate type match.
enum class AggregateMember : u8 {
    /// The instance a method is being called on.
    MethodInstance = 1,

    /// The method function being called.
    MethodFunction,

    /// A boolean that is true if the iterator returned a valid value.
    IteratorNextValid,

    /// The value returned by the iterator.
    IteratorNextValue,
};

/// Returns the required aggregate type for the given member.
AggregateType aggregate_type(AggregateMember member);

std::string_view to_string(AggregateMember member);

/* [[[cog
    from codegen.unions import define
    from codegen.ir import Value
    define(Value.tag)
]]] */
enum class ValueType : u8 {
    Read,
    Write,
    Alias,
    Phi,
    ObserveAssign,
    PublishAssign,
    Constant,
    OuterEnvironment,
    BinaryOp,
    UnaryOp,
    Call,
    Aggregate,
    GetAggregateMember,
    MethodCall,
    MakeEnvironment,
    MakeClosure,
    MakeIterator,
    Record,
    Container,
    Format,
    Error,
    Nop,
};

std::string_view to_string(ValueType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ir import Value
    define(Value)
]]] */
/// Represents the value of an instruction.
///
/// Values at this compilation stage do not allow inner control flow. Nested
/// language-level expressions that contain loops or conditionals are split up
/// so that only "simple" expressions remain.
class Value final {
public:
    /// Read from an lvalue to produce a value.
    struct Read final {
        /// Dereferenced lvalue.
        LValue target;

        explicit Read(const LValue& target_)
            : target(target_) {}
    };

    /// Write to an lvalue. Write operations are side effects only.
    /// The result of a write should not be an operand for other instructions.
    struct Write final {
        /// The write target.
        LValue target;

        /// The new value.
        InstId value;

        Write(const LValue& target_, const InstId& value_)
            : target(target_)
            , value(value_) {}
    };

    /// References the value of another instruction.
    struct Alias final {
        /// Used instruction.
        InstId target;

        explicit Alias(const InstId& target_)
            : target(target_) {}
    };

    /// Phi nodes can have one of multiple instructions as their value, depending on the code path that led to them.
    using Phi = tiro::ir::Phi;

    /// Similar to phi nodes, but used for exception handling.
    /// All operands must be PublishAssign values, and all such values must belong to the same symbol.
    struct ObserveAssign final {
        /// The symbol (variable) observed by this node.
        SymbolId symbol;

        /// The list of assignments observed by this node.
        LocalListId operands;

        ObserveAssign(const SymbolId& symbol_, const LocalListId& operands_)
            : symbol(symbol_)
            , operands(operands_) {}
    };

    /// Marks the assignment of an ssa variable to a new value.
    /// This is needed for exception handlers, which must be able to observe side effects.
    /// Optimized out when not actually used.
    struct PublishAssign final {
        /// The reassigned symbol.
        SymbolId symbol;

        /// The new SSA value.
        InstId value;

        PublishAssign(const SymbolId& symbol_, const InstId& value_)
            : symbol(symbol_)
            , value(value_) {}
    };

    /// A constant.
    using Constant = tiro::ir::Constant;

    /// Dereferences the function's outer closure environment
    struct OuterEnvironment final {};

    /// Simple binary operation.
    struct BinaryOp final {
        BinaryOpType op;

        /// Left operand.
        InstId left;

        /// Right operand.
        InstId right;

        BinaryOp(const BinaryOpType& op_, const InstId& left_, const InstId& right_)
            : op(op_)
            , left(left_)
            , right(right_) {}
    };

    /// Simple unary operation.
    struct UnaryOp final {
        UnaryOpType op;
        InstId operand;

        UnaryOp(const UnaryOpType& op_, const InstId& operand_)
            : op(op_)
            , operand(operand_) {}
    };

    /// Function call expression, i.e. `f(a, b, c)`.
    struct Call final {
        /// Function to call.
        InstId func;

        /// The list of function arguments.
        LocalListId args;

        Call(const InstId& func_, const LocalListId& args_)
            : func(func_)
            , args(args_) {}
    };

    /// Represents an aggregate value.
    using Aggregate = tiro::ir::Aggregate;

    /// Fetches a member value from an aggregate.
    struct GetAggregateMember final {
        /// Must be an aggregate value of the correct type.
        InstId aggregate;

        /// The aggregate member returned from the aggregate.
        AggregateMember member;

        GetAggregateMember(const InstId& aggregate_, const AggregateMember& member_)
            : aggregate(aggregate_)
            , member(member_) {}
    };

    /// Method call expression, i.e `a.b(c, d)`.
    struct MethodCall final {
        /// Method to be called. Must be a method value.
        InstId method;

        /// List of method arguments.
        LocalListId args;

        MethodCall(const InstId& method_, const LocalListId& args_)
            : method(method_)
            , args(args_) {}
    };

    /// Creates a new closure environment.
    struct MakeEnvironment final {
        /// The parent environment.
        InstId parent;

        /// The number of variable slots in the new environment.
        u32 size;

        MakeEnvironment(const InstId& parent_, const u32& size_)
            : parent(parent_)
            , size(size_) {}
    };

    /// Creates a new closure function.
    struct MakeClosure final {
        /// The closure environment.
        InstId env;

        /// The closure function's template location.
        ModuleMemberId func;

        MakeClosure(const InstId& env_, const ModuleMemberId& func_)
            : env(env_)
            , func(func_) {}
    };

    /// Creates a new iterator for a given container instance.
    struct MakeIterator final {
        /// The container being iterated.
        InstId container;

        explicit MakeIterator(const InstId& container_)
            : container(container_) {}
    };

    /// Creates a new record.
    struct Record final {
        /// Points to the record's content.
        RecordId value;

        explicit Record(const RecordId& value_)
            : value(value_) {}
    };

    /// Construct a container from the argument list,
    /// such as an array, a tuple or a map.
    struct Container final {
        /// Container type we're going to construct.
        ContainerType container;

        /// Arguments for the container constructor (list of elements,
        /// or list of key/value-pairs in the case of Map and Record).
        LocalListId args;

        Container(const ContainerType& container_, const LocalListId& args_)
            : container(container_)
            , args(args_) {}
    };

    /// Takes a list of values and formats them as a string.
    /// This is used to implement format string literals.
    struct Format final {
        /// The list of values.
        LocalListId args;

        explicit Format(const LocalListId& args_)
            : args(args_) {}
    };

    /// Represents an error value that was generated to continue with the translation (for analysis).
    /// Never present in a valid program.
    struct Error final {};

    /// A value that has been optimized out.
    struct Nop final {};

    static Value make_read(const LValue& target);
    static Value make_write(const LValue& target, const InstId& value);
    static Value make_alias(const InstId& target);
    static Value make_phi(const Phi& phi);
    static Value make_observe_assign(const SymbolId& symbol, const LocalListId& operands);
    static Value make_publish_assign(const SymbolId& symbol, const InstId& value);
    static Value make_constant(const Constant& constant);
    static Value make_outer_environment();
    static Value make_binary_op(const BinaryOpType& op, const InstId& left, const InstId& right);
    static Value make_unary_op(const UnaryOpType& op, const InstId& operand);
    static Value make_call(const InstId& func, const LocalListId& args);
    static Value make_aggregate(const Aggregate& aggregate);
    static Value make_get_aggregate_member(const InstId& aggregate, const AggregateMember& member);
    static Value make_method_call(const InstId& method, const LocalListId& args);
    static Value make_make_environment(const InstId& parent, const u32& size);
    static Value make_make_closure(const InstId& env, const ModuleMemberId& func);
    static Value make_make_iterator(const InstId& container);
    static Value make_record(const RecordId& value);
    static Value make_container(const ContainerType& container, const LocalListId& args);
    static Value make_format(const LocalListId& args);
    static Value make_error();
    static Value make_nop();

    Value(Read read);
    Value(Write write);
    Value(Alias alias);
    Value(Phi phi);
    Value(ObserveAssign observe_assign);
    Value(PublishAssign publish_assign);
    Value(Constant constant);
    Value(OuterEnvironment outer_environment);
    Value(BinaryOp binary_op);
    Value(UnaryOp unary_op);
    Value(Call call);
    Value(Aggregate aggregate);
    Value(GetAggregateMember get_aggregate_member);
    Value(MethodCall method_call);
    Value(MakeEnvironment make_environment);
    Value(MakeClosure make_closure);
    Value(MakeIterator make_iterator);
    Value(Record record);
    Value(Container container);
    Value(Format format);
    Value(Error error);
    Value(Nop nop);

    ~Value();

    Value(Value&& other) noexcept;
    Value& operator=(Value&& other) noexcept;

    ValueType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Read& as_read() const;
    Read& as_read();

    const Write& as_write() const;
    Write& as_write();

    const Alias& as_alias() const;
    Alias& as_alias();

    const Phi& as_phi() const;
    Phi& as_phi();

    const ObserveAssign& as_observe_assign() const;
    ObserveAssign& as_observe_assign();

    const PublishAssign& as_publish_assign() const;
    PublishAssign& as_publish_assign();

    const Constant& as_constant() const;
    Constant& as_constant();

    const OuterEnvironment& as_outer_environment() const;
    OuterEnvironment& as_outer_environment();

    const BinaryOp& as_binary_op() const;
    BinaryOp& as_binary_op();

    const UnaryOp& as_unary_op() const;
    UnaryOp& as_unary_op();

    const Call& as_call() const;
    Call& as_call();

    const Aggregate& as_aggregate() const;
    Aggregate& as_aggregate();

    const GetAggregateMember& as_get_aggregate_member() const;
    GetAggregateMember& as_get_aggregate_member();

    const MethodCall& as_method_call() const;
    MethodCall& as_method_call();

    const MakeEnvironment& as_make_environment() const;
    MakeEnvironment& as_make_environment();

    const MakeClosure& as_make_closure() const;
    MakeClosure& as_make_closure();

    const MakeIterator& as_make_iterator() const;
    MakeIterator& as_make_iterator();

    const Record& as_record() const;
    Record& as_record();

    const Container& as_container() const;
    Container& as_container();

    const Format& as_format() const;
    Format& as_format();

    const Error& as_error() const;
    Error& as_error();

    const Nop& as_nop() const;
    Nop& as_nop();

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    void _destroy_value() noexcept;
    void _move_construct_value(Value& other) noexcept;
    void _move_assign_value(Value& other) noexcept;

    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto) visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    ValueType type_;
    union {
        Read read_;
        Write write_;
        Alias alias_;
        Phi phi_;
        ObserveAssign observe_assign_;
        PublishAssign publish_assign_;
        Constant constant_;
        OuterEnvironment outer_environment_;
        BinaryOp binary_op_;
        UnaryOp unary_op_;
        Call call_;
        Aggregate aggregate_;
        GetAggregateMember get_aggregate_member_;
        MethodCall method_call_;
        MakeEnvironment make_environment_;
        MakeClosure make_closure_;
        MakeIterator make_iterator_;
        Record record_;
        Container container_;
        Format format_;
        Error error_;
        Nop nop_;
    };
};
// [[[end]]]

/// True if the instruction defines a new phi node.
bool is_phi_define(const Function& func, InstId inst);

/// True if the instruction defines a new observe assign node.
bool is_observe_assign(const Function& func, InstId inst);

/* [[[cog
    from cog import outl
    from codegen.unions import implement_inlines
    from codegen.ir import LValue, Constant, Aggregate, Value
    types = [LValue, Constant, Aggregate, Value]
    for index, type in enumerate(types):
        if index != 0:
            outl()
        implement_inlines(type)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto) LValue::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case LValueType::Param:
        return vis.visit_param(self.param_, std::forward<Args>(args)...);
    case LValueType::Closure:
        return vis.visit_closure(self.closure_, std::forward<Args>(args)...);
    case LValueType::Module:
        return vis.visit_module(self.module_, std::forward<Args>(args)...);
    case LValueType::Field:
        return vis.visit_field(self.field_, std::forward<Args>(args)...);
    case LValueType::TupleField:
        return vis.visit_tuple_field(self.tuple_field_, std::forward<Args>(args)...);
    case LValueType::Index:
        return vis.visit_index(self.index_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid LValue type.");
}

template<typename Self, typename Visitor, typename... Args>
decltype(auto) Constant::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case ConstantType::Integer:
        return vis.visit_integer(self.integer_, std::forward<Args>(args)...);
    case ConstantType::Float:
        return vis.visit_float(self.float_, std::forward<Args>(args)...);
    case ConstantType::String:
        return vis.visit_string(self.string_, std::forward<Args>(args)...);
    case ConstantType::Symbol:
        return vis.visit_symbol(self.symbol_, std::forward<Args>(args)...);
    case ConstantType::Null:
        return vis.visit_null(self.null_, std::forward<Args>(args)...);
    case ConstantType::True:
        return vis.visit_true(self.true_, std::forward<Args>(args)...);
    case ConstantType::False:
        return vis.visit_false(self.false_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid Constant type.");
}

template<typename Self, typename Visitor, typename... Args>
decltype(auto) Aggregate::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case AggregateType::Method:
        return vis.visit_method(self.method_, std::forward<Args>(args)...);
    case AggregateType::IteratorNext:
        return vis.visit_iterator_next(self.iterator_next_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid Aggregate type.");
}

template<typename Self, typename Visitor, typename... Args>
decltype(auto) Value::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case ValueType::Read:
        return vis.visit_read(self.read_, std::forward<Args>(args)...);
    case ValueType::Write:
        return vis.visit_write(self.write_, std::forward<Args>(args)...);
    case ValueType::Alias:
        return vis.visit_alias(self.alias_, std::forward<Args>(args)...);
    case ValueType::Phi:
        return vis.visit_phi(self.phi_, std::forward<Args>(args)...);
    case ValueType::ObserveAssign:
        return vis.visit_observe_assign(self.observe_assign_, std::forward<Args>(args)...);
    case ValueType::PublishAssign:
        return vis.visit_publish_assign(self.publish_assign_, std::forward<Args>(args)...);
    case ValueType::Constant:
        return vis.visit_constant(self.constant_, std::forward<Args>(args)...);
    case ValueType::OuterEnvironment:
        return vis.visit_outer_environment(self.outer_environment_, std::forward<Args>(args)...);
    case ValueType::BinaryOp:
        return vis.visit_binary_op(self.binary_op_, std::forward<Args>(args)...);
    case ValueType::UnaryOp:
        return vis.visit_unary_op(self.unary_op_, std::forward<Args>(args)...);
    case ValueType::Call:
        return vis.visit_call(self.call_, std::forward<Args>(args)...);
    case ValueType::Aggregate:
        return vis.visit_aggregate(self.aggregate_, std::forward<Args>(args)...);
    case ValueType::GetAggregateMember:
        return vis.visit_get_aggregate_member(
            self.get_aggregate_member_, std::forward<Args>(args)...);
    case ValueType::MethodCall:
        return vis.visit_method_call(self.method_call_, std::forward<Args>(args)...);
    case ValueType::MakeEnvironment:
        return vis.visit_make_environment(self.make_environment_, std::forward<Args>(args)...);
    case ValueType::MakeClosure:
        return vis.visit_make_closure(self.make_closure_, std::forward<Args>(args)...);
    case ValueType::MakeIterator:
        return vis.visit_make_iterator(self.make_iterator_, std::forward<Args>(args)...);
    case ValueType::Record:
        return vis.visit_record(self.record_, std::forward<Args>(args)...);
    case ValueType::Container:
        return vis.visit_container(self.container_, std::forward<Args>(args)...);
    case ValueType::Format:
        return vis.visit_format(self.format_, std::forward<Args>(args)...);
    case ValueType::Error:
        return vis.visit_error(self.error_, std::forward<Args>(args)...);
    case ValueType::Nop:
        return vis.visit_nop(self.nop_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid Value type.");
}
// [[[end]]]

} // namespace tiro::ir

TIRO_ENABLE_MEMBER_HASH(tiro::ir::FloatConstant)
TIRO_ENABLE_MEMBER_HASH(tiro::ir::Constant)

TIRO_ENABLE_FREE_TO_STRING(tiro::ir::LValueType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::LValue)
TIRO_ENABLE_FREE_TO_STRING(tiro::ir::ConstantType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::FloatConstant)
TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::Constant)
TIRO_ENABLE_FREE_TO_STRING(tiro::ir::AggregateType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::Aggregate)
TIRO_ENABLE_FREE_TO_STRING(tiro::ir::AggregateMember)
TIRO_ENABLE_FREE_TO_STRING(tiro::ir::ValueType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::Value)
TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::Phi)
TIRO_ENABLE_FREE_TO_STRING(tiro::ir::BinaryOpType)
TIRO_ENABLE_FREE_TO_STRING(tiro::ir::UnaryOpType)
TIRO_ENABLE_FREE_TO_STRING(tiro::ir::ContainerType)

#endif // TIRO_COMPILER_IR_VALUE_HPP
