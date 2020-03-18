#ifndef TIRO_MIR_TYPES_HPP
#define TIRO_MIR_TYPES_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/function_ref.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/core/id_type.hpp"
#include "tiro/core/index_map.hpp"
#include "tiro/core/iter_tools.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/core/span.hpp"
#include "tiro/core/string_table.hpp"
#include "tiro/core/vec_ptr.hpp"
#include "tiro/mir/fwd.hpp"

namespace tiro::mir {

TIRO_DEFINE_ID(BlockID, u32)
TIRO_DEFINE_ID(ParamID, u32)
TIRO_DEFINE_ID(ModuleMemberID, u32)
TIRO_DEFINE_ID(FunctionID, u32)
TIRO_DEFINE_ID(LocalID, u32)
TIRO_DEFINE_ID(PhiID, u32)
TIRO_DEFINE_ID(LocalListID, u32)

/// Represents a module that has been lowered to MIR.
class Module final {
public:
    explicit Module(InternedString name, StringTable& strings);
    ~Module();

    Module(Module&&) noexcept = default;
    Module& operator=(Module&&) noexcept = default;

    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

    StringTable& strings() const { return *strings_; }

    InternedString name() const { return name_; }

    ModuleMemberID make(const ModuleMember& member);
    FunctionID make(Function&& function);

    NotNull<VecPtr<ModuleMember>> operator[](ModuleMemberID id);
    NotNull<VecPtr<Function>> operator[](FunctionID id);

    NotNull<VecPtr<const ModuleMember>> operator[](ModuleMemberID id) const;
    NotNull<VecPtr<const Function>> operator[](FunctionID id) const;

    auto members() const { return range_view(members_); }
    auto functions() const { return range_view(functions_); }

    size_t member_count() const { return members_.size(); }
    size_t function_count() const { return functions_.size(); }

private:
    NotNull<StringTable*> strings_;

    InternedString name_;
    IndexMap<ModuleMember, IDMapper<ModuleMemberID>> members_;
    IndexMap<Function, IDMapper<FunctionID>> functions_;
};

void dump_module(const Module& module, FormatStream& stream);

/* [[[cog
    import unions
    import mir
    unions.define_type(mir.ModuleMemberType)
]]] */
enum class ModuleMemberType : u8 {
    Import,
    Variable,
    Function,
};

std::string_view to_string(ModuleMemberType type);
// [[[end]]]

/* [[[cog
    import unions
    import mir
    unions.define_type(mir.ModuleMember)
]]] */
/// Represents a member of a module.
class ModuleMember final {
public:
    /// Represents an import of another module.
    struct Import final {
        /// The name of the imported module.
        InternedString name;

        explicit Import(const InternedString& name_)
            : name(name_) {}
    };

    /// Represents a variable at module scope.
    struct Variable final {
        /// The name of the variable.
        InternedString name;

        explicit Variable(const InternedString& name_)
            : name(name_) {}
    };

    /// Represents a function of this module, in MIR form.
    struct Function final {
        /// The ID of the function within this module.
        FunctionID id;

        explicit Function(const FunctionID& id_)
            : id(id_) {}
    };

    static ModuleMember make_import(const InternedString& name);
    static ModuleMember make_variable(const InternedString& name);
    static ModuleMember make_function(const FunctionID& id);

    ModuleMember(const Import& import);
    ModuleMember(const Variable& variable);
    ModuleMember(const Function& function);

    ModuleMemberType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Import& as_import() const;
    const Variable& as_variable() const;
    const Function& as_function() const;

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

private:
    ModuleMemberType type_;
    union {
        Import import_;
        Variable variable_;
        Function function_;
    };
};
// [[[end]]]

enum class FunctionType : u8 {
    /// Function is a plain function and can be called and exported as-is.
    Normal,

    /// Function requires a closure environment to be called.
    Closure
};

std::string_view to_string(FunctionType type);

class Function final {
public:
    explicit Function(
        InternedString name, FunctionType type, StringTable& strings);

    ~Function();

    Function(Function&&) noexcept = default;
    Function& operator=(Function&&) noexcept = default;

    Function(const Function&) = delete;
    Function& operator=(const Function&) = delete;

    InternedString name() const noexcept { return name_; }
    FunctionType type() const noexcept { return type_; }

    StringTable& strings() const noexcept { return *strings_; }

    BlockID make(Block&& block);
    ParamID make(const Param& param);
    LocalID make(const Local& local);
    PhiID make(Phi&& phi);
    LocalListID make(LocalList&& rvalue_list);

    BlockID entry() const;
    BlockID exit() const;

    size_t block_count() const { return blocks_.size(); }
    size_t param_count() const { return params_.size(); }
    size_t local_count() const { return locals_.size(); }
    size_t phi_count() const { return phis_.size(); }
    size_t local_list_count() const { return local_lists_.size(); }

    NotNull<VecPtr<Block>> operator[](BlockID id);
    NotNull<VecPtr<Param>> operator[](ParamID id);
    NotNull<VecPtr<Local>> operator[](LocalID id);
    NotNull<VecPtr<Phi>> operator[](PhiID id);
    NotNull<VecPtr<LocalList>> operator[](LocalListID id);

    NotNull<VecPtr<const Block>> operator[](BlockID id) const;
    NotNull<VecPtr<const Param>> operator[](ParamID id) const;
    NotNull<VecPtr<const Local>> operator[](LocalID id) const;
    NotNull<VecPtr<const Phi>> operator[](PhiID id) const;
    NotNull<VecPtr<const LocalList>> operator[](LocalListID id) const;

    auto block_ids() const { return blocks_.keys(); }

    auto blocks() const { return range_view(blocks_); }
    auto locals() const { return range_view(locals_); }
    auto phis() const { return range_view(phis_); }

private:
    NotNull<StringTable*> strings_;

    InternedString name_;
    FunctionType type_;

    // Improvement: Can make these allocate from an arena instead
    IndexMap<Block, IDMapper<BlockID>> blocks_;
    IndexMap<Param, IDMapper<ParamID>> params_;
    IndexMap<Local, IDMapper<LocalID>> locals_;
    IndexMap<Phi, IDMapper<PhiID>> phis_;
    IndexMap<LocalList, IDMapper<LocalListID>> local_lists_;

    BlockID entry_;
    BlockID exit_;
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
    import unions
    import mir
    unions.define_type(mir.TerminatorType)
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
};

std::string_view to_string(BranchType type);

/* [[[cog
    import unions
    import mir
    unions.define_type(mir.Terminator)
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
        BlockID target;

        explicit Jump(const BlockID& target_)
            : target(target_) {}
    };

    /// A conditional jump with two successor blocks.
    struct Branch final {
        /// The kind of conditional jump.
        BranchType type;

        /// The value that is being tested.
        LocalID value;

        /// The jump target for successful tests.
        BlockID target;

        /// The jump target for failed tests.
        BlockID fallthrough;

        Branch(const BranchType& type_, const LocalID& value_,
            const BlockID& target_, const BlockID& fallthrough_)
            : type(type_)
            , value(value_)
            , target(target_)
            , fallthrough(fallthrough_) {}
    };

    /// Returns a value from the function.
    struct Return final {
        /// The value that is being returned.
        LocalID value;

        /// The successor block. This must be the exit block.
        /// These edges are needed to make all code paths converge at the exit block.
        BlockID target;

        Return(const LocalID& value_, const BlockID& target_)
            : value(value_)
            , target(target_) {}
    };

    /// Marks the exit block of the function.
    struct Exit final {};

    /// An assertion failure is an unconditional hard exit.
    struct AssertFail final {
        /// The string representation of the failed assertion.
        LocalID expr;

        /// The message that will be printed when the assertion fails.
        LocalID message;

        /// The successor block. This must be the exit block.
        /// These edges are needed to make all code paths converge at the exit block.
        BlockID target;

        AssertFail(const LocalID& expr_, const LocalID& message_,
            const BlockID& target_)
            : expr(expr_)
            , message(message_)
            , target(target_) {}
    };

    /// The block never returns (e.g. contains a statement that never terminates).
    struct Never final {
        /// The successor block. This must be the exit block.
        /// These edges are needed to make all code paths converge at the exit block.
        BlockID target;

        explicit Never(const BlockID& target_)
            : target(target_) {}
    };

    static Terminator make_none();
    static Terminator make_jump(const BlockID& target);
    static Terminator make_branch(const BranchType& type, const LocalID& value,
        const BlockID& target, const BlockID& fallthrough);
    static Terminator make_return(const LocalID& value, const BlockID& target);
    static Terminator make_exit();
    static Terminator make_assert_fail(
        const LocalID& expr, const LocalID& message, const BlockID& target);
    static Terminator make_never(const BlockID& target);

    Terminator(const None& none);
    Terminator(const Jump& jump);
    Terminator(const Branch& branch);
    Terminator(const Return& ret);
    Terminator(const Exit& exit);
    Terminator(const AssertFail& assert_fail);
    Terminator(const Never& never);

    TerminatorType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const None& as_none() const;
    const Jump& as_jump() const;
    const Branch& as_branch() const;
    const Return& as_return() const;
    const Exit& as_exit() const;
    const AssertFail& as_assert_fail() const;
    const Never& as_never() const;

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

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
void visit_targets(const Terminator& term, FunctionRef<void(BlockID)> callback);

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

    BlockID predecessor(size_t index) const;
    size_t predecessor_count() const;
    void append_predecessor(BlockID predecessor);
    void replace_predecessor(BlockID old_pred, BlockID new_pred);

    auto stmts() const { return range_view(stmts_); }
    size_t stmt_count() const;
    void insert_stmt(size_t index, const Stmt& stmt);
    void insert_stmts(size_t index, Span<const Stmt> stmts);
    void append_stmt(const Stmt& stmt);

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
    std::vector<BlockID> predecessors_;
    std::vector<Stmt> stmts_;
};

/* [[[cog
    import unions
    import mir
    unions.define_type(mir.LValueType)
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
    import unions
    import mir
    unions.define_type(mir.LValue)
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
        ParamID target;

        explicit Param(const ParamID& target_)
            : target(target_) {}
    };

    /// Reference to a variable captured from an outer scope.
    struct Closure final {
        /// The environment to search. Either a local variable or the function's outer environment.
        LocalID env;

        /// Levels to "go up" the environment hierarchy. 0 is the closure environment itself.
        u32 levels;

        /// Index into the environment.
        u32 index;

        Closure(const LocalID& env_, const u32& levels_, const u32& index_)
            : env(env_)
            , levels(levels_)
            , index(index_) {}
    };

    /// Reference to a variable at module scope.
    struct Module final {
        /// ID of the module level variable.
        ModuleMemberID member;

        explicit Module(const ModuleMemberID& member_)
            : member(member_) {}
    };

    /// Reference to the field of an object (i.e. `object.foo`).
    struct Field final {
        /// Dereferenced object.
        LocalID object;

        /// Field name to access.
        InternedString name;

        Field(const LocalID& object_, const InternedString& name_)
            : object(object_)
            , name(name_) {}
    };

    /// Referencce to a tuple field of a tuple (i.e. `tuple.3`).
    struct TupleField final {
        /// Dereferenced tuple object.
        LocalID object;

        /// Index of the tuple member.
        u32 index;

        TupleField(const LocalID& object_, const u32& index_)
            : object(object_)
            , index(index_) {}
    };

    /// Reference to an index of an array (or a map), i.e. `thing[foo]`.
    struct Index final {
        /// Dereferenced arraylike object.
        LocalID object;

        /// Index into the array.
        LocalID index;

        Index(const LocalID& object_, const LocalID& index_)
            : object(object_)
            , index(index_) {}
    };

    static LValue make_param(const ParamID& target);
    static LValue
    make_closure(const LocalID& env, const u32& levels, const u32& index);
    static LValue make_module(const ModuleMemberID& member);
    static LValue make_field(const LocalID& object, const InternedString& name);
    static LValue make_tuple_field(const LocalID& object, const u32& index);
    static LValue make_index(const LocalID& object, const LocalID& index);

    LValue(const Param& param);
    LValue(const Closure& closure);
    LValue(const Module& module);
    LValue(const Field& field);
    LValue(const TupleField& tuple_field);
    LValue(const Index& index);

    LValueType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Param& as_param() const;
    const Closure& as_closure() const;
    const Module& as_module() const;
    const Field& as_field() const;
    const TupleField& as_tuple_field() const;
    const Index& as_index() const;

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

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
    import unions
    import mir
    unions.define_type(mir.ConstantType)
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
    void build_hash(Hasher& h) const;
};

bool operator==(const FloatConstant& lhs, const FloatConstant& rhs);
bool operator!=(const FloatConstant& lhs, const FloatConstant& rhs);
bool operator<(const FloatConstant& lhs, const FloatConstant& rhs);
bool operator>(const FloatConstant& lhs, const FloatConstant& rhs);
bool operator<=(const FloatConstant& lhs, const FloatConstant& rhs);
bool operator>=(const FloatConstant& lhs, const FloatConstant& rhs);

/* [[[cog
    import unions
    import mir
    unions.define_type(mir.Constant)
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

    Constant(const Integer& integer);
    Constant(const Float& f);
    Constant(const String& string);
    Constant(const Symbol& symbol);
    Constant(const Null& null);
    Constant(const True& t);
    Constant(const False& f);

    ConstantType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    void build_hash(Hasher& h) const;

    const Integer& as_integer() const;
    const Float& as_float() const;
    const String& as_string() const;
    const Symbol& as_symbol() const;
    const Null& as_null() const;
    const True& as_true() const;
    const False& as_false() const;

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

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
    import unions
    import mir
    unions.define_type(mir.RValueType)
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
    MethodHandle,
    MethodCall,
    MakeEnvironment,
    MakeClosure,
    Container,
    Format,
};

std::string_view to_string(RValueType type);
// [[[end]]]

/* [[[cog
    import unions
    import mir
    unions.define_type(mir.RValue)
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
        LocalID target;

        explicit UseLocal(const LocalID& target_)
            : target(target_) {}
    };

    /// Phi nodes can have one of multiple locals as their value, depending on the code path that led to them.
    struct Phi final {
        /// The possible alternatives.
        PhiID value;

        explicit Phi(const PhiID& value_)
            : value(value_) {}
    };

    /// Marker to store the fact that this local has been visited during variable resolution (used to stop recursion).
    struct Phi0 final {};

    /// A constant.
    using Constant = mir::Constant;

    /// Deferences the function's outer closure environment
    struct OuterEnvironment final {};

    /// Simple binary operation.
    struct BinaryOp final {
        BinaryOpType op;

        /// Left operand.
        LocalID left;

        /// Right operand.
        LocalID right;

        BinaryOp(const BinaryOpType& op_, const LocalID& left_,
            const LocalID& right_)
            : op(op_)
            , left(left_)
            , right(right_) {}
    };

    /// Simple unary operation.
    struct UnaryOp final {
        UnaryOpType op;
        LocalID operand;

        UnaryOp(const UnaryOpType& op_, const LocalID& operand_)
            : op(op_)
            , operand(operand_) {}
    };

    /// Function call expression, i.e. `f(a, b, c)`.
    struct Call final {
        /// Function to call.
        LocalID func;

        /// The list of function arguments.
        LocalListID args;

        Call(const LocalID& func_, const LocalListID& args_)
            : func(func_)
            , args(args_) {}
    };

    /// Represents an evaluated method access on an object, i.e. `object.method()`.
    /// This is a separate value in order to support left-to-right evaluation order.
    struct MethodHandle final {
        /// The object instance.
        LocalID instance;

        /// The name of the method.
        InternedString method;

        MethodHandle(const LocalID& instance_, const InternedString& method_)
            : instance(instance_)
            , method(method_) {}
    };

    /// Method call expression, i.e `a.b(c, d)`.
    struct MethodCall final {
        /// Method to be called. Must be a method handle.
        LocalID method;

        /// List of method arguments.
        LocalListID args;

        MethodCall(const LocalID& method_, const LocalListID& args_)
            : method(method_)
            , args(args_) {}
    };

    /// Creates a new closure environment.
    struct MakeEnvironment final {
        /// The parent environment.
        LocalID parent;

        /// The number of variable slots in the new environment.
        u32 size;

        MakeEnvironment(const LocalID& parent_, const u32& size_)
            : parent(parent_)
            , size(size_) {}
    };

    /// Creates a new closure function.
    struct MakeClosure final {
        /// The closure environment.
        LocalID env;

        /// The closure function's template location.
        LocalID func;

        MakeClosure(const LocalID& env_, const LocalID& func_)
            : env(env_)
            , func(func_) {}
    };

    /// Construct a container from the argument list,
    /// such as an array, a tuple or a map.
    struct Container final {
        /// Container type we're going to construct.
        ContainerType container;

        /// Arguments for the container constructor (list of elements,
        /// or list of key/value-pairs in the case of Map).
        LocalListID args;

        Container(const ContainerType& container_, const LocalListID& args_)
            : container(container_)
            , args(args_) {}
    };

    /// Takes a list of values and formats them as a string.
    /// This is used to implement format string literals.
    struct Format final {
        /// The list of values.
        LocalListID args;

        explicit Format(const LocalListID& args_)
            : args(args_) {}
    };

    static RValue make_use_lvalue(const LValue& target);
    static RValue make_use_local(const LocalID& target);
    static RValue make_phi(const PhiID& value);
    static RValue make_phi0();
    static RValue make_constant(const Constant& constant);
    static RValue make_outer_environment();
    static RValue make_binary_op(
        const BinaryOpType& op, const LocalID& left, const LocalID& right);
    static RValue make_unary_op(const UnaryOpType& op, const LocalID& operand);
    static RValue make_call(const LocalID& func, const LocalListID& args);
    static RValue
    make_method_handle(const LocalID& instance, const InternedString& method);
    static RValue
    make_method_call(const LocalID& method, const LocalListID& args);
    static RValue make_make_environment(const LocalID& parent, const u32& size);
    static RValue make_make_closure(const LocalID& env, const LocalID& func);
    static RValue
    make_container(const ContainerType& container, const LocalListID& args);
    static RValue make_format(const LocalListID& args);

    RValue(const UseLValue& use_lvalue);
    RValue(const UseLocal& use_local);
    RValue(const Phi& phi);
    RValue(const Phi0& phi0);
    RValue(const Constant& constant);
    RValue(const OuterEnvironment& outer_environment);
    RValue(const BinaryOp& binary_op);
    RValue(const UnaryOp& unary_op);
    RValue(const Call& call);
    RValue(const MethodHandle& method_handle);
    RValue(const MethodCall& method_call);
    RValue(const MakeEnvironment& make_environment);
    RValue(const MakeClosure& make_closure);
    RValue(const Container& container);
    RValue(const Format& format);

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
    const MethodHandle& as_method_handle() const;
    const MethodCall& as_method_call() const;
    const MakeEnvironment& as_make_environment() const;
    const MakeClosure& as_make_closure() const;
    const Container& as_container() const;
    const Format& as_format() const;

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

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
        MethodHandle method_handle_;
        MethodCall method_call_;
        MakeEnvironment make_environment_;
        MakeClosure make_closure_;
        Container container_;
        Format format_;
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
    Phi();
    Phi(std::initializer_list<LocalID> operands);
    Phi(std::vector<LocalID>&& operands); // TODO small vec

    ~Phi();

    Phi(Phi&&) noexcept = default;
    Phi& operator=(Phi&&) noexcept = default;

    // Prevent accidental copying
    Phi(const Phi&) = delete;
    Phi& operator=(const Phi&) = delete;

    void append_operand(LocalID operand);

    void format(FormatStream& stream) const;

    auto operands() const {
        return IterRange(operands_.begin(), operands_.end());
    }

    LocalID operand(size_t index) const;

    void operand(size_t index, LocalID local);

    size_t operand_count() const { return operands_.size(); }

private:
    // TODO: Track phi node users
    // TODO Small Vector
    std::vector<LocalID> operands_;
};

/// Represents a list of local variables, e.g. the arguments to a function call
/// or the items of an array.
class LocalList final {
public:
    LocalList();
    LocalList(std::initializer_list<LocalID> rvalues);
    LocalList(std::vector<LocalID>&& locals);
    ~LocalList();

    LocalList(LocalList&&) noexcept = default;
    LocalList& operator=(LocalList&&) noexcept = default;

    // Prevent accidental copying.
    LocalList(const LocalList&) = delete;
    LocalList& operator=(const LocalList&) = delete;

    auto begin() const { return locals_.begin(); }
    auto end() const { return locals_.end(); }

    size_t size() const { return locals_.size(); }
    bool empty() const { return locals_.empty(); }

    LocalID operator[](size_t index) const {
        TIRO_ASSERT(index < locals_.size(), "Index out of bounds.");
        return locals_[index];
    }

    LocalID get(size_t index) const { return (*this)[index]; }

    void set(size_t index, mir::LocalID value) {
        TIRO_ASSERT(index < locals_.size(), "Index out of bounds.");
        locals_[index] = value;
    }

    void remove(size_t index, size_t count) {
        TIRO_ASSERT(index <= locals_.size() && count <= locals_.size() - index,
            "Range out of bounds.");
        const auto pos = locals_.begin() + index;
        locals_.erase(pos, pos + count);
    }

    void append(LocalID local) { locals_.push_back(local); }

private:
    // TODO Small Vector
    std::vector<LocalID> locals_;
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
    import unions
    import mir
    unions.define_type(mir.StmtType)
]]] */
enum class StmtType : u8 {
    Assign,
    Define,
};

std::string_view to_string(StmtType type);
// [[[end]]]

/* [[[cog
    import unions
    import mir
    unions.define_type(mir.Stmt)
]]] */
/// Represents a statement, i.e. a single instruction inside a basic block.
class Stmt final {
public:
    /// Assigns a value to a memory location (non-SSA operations).
    struct Assign final {
        /// The assignment target.
        LValue target;

        /// The new value.
        LocalID value;

        Assign(const LValue& target_, const LocalID& value_)
            : target(target_)
            , value(value_) {}
    };

    /// Defines a new local variable (SSA).
    struct Define final {
        LocalID local;

        explicit Define(const LocalID& local_)
            : local(local_) {}
    };

    static Stmt make_assign(const LValue& target, const LocalID& value);
    static Stmt make_define(const LocalID& local);

    Stmt(const Assign& assign);
    Stmt(const Define& define);

    StmtType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Assign& as_assign() const;
    const Define& as_define() const;

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

private:
    StmtType type_;
    union {
        Assign assign_;
        Define define_;
    };
};
// [[[end]]]

/// True if the statement defines a new phi node.
bool is_phi_define(const Function& func, const mir::Stmt& stmt);

/* [[[cog
    import cog
    import unions
    import mir
    types = [mir.ModuleMember, mir.Terminator, mir.LValue, mir.Constant, mir.RValue, mir.Stmt]
    for index, type in enumerate(types):
        if index != 0:
            cog.outl()
        unions.define_inlines(type)
]]] */
template<typename Self, typename Visitor>
decltype(auto) ModuleMember::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case ModuleMemberType::Import:
        return vis.visit_import(self.import_);
    case ModuleMemberType::Variable:
        return vis.visit_variable(self.variable_);
    case ModuleMemberType::Function:
        return vis.visit_function(self.function_);
    }
    TIRO_UNREACHABLE("Invalid ModuleMember type.");
}

template<typename Self, typename Visitor>
decltype(auto) Terminator::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case TerminatorType::None:
        return vis.visit_none(self.none_);
    case TerminatorType::Jump:
        return vis.visit_jump(self.jump_);
    case TerminatorType::Branch:
        return vis.visit_branch(self.branch_);
    case TerminatorType::Return:
        return vis.visit_return(self.return_);
    case TerminatorType::Exit:
        return vis.visit_exit(self.exit_);
    case TerminatorType::AssertFail:
        return vis.visit_assert_fail(self.assert_fail_);
    case TerminatorType::Never:
        return vis.visit_never(self.never_);
    }
    TIRO_UNREACHABLE("Invalid Terminator type.");
}

template<typename Self, typename Visitor>
decltype(auto) LValue::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case LValueType::Param:
        return vis.visit_param(self.param_);
    case LValueType::Closure:
        return vis.visit_closure(self.closure_);
    case LValueType::Module:
        return vis.visit_module(self.module_);
    case LValueType::Field:
        return vis.visit_field(self.field_);
    case LValueType::TupleField:
        return vis.visit_tuple_field(self.tuple_field_);
    case LValueType::Index:
        return vis.visit_index(self.index_);
    }
    TIRO_UNREACHABLE("Invalid LValue type.");
}

template<typename Self, typename Visitor>
decltype(auto) Constant::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case ConstantType::Integer:
        return vis.visit_integer(self.integer_);
    case ConstantType::Float:
        return vis.visit_float(self.float_);
    case ConstantType::String:
        return vis.visit_string(self.string_);
    case ConstantType::Symbol:
        return vis.visit_symbol(self.symbol_);
    case ConstantType::Null:
        return vis.visit_null(self.null_);
    case ConstantType::True:
        return vis.visit_true(self.true_);
    case ConstantType::False:
        return vis.visit_false(self.false_);
    }
    TIRO_UNREACHABLE("Invalid Constant type.");
}

template<typename Self, typename Visitor>
decltype(auto) RValue::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case RValueType::UseLValue:
        return vis.visit_use_lvalue(self.use_lvalue_);
    case RValueType::UseLocal:
        return vis.visit_use_local(self.use_local_);
    case RValueType::Phi:
        return vis.visit_phi(self.phi_);
    case RValueType::Phi0:
        return vis.visit_phi0(self.phi0_);
    case RValueType::Constant:
        return vis.visit_constant(self.constant_);
    case RValueType::OuterEnvironment:
        return vis.visit_outer_environment(self.outer_environment_);
    case RValueType::BinaryOp:
        return vis.visit_binary_op(self.binary_op_);
    case RValueType::UnaryOp:
        return vis.visit_unary_op(self.unary_op_);
    case RValueType::Call:
        return vis.visit_call(self.call_);
    case RValueType::MethodHandle:
        return vis.visit_method_handle(self.method_handle_);
    case RValueType::MethodCall:
        return vis.visit_method_call(self.method_call_);
    case RValueType::MakeEnvironment:
        return vis.visit_make_environment(self.make_environment_);
    case RValueType::MakeClosure:
        return vis.visit_make_closure(self.make_closure_);
    case RValueType::Container:
        return vis.visit_container(self.container_);
    case RValueType::Format:
        return vis.visit_format(self.format_);
    }
    TIRO_UNREACHABLE("Invalid RValue type.");
}

template<typename Self, typename Visitor>
decltype(auto) Stmt::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case StmtType::Assign:
        return vis.visit_assign(self.assign_);
    case StmtType::Define:
        return vis.visit_define(self.define_);
    }
    TIRO_UNREACHABLE("Invalid Stmt type.");
}
// [[[end]]]

namespace dump_helpers {

struct DumpBlock {
    const Function& parent;
    BlockID block;
};

void format(const DumpBlock& d, FormatStream& stream);

struct DumpTerminator {
    const Function& parent;
    const Terminator& value;
};

void format(const DumpTerminator& d, FormatStream& stream);

struct DumpLValue {
    const Function& parent;
    const LValue& value;
};

void format(const DumpLValue& d, FormatStream& stream);

struct DumpConstant {
    const Function& parent;
    const Constant& value;
};

void format(const DumpConstant& d, FormatStream& stream);

struct DumpRValue {
    const Function& parent;
    const RValue& value;
};

void format(const DumpRValue& d, FormatStream& stream);

struct DumpLocal {
    const Function& parent;
    LocalID local;
};

void format(const DumpLocal& d, FormatStream& stream);

struct DumpDefine {
    const Function& parent;
    LocalID local;
};

void format(const DumpDefine& d, FormatStream& stream);

struct DumpPhi {
    const Function& parent;
    PhiID phi;
};

void format(const DumpPhi& d, FormatStream& stream);

struct DumpLocalList {
    const Function& parent;
    LocalListID list;
};

void format(const DumpLocalList& d, FormatStream& stream);

struct DumpStmt {
    const Function& parent;
    const Stmt& stmt;
};

void format(const DumpStmt& d, FormatStream& stream);

}; // namespace dump_helpers

} // namespace tiro::mir

TIRO_ENABLE_BUILD_HASH(tiro::mir::FloatConstant)
TIRO_ENABLE_BUILD_HASH(tiro::mir::Constant)

TIRO_ENABLE_FREE_TO_STRING(tiro::mir::ModuleMemberType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::mir::ModuleMember)
TIRO_ENABLE_FREE_TO_STRING(tiro::mir::FunctionType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::mir::Block)
TIRO_ENABLE_MEMBER_FORMAT(tiro::mir::Param)
TIRO_ENABLE_FREE_TO_STRING(tiro::mir::LocalType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::mir::Local)
TIRO_ENABLE_FREE_TO_STRING(tiro::mir::TerminatorType)
TIRO_ENABLE_FREE_TO_STRING(tiro::mir::BranchType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::mir::Terminator)
TIRO_ENABLE_FREE_TO_STRING(tiro::mir::LValueType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::mir::LValue)
TIRO_ENABLE_FREE_TO_STRING(tiro::mir::ConstantType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::mir::FloatConstant)
TIRO_ENABLE_MEMBER_FORMAT(tiro::mir::Constant)
TIRO_ENABLE_FREE_TO_STRING(tiro::mir::RValueType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::mir::RValue)
TIRO_ENABLE_MEMBER_FORMAT(tiro::mir::Phi)
TIRO_ENABLE_FREE_TO_STRING(tiro::mir::BinaryOpType)
TIRO_ENABLE_FREE_TO_STRING(tiro::mir::UnaryOpType)
TIRO_ENABLE_FREE_TO_STRING(tiro::mir::ContainerType)
TIRO_ENABLE_FREE_TO_STRING(tiro::mir::StmtType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::mir::Stmt)

TIRO_ENABLE_FREE_FORMAT(tiro::mir::dump_helpers::DumpBlock)
TIRO_ENABLE_FREE_FORMAT(tiro::mir::dump_helpers::DumpTerminator)
TIRO_ENABLE_FREE_FORMAT(tiro::mir::dump_helpers::DumpLValue)
TIRO_ENABLE_FREE_FORMAT(tiro::mir::dump_helpers::DumpConstant)
TIRO_ENABLE_FREE_FORMAT(tiro::mir::dump_helpers::DumpRValue)
TIRO_ENABLE_FREE_FORMAT(tiro::mir::dump_helpers::DumpLocal)
TIRO_ENABLE_FREE_FORMAT(tiro::mir::dump_helpers::DumpDefine)
TIRO_ENABLE_FREE_FORMAT(tiro::mir::dump_helpers::DumpPhi)
TIRO_ENABLE_FREE_FORMAT(tiro::mir::dump_helpers::DumpLocalList)
TIRO_ENABLE_FREE_FORMAT(tiro::mir::dump_helpers::DumpStmt)

#endif // TIRO_MIR_TYPES_HPP
