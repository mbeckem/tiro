#ifndef TIRO_COMPILER_IR_FUNCTION_HPP
#define TIRO_COMPILER_IR_FUNCTION_HPP

#include "common/adt/function_ref.hpp"
#include "common/adt/span.hpp"
#include "common/defs.hpp"
#include "common/format.hpp"
#include "common/hash.hpp"
#include "common/id_type.hpp"
#include "common/index_map.hpp"
#include "common/iter_tools.hpp"
#include "common/not_null.hpp"
#include "common/string_table.hpp"
#include "common/vec_ptr.hpp"
#include "compiler/ir/fwd.hpp"
#include "compiler/ir/id.hpp"

#include <absl/container/inlined_vector.h>

namespace tiro {

enum class FunctionType : u8 {
    /// Function is a plain function and can be called and exported as-is.
    Normal,

    /// Function requires a closure environment to be called.
    Closure
};

std::string_view to_string(FunctionType type);

class Function final {
public:
    explicit Function(InternedString name, FunctionType type, StringTable& strings);

    ~Function();

    Function(Function&&) noexcept = default;
    Function& operator=(Function&&) noexcept = default;

    Function(const Function&) = delete;
    Function& operator=(const Function&) = delete;

    InternedString name() const noexcept { return name_; }
    FunctionType type() const noexcept { return type_; }

    StringTable& strings() const noexcept { return *strings_; }

    BlockId make(Block&& block);
    ParamId make(const Param& param);
    LocalId make(const Local& local);
    PhiId make(Phi&& phi);
    LocalListId make(LocalList&& rvalue_list);
    RecordId make(Record&& record);

    BlockId entry() const;
    BlockId exit() const;

    size_t block_count() const { return blocks_.size(); }
    size_t param_count() const { return params_.size(); }
    size_t local_count() const { return locals_.size(); }
    size_t phi_count() const { return phis_.size(); }
    size_t local_list_count() const { return local_lists_.size(); }

    NotNull<VecPtr<Block>> operator[](BlockId id);
    NotNull<VecPtr<Param>> operator[](ParamId id);
    NotNull<VecPtr<Local>> operator[](LocalId id);
    NotNull<VecPtr<Phi>> operator[](PhiId id);
    NotNull<VecPtr<LocalList>> operator[](LocalListId id);
    NotNull<VecPtr<Record>> operator[](RecordId id);

    NotNull<VecPtr<const Block>> operator[](BlockId id) const;
    NotNull<VecPtr<const Param>> operator[](ParamId id) const;
    NotNull<VecPtr<const Local>> operator[](LocalId id) const;
    NotNull<VecPtr<const Phi>> operator[](PhiId id) const;
    NotNull<VecPtr<const LocalList>> operator[](LocalListId id) const;
    NotNull<VecPtr<const Record>> operator[](RecordId id) const;

    auto block_ids() const { return blocks_.keys(); }

    auto blocks() const { return range_view(blocks_); }
    auto locals() const { return range_view(locals_); }
    auto phis() const { return range_view(phis_); }

private:
    NotNull<StringTable*> strings_;

    InternedString name_;
    FunctionType type_;

    // Improvement: Can make these allocate from an arena instead
    IndexMap<Block, IdMapper<BlockId>> blocks_;
    IndexMap<Param, IdMapper<ParamId>> params_;
    IndexMap<Local, IdMapper<LocalId>> locals_;
    IndexMap<Phi, IdMapper<PhiId>> phis_;
    IndexMap<LocalList, IdMapper<LocalListId>> local_lists_;
    IndexMap<Record, IdMapper<RecordId>> records_;

    BlockId entry_;
    BlockId exit_;
};

void dump_function(const Function& func, FormatStream& stream);

/// Represents a parameter to the function. Parameters appear in the same order
/// as in the source code.
class Param final {
public:
    // TODO source location

    explicit Param(InternedString name);

    InternedString name() const;

    void format(FormatStream& stream) const;

private:
    InternedString name_;
};

/* [[[cog
    from codegen.unions import define
    from codegen.ir import Terminator
    define(Terminator.tag)
]]] */
enum class TerminatorType : u8 {
    None,
    Jump,
    Branch,
    Return,
    Exit,
    AssertFail,
    Never,
};

std::string_view to_string(TerminatorType type);
// [[[end]]]

/// Represents the type of a conditional jump.
enum class BranchType : u8 {
    IfTrue,
    IfFalse,
    IfNull,
    IfNotNull,
};

std::string_view to_string(BranchType type);

/* [[[cog
    from codegen.unions import define
    from codegen.ir import Terminator
    define(Terminator)
]]] */
/// Represents edges connecting different basic blocks.
class Terminator final {
public:
    /// The block has no outgoing edge. This is the initial value after a new block has been created.
    /// must be changed to one of the valid edge types when construction is complete.
    struct None final {};

    /// A single successor block, reached through an unconditional jump.
    struct Jump final {
        /// The jump target.
        BlockId target;

        explicit Jump(const BlockId& target_)
            : target(target_) {}
    };

    /// A conditional jump with two successor blocks.
    struct Branch final {
        /// The kind of conditional jump.
        BranchType type;

        /// The value that is being tested.
        LocalId value;

        /// The jump target for successful tests.
        BlockId target;

        /// The jump target for failed tests.
        BlockId fallthrough;

        Branch(const BranchType& type_, const LocalId& value_, const BlockId& target_,
            const BlockId& fallthrough_)
            : type(type_)
            , value(value_)
            , target(target_)
            , fallthrough(fallthrough_) {}
    };

    /// Returns a value from the function.
    struct Return final {
        /// The value that is being returned.
        LocalId value;

        /// The successor block. This must be the exit block.
        /// These edges are needed to make all code paths converge at the exit block.
        BlockId target;

        Return(const LocalId& value_, const BlockId& target_)
            : value(value_)
            , target(target_) {}
    };

    /// Marks the exit block of the function.
    struct Exit final {};

    /// An assertion failure is an unconditional hard exit.
    struct AssertFail final {
        /// The string representation of the failed assertion.
        LocalId expr;

        /// The message that will be printed when the assertion fails.
        LocalId message;

        /// The successor block. This must be the exit block.
        /// These edges are needed to make all code paths converge at the exit block.
        BlockId target;

        AssertFail(const LocalId& expr_, const LocalId& message_, const BlockId& target_)
            : expr(expr_)
            , message(message_)
            , target(target_) {}
    };

    /// The block never returns (e.g. contains a statement that never terminates).
    struct Never final {
        /// The successor block. This must be the exit block.
        /// These edges are needed to make all code paths converge at the exit block.
        BlockId target;

        explicit Never(const BlockId& target_)
            : target(target_) {}
    };

    static Terminator make_none();
    static Terminator make_jump(const BlockId& target);
    static Terminator make_branch(const BranchType& type, const LocalId& value,
        const BlockId& target, const BlockId& fallthrough);
    static Terminator make_return(const LocalId& value, const BlockId& target);
    static Terminator make_exit();
    static Terminator
    make_assert_fail(const LocalId& expr, const LocalId& message, const BlockId& target);
    static Terminator make_never(const BlockId& target);

    Terminator(None none);
    Terminator(Jump jump);
    Terminator(Branch branch);
    Terminator(Return ret);
    Terminator(Exit exit);
    Terminator(AssertFail assert_fail);
    Terminator(Never never);

    TerminatorType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const None& as_none() const;
    const Jump& as_jump() const;
    const Branch& as_branch() const;
    const Return& as_return() const;
    const Exit& as_exit() const;
    const AssertFail& as_assert_fail() const;
    const Never& as_never() const;

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
    TerminatorType type_;
    union {
        None none_;
        Jump jump_;
        Branch branch_;
        Return return_;
        Exit exit_;
        AssertFail assert_fail_;
        Never never_;
    };
};
// [[[end]]]

/// Invokes the callback for every block reachable via the given terminator.
// TODO This should be an iterator.
void visit_targets(const Terminator& term, FunctionRef<void(BlockId)> callback);

/// Returns the number of targets of the given terminator.
u32 target_count(const Terminator& term);

/// Represents a single basic block in the control flow graph of a function.
///
/// A block contains a simple sequence of statements. The list of statements
/// does not contain inner control flow: if the basic block is entered, its complete
/// sequence of statements will be executed.
///
/// Blocks are connected by incoming and outgoing edges. These model the control flow,
/// including branches, jumps and returns.
///
/// The initial "entry" block of a functions does not have any incoming edges,
/// and only the final "exit" block has an outgoing return edge.
class Block final {
public:
    explicit Block(InternedString label);
    ~Block();

    Block(Block&&) noexcept = default;
    Block& operator=(Block&&) noexcept = default;

    // Prevent accidental copying.
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;

    InternedString label() const { return label_; }

    /// A sealed block no longer accepts incoming edges.
    void sealed(bool is_sealed) { sealed_ = is_sealed; }
    bool sealed() const { return sealed_; }

    /// A filled block no longer accepts additional statements.
    void filled(bool is_filled) { filled_ = is_filled; }
    bool filled() const { return filled_; }

    const Terminator& terminator() const { return term_; }
    void terminator(const Terminator& term) { term_ = term; }

    auto predecessors() const { return range_view(predecessors_); }

    BlockId predecessor(size_t index) const;
    size_t predecessor_count() const;
    void append_predecessor(BlockId predecessor);
    void replace_predecessor(BlockId old_pred, BlockId new_pred);

    auto stmts() const { return range_view(stmts_); }

    const Stmt& stmt(size_t index) const;
    size_t stmt_count() const;

    void insert_stmt(size_t index, const Stmt& stmt);
    void insert_stmts(size_t index, Span<const Stmt> stmts);
    void append_stmt(const Stmt& stmt);

    /// Returns the number of phi nodes at the beginning of the block.
    size_t phi_count(const Function& parent) const;

    /// Called to transform a phi function into a normal value.
    /// This function will apply the new value and move the definition after the other phi functions
    /// to ensure that phis remain clustered at the start of the block.
    void remove_phi(Function& parent, LocalId local_id, const RValue& new_value);

    auto& raw_stmts() { return stmts_; }

    /// Removes all statements from this block for which the given predicate
    /// returns true.
    template<typename Pred>
    void remove_stmts(Pred&& pred) {
        auto rem = std::remove_if(stmts_.begin(), stmts_.end(), pred);
        stmts_.erase(rem, stmts_.end());
    }

    void format(FormatStream& stream) const;

private:
    InternedString label_;
    bool sealed_ = false;
    bool filled_ = false;
    Terminator term_ = Terminator::None{};
    std::vector<BlockId> predecessors_;
    std::vector<Stmt> stmts_;
};

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
        LocalId env;

        /// Levels to "go up" the environment hierarchy. 0 is the closure environment itself.
        u32 levels;

        /// Index into the environment.
        u32 index;

        Closure(const LocalId& env_, const u32& levels_, const u32& index_)
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
        LocalId object;

        /// Field name to access.
        InternedString name;

        Field(const LocalId& object_, const InternedString& name_)
            : object(object_)
            , name(name_) {}
    };

    /// Referencce to a tuple field of a tuple (i.e. `tuple.3`).
    struct TupleField final {
        /// Dereferenced tuple object.
        LocalId object;

        /// Index of the tuple member.
        u32 index;

        TupleField(const LocalId& object_, const u32& index_)
            : object(object_)
            , index(index_) {}
    };

    /// Reference to an index of an array (or a map), i.e. `thing[foo]`.
    struct Index final {
        /// Dereferenced arraylike object.
        LocalId object;

        /// Index into the array.
        LocalId index;

        Index(const LocalId& object_, const LocalId& index_)
            : object(object_)
            , index(index_) {}
    };

    static LValue make_param(const ParamId& target);
    static LValue make_closure(const LocalId& env, const u32& levels, const u32& index);
    static LValue make_module(const ModuleMemberId& member);
    static LValue make_field(const LocalId& object, const InternedString& name);
    static LValue make_tuple_field(const LocalId& object, const u32& index);
    static LValue make_index(const LocalId& object, const LocalId& index);

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
struct FloatConstant {
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
        LocalId instance;
        InternedString function;

        Method(const LocalId& instance_, const InternedString& function_)
            : instance(instance_)
            , function(function_) {}
    };

    /// Represents the result of advancing an iterator (returns valid, value).
    struct IteratorNext final {
        LocalId iterator;

        explicit IteratorNext(const LocalId& iterator_)
            : iterator(iterator_) {}
    };

    static Aggregate make_method(const LocalId& instance, const InternedString& function);
    static Aggregate make_iterator_next(const LocalId& iterator);

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
    from codegen.ir import RValue
    define(RValue.tag)
]]] */
enum class RValueType : u8 {
    UseLValue,
    UseLocal,
    Phi,
    Phi0,
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
};

std::string_view to_string(RValueType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ir import RValue
    define(RValue)
]]] */
/// Represents an rvalue.
/// RValues can be used as the right hand side of an assignment or definition.
///
/// RValues at this compilation stage do not allow inner control flow. Nested
/// language-level expressions that contain loops or conditionals are split up
/// so that only "simple" expressions remain.
class RValue final {
public:
    /// References an lvalue to produce a value.
    struct UseLValue final {
        /// Dereferenced lvalue.
        LValue target;

        explicit UseLValue(const LValue& target_)
            : target(target_) {}
    };

    /// References a local variable.
    struct UseLocal final {
        /// Dereferenced local.
        LocalId target;

        explicit UseLocal(const LocalId& target_)
            : target(target_) {}
    };

    /// Phi nodes can have one of multiple locals as their value, depending on the code path that led to them.
    struct Phi final {
        /// The possible alternatives.
        PhiId value;

        explicit Phi(const PhiId& value_)
            : value(value_) {}
    };

    /// Marker to store the fact that this local has been visited during variable resolution (used to stop recursion).
    struct Phi0 final {};

    /// A constant.
    using Constant = tiro::Constant;

    /// Deferences the function's outer closure environment
    struct OuterEnvironment final {};

    /// Simple binary operation.
    struct BinaryOp final {
        BinaryOpType op;

        /// Left operand.
        LocalId left;

        /// Right operand.
        LocalId right;

        BinaryOp(const BinaryOpType& op_, const LocalId& left_, const LocalId& right_)
            : op(op_)
            , left(left_)
            , right(right_) {}
    };

    /// Simple unary operation.
    struct UnaryOp final {
        UnaryOpType op;
        LocalId operand;

        UnaryOp(const UnaryOpType& op_, const LocalId& operand_)
            : op(op_)
            , operand(operand_) {}
    };

    /// Function call expression, i.e. `f(a, b, c)`.
    struct Call final {
        /// Function to call.
        LocalId func;

        /// The list of function arguments.
        LocalListId args;

        Call(const LocalId& func_, const LocalListId& args_)
            : func(func_)
            , args(args_) {}
    };

    /// Represents an aggregate value.
    using Aggregate = tiro::Aggregate;

    /// Fetches a member value from an aggregate.
    struct GetAggregateMember final {
        /// Must be an aggregate value of the correct type.
        LocalId aggregate;

        /// The aggregate member returned from the aggregate.
        AggregateMember member;

        GetAggregateMember(const LocalId& aggregate_, const AggregateMember& member_)
            : aggregate(aggregate_)
            , member(member_) {}
    };

    /// Method call expression, i.e `a.b(c, d)`.
    struct MethodCall final {
        /// Method to be called. Must be a method value.
        LocalId method;

        /// List of method arguments.
        LocalListId args;

        MethodCall(const LocalId& method_, const LocalListId& args_)
            : method(method_)
            , args(args_) {}
    };

    /// Creates a new closure environment.
    struct MakeEnvironment final {
        /// The parent environment.
        LocalId parent;

        /// The number of variable slots in the new environment.
        u32 size;

        MakeEnvironment(const LocalId& parent_, const u32& size_)
            : parent(parent_)
            , size(size_) {}
    };

    /// Creates a new closure function.
    struct MakeClosure final {
        /// The closure environment.
        LocalId env;

        /// The closure function's template location.
        LocalId func;

        MakeClosure(const LocalId& env_, const LocalId& func_)
            : env(env_)
            , func(func_) {}
    };

    /// Creates a new iterator for a given container instance.
    struct MakeIterator final {
        /// The container being iterated.
        LocalId container;

        explicit MakeIterator(const LocalId& container_)
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

    static RValue make_use_lvalue(const LValue& target);
    static RValue make_use_local(const LocalId& target);
    static RValue make_phi(const PhiId& value);
    static RValue make_phi0();
    static RValue make_constant(const Constant& constant);
    static RValue make_outer_environment();
    static RValue make_binary_op(const BinaryOpType& op, const LocalId& left, const LocalId& right);
    static RValue make_unary_op(const UnaryOpType& op, const LocalId& operand);
    static RValue make_call(const LocalId& func, const LocalListId& args);
    static RValue make_aggregate(const Aggregate& aggregate);
    static RValue
    make_get_aggregate_member(const LocalId& aggregate, const AggregateMember& member);
    static RValue make_method_call(const LocalId& method, const LocalListId& args);
    static RValue make_make_environment(const LocalId& parent, const u32& size);
    static RValue make_make_closure(const LocalId& env, const LocalId& func);
    static RValue make_make_iterator(const LocalId& container);
    static RValue make_record(const RecordId& value);
    static RValue make_container(const ContainerType& container, const LocalListId& args);
    static RValue make_format(const LocalListId& args);
    static RValue make_error();

    RValue(UseLValue use_lvalue);
    RValue(UseLocal use_local);
    RValue(Phi phi);
    RValue(Phi0 phi0);
    RValue(Constant constant);
    RValue(OuterEnvironment outer_environment);
    RValue(BinaryOp binary_op);
    RValue(UnaryOp unary_op);
    RValue(Call call);
    RValue(Aggregate aggregate);
    RValue(GetAggregateMember get_aggregate_member);
    RValue(MethodCall method_call);
    RValue(MakeEnvironment make_environment);
    RValue(MakeClosure make_closure);
    RValue(MakeIterator make_iterator);
    RValue(Record record);
    RValue(Container container);
    RValue(Format format);
    RValue(Error error);

    RValueType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const UseLValue& as_use_lvalue() const;
    const UseLocal& as_use_local() const;
    const Phi& as_phi() const;
    const Phi0& as_phi0() const;
    const Constant& as_constant() const;
    const OuterEnvironment& as_outer_environment() const;
    const BinaryOp& as_binary_op() const;
    const UnaryOp& as_unary_op() const;
    const Call& as_call() const;
    const Aggregate& as_aggregate() const;
    const GetAggregateMember& as_get_aggregate_member() const;
    const MethodCall& as_method_call() const;
    const MakeEnvironment& as_make_environment() const;
    const MakeClosure& as_make_closure() const;
    const MakeIterator& as_make_iterator() const;
    const Record& as_record() const;
    const Container& as_container() const;
    const Format& as_format() const;
    const Error& as_error() const;

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
    RValueType type_;
    union {
        UseLValue use_lvalue_;
        UseLocal use_local_;
        Phi phi_;
        Phi0 phi0_;
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
    };
};
// [[[end]]]

/// Represents a local variable (user defined or temporary).
/// Locals use SSA (Static Single Assignment) form: they are defined exactly once.
class Local final {
public:
    // TODO source ranges

    Local(const RValue& value);

    /// Only declared variables have a valid name.
    void name(InternedString name) { name_ = name; }
    InternedString name() const noexcept { return name_; }

    /// The rvalue bound to this local.
    const RValue& value() const noexcept { return value_; }
    void value(const RValue& value) { value_ = value; }

    void format(FormatStream& stream) const;

private:
    InternedString name_;
    RValue value_;
};

/// Represents a phi node. Phi nodes are used at control flow join points to record that
/// fact that an SSA value may have one of multiple possible values, depending on the code path
/// used to reach the value.
class Phi final {
public:
    using Storage = absl::InlinedVector<LocalId, 6>;

    Phi();
    Phi(std::initializer_list<LocalId> operands);
    Phi(Storage&& operands);
    ~Phi();

    Phi(Phi&&) noexcept = default;
    Phi& operator=(Phi&&) = default;

    // Prevent accidental copying
    Phi(const Phi&) = delete;
    Phi& operator=(const Phi&) = delete;

    void append_operand(LocalId operand);

    void format(FormatStream& stream) const;

    auto operands() const { return IterRange(operands_.begin(), operands_.end()); }

    LocalId operand(size_t index) const;

    void operand(size_t index, LocalId local);

    size_t operand_count() const { return operands_.size(); }

private:
    // TODO: Track phi node users
    Storage operands_;
};

/// Represents a list of local variables, e.g. the arguments to a function call
/// or the items of an array.
class LocalList final {
public:
    using Storage = absl::InlinedVector<LocalId, 8>;

    LocalList();
    LocalList(std::initializer_list<LocalId> rvalues);
    LocalList(Storage&& locals);
    ~LocalList();

    LocalList(LocalList&&) noexcept = default;
    LocalList& operator=(LocalList&&) = default;

    // Prevent accidental copying.
    LocalList(const LocalList&) = delete;
    LocalList& operator=(const LocalList&) = delete;

    auto begin() const { return locals_.begin(); }
    auto end() const { return locals_.end(); }

    size_t size() const { return locals_.size(); }
    bool empty() const { return locals_.empty(); }

    LocalId operator[](size_t index) const {
        TIRO_DEBUG_ASSERT(index < locals_.size(), "Index out of bounds.");
        return locals_[index];
    }

    LocalId get(size_t index) const { return (*this)[index]; }

    void set(size_t index, LocalId value) {
        TIRO_DEBUG_ASSERT(index < locals_.size(), "Index out of bounds.");
        locals_[index] = value;
    }

    void remove(size_t index, size_t count) {
        TIRO_DEBUG_ASSERT(
            index <= locals_.size() && count <= locals_.size() - index, "Range out of bounds.");
        const auto pos = locals_.begin() + index;
        locals_.erase(pos, pos + count);
    }

    void append(LocalId local) { locals_.push_back(local); }

private:
    Storage locals_;
};

/// A record represents an object that maps keys (symbols) to values.
/// In this instance, the keys are known at compile time.
/// Record entries are currently represented as a simple vector because
/// because the semantic analysis pass already checks that keys are unique.
class Record final {
public:
    using Entry = std::pair<InternedString, LocalId>;

    Record();

    Record(Record&&) noexcept = default;
    Record& operator=(Record&&) = default;

    Record(const Record&) = delete;
    Record& operator=(const Record&) = delete;

    auto begin() const { return entries_.begin(); }
    auto end() const { return entries_.end(); }

    size_t size() const { return entries_.size(); }

    void insert(InternedString name, LocalId value);

private:
    absl::InlinedVector<Entry, 8> entries_;
};

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
    from codegen.ir import Stmt
    define(Stmt.tag)
]]] */
enum class StmtType : u8 {
    Assign,
    Define,
};

std::string_view to_string(StmtType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ir import Stmt
    define(Stmt)
]]] */
/// Represents a statement, i.e. a single instruction inside a basic block.
class Stmt final {
public:
    /// Assigns a value to a memory location (non-SSA operations).
    struct Assign final {
        /// The assignment target.
        LValue target;

        /// The new value.
        LocalId value;

        Assign(const LValue& target_, const LocalId& value_)
            : target(target_)
            , value(value_) {}
    };

    /// Defines a new local variable (SSA).
    struct Define final {
        LocalId local;

        explicit Define(const LocalId& local_)
            : local(local_) {}
    };

    static Stmt make_assign(const LValue& target, const LocalId& value);
    static Stmt make_define(const LocalId& local);

    Stmt(Assign assign);
    Stmt(Define define);

    StmtType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Assign& as_assign() const;
    const Define& as_define() const;

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
    StmtType type_;
    union {
        Assign assign_;
        Define define_;
    };
};
// [[[end]]]

/// True if the statement defines a new phi node.
bool is_phi_define(const Function& func, const Stmt& stmt);

/* [[[cog
    from cog import outl
    from codegen.unions import implement_inlines
    from codegen.ir import Terminator, LValue, Constant, Aggregate, RValue, Stmt
    types = [Terminator, LValue, Constant, Aggregate, RValue, Stmt]
    for index, type in enumerate(types):
        if index != 0:
            outl()
        implement_inlines(type)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto) Terminator::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case TerminatorType::None:
        return vis.visit_none(self.none_, std::forward<Args>(args)...);
    case TerminatorType::Jump:
        return vis.visit_jump(self.jump_, std::forward<Args>(args)...);
    case TerminatorType::Branch:
        return vis.visit_branch(self.branch_, std::forward<Args>(args)...);
    case TerminatorType::Return:
        return vis.visit_return(self.return_, std::forward<Args>(args)...);
    case TerminatorType::Exit:
        return vis.visit_exit(self.exit_, std::forward<Args>(args)...);
    case TerminatorType::AssertFail:
        return vis.visit_assert_fail(self.assert_fail_, std::forward<Args>(args)...);
    case TerminatorType::Never:
        return vis.visit_never(self.never_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid Terminator type.");
}

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
decltype(auto) RValue::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case RValueType::UseLValue:
        return vis.visit_use_lvalue(self.use_lvalue_, std::forward<Args>(args)...);
    case RValueType::UseLocal:
        return vis.visit_use_local(self.use_local_, std::forward<Args>(args)...);
    case RValueType::Phi:
        return vis.visit_phi(self.phi_, std::forward<Args>(args)...);
    case RValueType::Phi0:
        return vis.visit_phi0(self.phi0_, std::forward<Args>(args)...);
    case RValueType::Constant:
        return vis.visit_constant(self.constant_, std::forward<Args>(args)...);
    case RValueType::OuterEnvironment:
        return vis.visit_outer_environment(self.outer_environment_, std::forward<Args>(args)...);
    case RValueType::BinaryOp:
        return vis.visit_binary_op(self.binary_op_, std::forward<Args>(args)...);
    case RValueType::UnaryOp:
        return vis.visit_unary_op(self.unary_op_, std::forward<Args>(args)...);
    case RValueType::Call:
        return vis.visit_call(self.call_, std::forward<Args>(args)...);
    case RValueType::Aggregate:
        return vis.visit_aggregate(self.aggregate_, std::forward<Args>(args)...);
    case RValueType::GetAggregateMember:
        return vis.visit_get_aggregate_member(
            self.get_aggregate_member_, std::forward<Args>(args)...);
    case RValueType::MethodCall:
        return vis.visit_method_call(self.method_call_, std::forward<Args>(args)...);
    case RValueType::MakeEnvironment:
        return vis.visit_make_environment(self.make_environment_, std::forward<Args>(args)...);
    case RValueType::MakeClosure:
        return vis.visit_make_closure(self.make_closure_, std::forward<Args>(args)...);
    case RValueType::MakeIterator:
        return vis.visit_make_iterator(self.make_iterator_, std::forward<Args>(args)...);
    case RValueType::Record:
        return vis.visit_record(self.record_, std::forward<Args>(args)...);
    case RValueType::Container:
        return vis.visit_container(self.container_, std::forward<Args>(args)...);
    case RValueType::Format:
        return vis.visit_format(self.format_, std::forward<Args>(args)...);
    case RValueType::Error:
        return vis.visit_error(self.error_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid RValue type.");
}

template<typename Self, typename Visitor, typename... Args>
decltype(auto) Stmt::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case StmtType::Assign:
        return vis.visit_assign(self.assign_, std::forward<Args>(args)...);
    case StmtType::Define:
        return vis.visit_define(self.define_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid Stmt type.");
}
// [[[end]]]

namespace dump_helpers {

template<typename T>
struct Dump {
    const Function& parent;
    T value;
};

template<typename T>
Dump<T> dump(const Function& parent, const T& value) {
    return Dump<T>{parent, value};
}

void format(const Dump<BlockId>& d, FormatStream& stream);
void format(const Dump<Terminator>& d, FormatStream& stream);
void format(const Dump<LValue>& d, FormatStream& stream);
void format(const Dump<Constant>& d, FormatStream& stream);
void format(const Dump<Aggregate>& d, FormatStream& stream);
void format(const Dump<RValue>& d, FormatStream& stream);
void format(const Dump<LocalId>& d, FormatStream& stream);
void format(const Dump<PhiId>& d, FormatStream& stream);
void format(const Dump<LocalListId>& d, FormatStream& stream);
void format(const Dump<RecordId>& d, FormatStream& stream);
void format(const Dump<Stmt>& d, FormatStream& stream);

}; // namespace dump_helpers

} // namespace tiro

TIRO_ENABLE_MEMBER_HASH(tiro::FloatConstant)
TIRO_ENABLE_MEMBER_HASH(tiro::Constant)

TIRO_ENABLE_FREE_TO_STRING(tiro::FunctionType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::Block)
TIRO_ENABLE_MEMBER_FORMAT(tiro::Param)
TIRO_ENABLE_FREE_TO_STRING(tiro::LocalType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::Local)
TIRO_ENABLE_FREE_TO_STRING(tiro::TerminatorType)
TIRO_ENABLE_FREE_TO_STRING(tiro::BranchType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::Terminator)
TIRO_ENABLE_FREE_TO_STRING(tiro::LValueType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::LValue)
TIRO_ENABLE_FREE_TO_STRING(tiro::ConstantType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::FloatConstant)
TIRO_ENABLE_MEMBER_FORMAT(tiro::Constant)
TIRO_ENABLE_FREE_TO_STRING(tiro::AggregateType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::Aggregate)
TIRO_ENABLE_FREE_TO_STRING(tiro::AggregateMember)
TIRO_ENABLE_FREE_TO_STRING(tiro::RValueType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::RValue)
TIRO_ENABLE_MEMBER_FORMAT(tiro::Phi)
TIRO_ENABLE_FREE_TO_STRING(tiro::BinaryOpType)
TIRO_ENABLE_FREE_TO_STRING(tiro::UnaryOpType)
TIRO_ENABLE_FREE_TO_STRING(tiro::ContainerType)
TIRO_ENABLE_FREE_TO_STRING(tiro::StmtType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::Stmt)

TIRO_ENABLE_FREE_FORMAT(tiro::dump_helpers::Dump<tiro::BlockId>)
TIRO_ENABLE_FREE_FORMAT(tiro::dump_helpers::Dump<tiro::Terminator>)
TIRO_ENABLE_FREE_FORMAT(tiro::dump_helpers::Dump<tiro::LValue>)
TIRO_ENABLE_FREE_FORMAT(tiro::dump_helpers::Dump<tiro::Constant>)
TIRO_ENABLE_FREE_FORMAT(tiro::dump_helpers::Dump<tiro::Aggregate>)
TIRO_ENABLE_FREE_FORMAT(tiro::dump_helpers::Dump<tiro::RValue>)
TIRO_ENABLE_FREE_FORMAT(tiro::dump_helpers::Dump<tiro::LocalId>)
TIRO_ENABLE_FREE_FORMAT(tiro::dump_helpers::Dump<tiro::PhiId>)
TIRO_ENABLE_FREE_FORMAT(tiro::dump_helpers::Dump<tiro::LocalListId>)
TIRO_ENABLE_FREE_FORMAT(tiro::dump_helpers::Dump<tiro::RecordId>)
TIRO_ENABLE_FREE_FORMAT(tiro::dump_helpers::Dump<tiro::Stmt>)

#endif // TIRO_COMPILER_IR_FUNCTION_HPP
