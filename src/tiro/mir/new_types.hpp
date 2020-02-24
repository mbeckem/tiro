#ifndef TIRO_MIR_NEW_TYPES_HPP
#define TIRO_MIR_NEW_TYPES_HPP

#include "tiro/compiler/string_table.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/format_stream.hpp"
#include "tiro/core/id_type.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/mir/fwd.hpp"
#include "tiro/mir/vec_ptr.hpp"

namespace tiro::compiler::mir {

TIRO_DEFINE_ID(BlockID, u32)
TIRO_DEFINE_ID(ScopeID, u32)
TIRO_DEFINE_ID(ParamID, u32)
TIRO_DEFINE_ID(LocalID, u32)
TIRO_DEFINE_ID(LocalListID, u32)

class Function final {
public:
    explicit Function(StringTable& strings);
    ~Function();

    Function(Function&&) noexcept = default;
    Function& operator=(Function&&) noexcept = default;

    Function(const Function&) = delete;
    Function& operator=(const Function&) = delete;

    BlockID make(Block block);
    ScopeID make(Scope scope);
    ParamID make(Param param);
    LocalID make(Local local);
    LocalListID make(LocalList rvalue_list);

    BlockID entry() const;
    BlockID exit() const;

    NotNull<VecPtr<Block>> operator[](BlockID id);

private:
    template<typename ID, typename Vec, typename T>
    ID add_impl(Vec& vec, T&& value) {
        const u32 id = check_id_value(vec.size());
        vec.push_back(std::forward<T>(value));
        return ID(id);
    }

    template<typename ID, typename Vec>
    bool check_id(const ID& id, const Vec& vec) {
        return id && id.value() < vec.size();
    }

    static u32 check_id_value(size_t vector_index);

private:
    // Improvement: Can make these allocate from an arena instead
    std::vector<Block> blocks_;
    std::vector<Scope> scopes_;
    std::vector<Param> params_;
    std::vector<Local> locals_;
    std::vector<LocalList> local_lists_;

    BlockID entry_;
    BlockID exit_;
};

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
    import mir
    codegen.define_type(mir.EdgeType)
]]] */
enum class EdgeType : u8 {
    None,
    Jump,
    Branch,
    Return,
    AssertFail,
    Never,
};

std::string_view to_string(EdgeType type);
// [[[end]]]

/* [[[cog
    import mir
    codegen.define_type(mir.Edge)
]]] */
/// Represents an edge that connects two basic blocks.
class Edge final {
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

    /// The block returns from the function.
    struct Return final {};

    /// An assertion failure is an unconditional hard exit.
    struct AssertFail final {
        /// The message that will be printed when the assertion fails.
        LocalID message;

        explicit AssertFail(const LocalID& message_)
            : message(message_) {}
    };

    /// The block never returns (e.g. contains a statement that never terminates).
    struct Never final {};

    Edge(const None& none)
        : type_(EdgeType::None)
        , none_(none) {}

    Edge(const Jump& jump)
        : type_(EdgeType::Jump)
        , jump_(jump) {}

    Edge(const Branch& branch)
        : type_(EdgeType::Branch)
        , branch_(branch) {}

    Edge(const Return& return_)
        : type_(EdgeType::Return)
        , return_(return_) {}

    Edge(const AssertFail& assert_fail)
        : type_(EdgeType::AssertFail)
        , assert_fail_(assert_fail) {}

    Edge(const Never& never)
        : type_(EdgeType::Never)
        , never_(never) {}

    EdgeType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const None& as_none() const {
        check_access(EdgeType::None);
        return none_;
    }

    const Jump& as_jump() const {
        check_access(EdgeType::Jump);
        return jump_;
    }

    const Branch& as_branch() const {
        check_access(EdgeType::Branch);
        return branch_;
    }

    const Return& as_return() const {
        check_access(EdgeType::Return);
        return return_;
    }

    const AssertFail& as_assert_fail() const {
        check_access(EdgeType::AssertFail);
        return assert_fail_;
    }

    const Never& as_never() const {
        check_access(EdgeType::Never);
        return never_;
    }

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    void check_access([[maybe_unused]] EdgeType type) const {
        TIRO_ASSERT(
            type == type_, "Bad member access on Edge: unexpected type.");
    }

    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

private:
    EdgeType type_;
    union {
        None none_;
        Jump jump_;
        Branch branch_;
        Return return_;
        AssertFail assert_fail_;
        Never never_;
    };
};
// [[[end]]]

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

    const Edge& edge() const { return edge_; }
    void edge(const Edge& edge) { edge_ = edge; }

    auto stmts() const { return IterRange(stmts_.begin(), stmts_.end()); }

    void append(const Stmt& stmt);

    void format(FormatStream& stream) const;

private:
    InternedString label_;
    Edge edge_ = Edge::None{};
    std::vector<Stmt> stmts_;
};

/// Represents a scope in which local variables are declared.
class Scope final {
public:
    explicit Scope(ScopeID parent)
        : parent_(parent) {}

    /// The (optional) parent scope.
    ScopeID parent() const noexcept { return parent_; }

    // TODO: Source range

    void format(FormatStream& stream) const;

private:
    ScopeID parent_;
};

/* [[[cog
    import mir
    codegen.define_type(mir.LValueType)
]]] */
enum class LValueType : u8 {
    Argument,
    Closure,
    Module,
    Field,
    TupleField,
    Index,
};

std::string_view to_string(LValueType type);
// [[[end]]]

/* [[[cog
    import mir
    codegen.define_type(mir.LValue)
]]] */
/// LValues can appear as the left hand side of an assignment.
/// They are associated with a mutable storage location.
/// LValues do not use SSA form since they may reference memory shared
/// with other parts of the program.
class LValue final {
public:
    /// Reference to a function argument.
    struct Argument final {
        /// Argument index in parameter list.
        u32 index;

        explicit Argument(const u32& index_)
            : index(index_) {}
    };

    /// Reference to a variable captured from an outer scope.
    struct Closure final {
        /// The context to search. Either a local variable or the function's outer context.
        LocalID context;

        /// Levels to "go up" the closure hierarchy. 0 is the closure context itself.
        u32 levels;

        /// Index into the closure context.
        u32 index;

        Closure(const LocalID& context_, const u32& levels_, const u32& index_)
            : context(context_)
            , levels(levels_)
            , index(index_) {}
    };

    /// Reference to a variable at module scope.
    struct Module final {
        /// Index into the module.
        u32 index;

        explicit Module(const u32& index_)
            : index(index_) {}
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

    LValue(const Argument& argument)
        : type_(LValueType::Argument)
        , argument_(argument) {}

    LValue(const Closure& closure)
        : type_(LValueType::Closure)
        , closure_(closure) {}

    LValue(const Module& module)
        : type_(LValueType::Module)
        , module_(module) {}

    LValue(const Field& field)
        : type_(LValueType::Field)
        , field_(field) {}

    LValue(const TupleField& tuple_field)
        : type_(LValueType::TupleField)
        , tuple_field_(tuple_field) {}

    LValue(const Index& index)
        : type_(LValueType::Index)
        , index_(index) {}

    LValueType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Argument& as_argument() const {
        check_access(LValueType::Argument);
        return argument_;
    }

    const Closure& as_closure() const {
        check_access(LValueType::Closure);
        return closure_;
    }

    const Module& as_module() const {
        check_access(LValueType::Module);
        return module_;
    }

    const Field& as_field() const {
        check_access(LValueType::Field);
        return field_;
    }

    const TupleField& as_tuple_field() const {
        check_access(LValueType::TupleField);
        return tuple_field_;
    }

    const Index& as_index() const {
        check_access(LValueType::Index);
        return index_;
    }

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    void check_access([[maybe_unused]] LValueType type) const {
        TIRO_ASSERT(
            type == type_, "Bad member access on LValue: unexpected type.");
    }

    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

private:
    LValueType type_;
    union {
        Argument argument_;
        Closure closure_;
        Module module_;
        Field field_;
        TupleField tuple_field_;
        Index index_;
    };
};
// [[[end]]]

/* [[[cog
    import mir
    codegen.define_type(mir.ConstantType)
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

/* [[[cog
    import mir
    codegen.define_type(mir.Constant)
]]] */
/// Represents a compile time constant.
class Constant final {
public:
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

    struct Null final {};

    struct True final {};

    struct False final {};

    Constant(const Integer& integer)
        : type_(ConstantType::Integer)
        , integer_(integer) {}

    Constant(const Float& float_)
        : type_(ConstantType::Float)
        , float_(float_) {}

    Constant(const String& string)
        : type_(ConstantType::String)
        , string_(string) {}

    Constant(const Symbol& symbol)
        : type_(ConstantType::Symbol)
        , symbol_(symbol) {}

    Constant(const Null& null)
        : type_(ConstantType::Null)
        , null_(null) {}

    Constant(const True& true_)
        : type_(ConstantType::True)
        , true_(true_) {}

    Constant(const False& false_)
        : type_(ConstantType::False)
        , false_(false_) {}

    ConstantType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Integer& as_integer() const {
        check_access(ConstantType::Integer);
        return integer_;
    }

    const Float& as_float() const {
        check_access(ConstantType::Float);
        return float_;
    }

    const String& as_string() const {
        check_access(ConstantType::String);
        return string_;
    }

    const Symbol& as_symbol() const {
        check_access(ConstantType::Symbol);
        return symbol_;
    }

    const Null& as_null() const {
        check_access(ConstantType::Null);
        return null_;
    }

    const True& as_true() const {
        check_access(ConstantType::True);
        return true_;
    }

    const False& as_false() const {
        check_access(ConstantType::False);
        return false_;
    }

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    void check_access([[maybe_unused]] ConstantType type) const {
        TIRO_ASSERT(
            type == type_, "Bad member access on Constant: unexpected type.");
    }

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
// [[[end]]]

/* [[[cog
    import mir
    codegen.define_type(mir.RValueType)
]]] */
enum class RValueType : u8 {
    UseLValue,
    UseLocal,
    Constant,
    OuterContext,
    BinaryOp,
    UnaryOp,
    Call,
    MethodCall,
    Container,
};

std::string_view to_string(RValueType type);
// [[[end]]]

/* [[[cog
    import mir
    codegen.define_type(mir.RValue)
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

    using Constant = mir::Constant;

    /// Deferences the function's outer closure context
    struct OuterContext final {};

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

    /// Method call expression, i.e `a.b(c, d)`.
    struct MethodCall final {
        /// Object whose method we're going to invoke.
        LocalID object;

        /// Name of the method to be called.
        InternedString method;

        /// List of method arguments.
        LocalListID args;

        MethodCall(const LocalID& object_, const InternedString& method_,
            const LocalListID& args_)
            : object(object_)
            , method(method_)
            , args(args_) {}
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

    RValue(const UseLValue& use_lvalue)
        : type_(RValueType::UseLValue)
        , use_lvalue_(use_lvalue) {}

    RValue(const UseLocal& use_local)
        : type_(RValueType::UseLocal)
        , use_local_(use_local) {}

    RValue(const Constant& constant)
        : type_(RValueType::Constant)
        , constant_(constant) {}

    RValue(const OuterContext& outer_context)
        : type_(RValueType::OuterContext)
        , outer_context_(outer_context) {}

    RValue(const BinaryOp& binary_op)
        : type_(RValueType::BinaryOp)
        , binary_op_(binary_op) {}

    RValue(const UnaryOp& unary_op)
        : type_(RValueType::UnaryOp)
        , unary_op_(unary_op) {}

    RValue(const Call& call)
        : type_(RValueType::Call)
        , call_(call) {}

    RValue(const MethodCall& method_call)
        : type_(RValueType::MethodCall)
        , method_call_(method_call) {}

    RValue(const Container& container)
        : type_(RValueType::Container)
        , container_(container) {}

    RValueType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const UseLValue& as_use_lvalue() const {
        check_access(RValueType::UseLValue);
        return use_lvalue_;
    }

    const UseLocal& as_use_local() const {
        check_access(RValueType::UseLocal);
        return use_local_;
    }

    const Constant& as_constant() const {
        check_access(RValueType::Constant);
        return constant_;
    }

    const OuterContext& as_outer_context() const {
        check_access(RValueType::OuterContext);
        return outer_context_;
    }

    const BinaryOp& as_binary_op() const {
        check_access(RValueType::BinaryOp);
        return binary_op_;
    }

    const UnaryOp& as_unary_op() const {
        check_access(RValueType::UnaryOp);
        return unary_op_;
    }

    const Call& as_call() const {
        check_access(RValueType::Call);
        return call_;
    }

    const MethodCall& as_method_call() const {
        check_access(RValueType::MethodCall);
        return method_call_;
    }

    const Container& as_container() const {
        check_access(RValueType::Container);
        return container_;
    }

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    void check_access([[maybe_unused]] RValueType type) const {
        TIRO_ASSERT(
            type == type_, "Bad member access on RValue: unexpected type.");
    }

    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

private:
    RValueType type_;
    union {
        UseLValue use_lvalue_;
        UseLocal use_local_;
        Constant constant_;
        OuterContext outer_context_;
        BinaryOp binary_op_;
        UnaryOp unary_op_;
        Call call_;
        MethodCall method_call_;
        Container container_;
    };
};
// [[[end]]]

/// Represents the type of a local variable.
enum class LocalType : u8 {
    Declared, ///< Declared by the user.
    Temp,     ///< Generated by the compiler.
};

std::string to_string(LocalType type);

/// Represents a local variable (user defined or temporary).
/// Locals use SSA (Static Single Assignment) form: they are defined exactly once.
class Local final {
public:
    // TODO Source ranges

    /// Creates a new local variable declared by the user.
    static Local
    declared(InternedString name, ScopeID scope, const RValue& value);

    /// Creates a new temporary local variable.
    static Local temp(ScopeID scope, const RValue& value);

    /// Returns the local variable's declaration type (declared or temporary).
    LocalType type() const noexcept { return type_; }

    /// Only declared variables have a valid name.
    InternedString name() const noexcept { return name_; }

    /// Returns the scope in which this variable was declared.
    ScopeID scope() const noexcept { return scope_; }

    /// The rvalue bound to this local.
    /// TODO: PHI
    const RValue& value() const noexcept { return value_; }

    void format(FormatStream& stream) const;

private:
    explicit Local(const RValue& value);

private:
    LocalType type_;
    InternedString name_;
    ScopeID scope_;
    RValue value_;
};

/// Represents a list of local variables, e.g. the arguments to a function call
/// or the items of an array.
class LocalList final {
public:
    LocalList();
    LocalList(std::initializer_list<LocalID> rvalues);
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
    import mir
    codegen.define_type(mir.StmtType)
]]] */
enum class StmtType : u8 {
    Assign,
    Define,
    SetReturn,
    EnterScope,
    ExitScope,
};

std::string_view to_string(StmtType type);
// [[[end]]]

/* [[[cog
    import mir
    codegen.define_type(mir.Stmt)
]]] */
/// Represents a statement, i.e. a single instruction inside a basic block.
class Stmt final {
public:
    /// Assigns a value to a memory location (non-SSA operations).
    struct Assign final {
        /// The assignment target.
        LValue target;

        /// The new value.
        RValue value;

        Assign(const LValue& target_, const RValue& value_)
            : target(target_)
            , value(value_) {}
    };

    /// Defines a new local variable (SSA).
    struct Define final {
        LocalID local;

        explicit Define(const LocalID& local_)
            : local(local_) {}
    };

    /// Sets the function's return value.
    struct SetReturn final {
        /// The return value.
        LocalID value;

        explicit SetReturn(const LocalID& value_)
            : value(value_) {}
    };

    /// Marks the start of a lexical scope.
    struct EnterScope final {
        /// The id of the scope.
        ScopeID scope;

        explicit EnterScope(const ScopeID& scope_)
            : scope(scope_) {}
    };

    /// Marks the end of a lexical scope.
    struct ExitScope final {
        /// The id of the scope.
        ScopeID scope;

        explicit ExitScope(const ScopeID& scope_)
            : scope(scope_) {}
    };

    Stmt(const Assign& assign)
        : type_(StmtType::Assign)
        , assign_(assign) {}

    Stmt(const Define& define)
        : type_(StmtType::Define)
        , define_(define) {}

    Stmt(const SetReturn& set_return)
        : type_(StmtType::SetReturn)
        , set_return_(set_return) {}

    Stmt(const EnterScope& enter_scope)
        : type_(StmtType::EnterScope)
        , enter_scope_(enter_scope) {}

    Stmt(const ExitScope& exit_scope)
        : type_(StmtType::ExitScope)
        , exit_scope_(exit_scope) {}

    StmtType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Assign& as_assign() const {
        check_access(StmtType::Assign);
        return assign_;
    }

    const Define& as_define() const {
        check_access(StmtType::Define);
        return define_;
    }

    const SetReturn& as_set_return() const {
        check_access(StmtType::SetReturn);
        return set_return_;
    }

    const EnterScope& as_enter_scope() const {
        check_access(StmtType::EnterScope);
        return enter_scope_;
    }

    const ExitScope& as_exit_scope() const {
        check_access(StmtType::ExitScope);
        return exit_scope_;
    }

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    void check_access([[maybe_unused]] StmtType type) const {
        TIRO_ASSERT(
            type == type_, "Bad member access on Stmt: unexpected type.");
    }

    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

private:
    StmtType type_;
    union {
        Assign assign_;
        Define define_;
        SetReturn set_return_;
        EnterScope enter_scope_;
        ExitScope exit_scope_;
    };
};
// [[[end]]]

/* [[[cog
    import cog
    import mir
    types = [mir.Edge, mir.LValue, mir.Constant, mir.RValue, mir.Stmt]
    for index, type in enumerate(types):
        if index != 0:
            cog.outl()
        codegen.define_inlines(type)
]]] */
template<typename Self, typename Visitor>
decltype(auto) Edge::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case EdgeType::None:
        return vis(self.as_none());
    case EdgeType::Jump:
        return vis(self.as_jump());
    case EdgeType::Branch:
        return vis(self.as_branch());
    case EdgeType::Return:
        return vis(self.as_return());
    case EdgeType::AssertFail:
        return vis(self.as_assert_fail());
    case EdgeType::Never:
        return vis(self.as_never());
    }
    TIRO_UNREACHABLE("Invalid Edge type.");
}

template<typename Self, typename Visitor>
decltype(auto) LValue::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case LValueType::Argument:
        return vis(self.as_argument());
    case LValueType::Closure:
        return vis(self.as_closure());
    case LValueType::Module:
        return vis(self.as_module());
    case LValueType::Field:
        return vis(self.as_field());
    case LValueType::TupleField:
        return vis(self.as_tuple_field());
    case LValueType::Index:
        return vis(self.as_index());
    }
    TIRO_UNREACHABLE("Invalid LValue type.");
}

template<typename Self, typename Visitor>
decltype(auto) Constant::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case ConstantType::Integer:
        return vis(self.as_integer());
    case ConstantType::Float:
        return vis(self.as_float());
    case ConstantType::String:
        return vis(self.as_string());
    case ConstantType::Symbol:
        return vis(self.as_symbol());
    case ConstantType::Null:
        return vis(self.as_null());
    case ConstantType::True:
        return vis(self.as_true());
    case ConstantType::False:
        return vis(self.as_false());
    }
    TIRO_UNREACHABLE("Invalid Constant type.");
}

template<typename Self, typename Visitor>
decltype(auto) RValue::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case RValueType::UseLValue:
        return vis(self.as_use_lvalue());
    case RValueType::UseLocal:
        return vis(self.as_use_local());
    case RValueType::Constant:
        return vis(self.as_constant());
    case RValueType::OuterContext:
        return vis(self.as_outer_context());
    case RValueType::BinaryOp:
        return vis(self.as_binary_op());
    case RValueType::UnaryOp:
        return vis(self.as_unary_op());
    case RValueType::Call:
        return vis(self.as_call());
    case RValueType::MethodCall:
        return vis(self.as_method_call());
    case RValueType::Container:
        return vis(self.as_container());
    }
    TIRO_UNREACHABLE("Invalid RValue type.");
}

template<typename Self, typename Visitor>
decltype(auto) Stmt::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case StmtType::Assign:
        return vis(self.as_assign());
    case StmtType::Define:
        return vis(self.as_define());
    case StmtType::SetReturn:
        return vis(self.as_set_return());
    case StmtType::EnterScope:
        return vis(self.as_enter_scope());
    case StmtType::ExitScope:
        return vis(self.as_exit_scope());
    }
    TIRO_UNREACHABLE("Invalid Stmt type.");
}
// [[[end]]]

/* [[[cog
    import cog
    import mir
    types = [mir.Edge, mir.LValue, mir.Constant, mir.RValue, mir.Stmt]

    check_trivial = lambda name: (
        f"static_assert(std::is_trivially_copyable_v<{name}>);\n"
        f"static_assert(std::is_trivially_destructible_v<{name}>);"
    )

    for index, type in enumerate(types):
        if index != 0:
            cog.outl()

        for member in type.members:
            cog.outl(check_trivial(f"{type.name}::{member.name}"))
        cog.outl(check_trivial(type.name))
]]] */
static_assert(std::is_trivially_copyable_v<Edge::None>);
static_assert(std::is_trivially_destructible_v<Edge::None>);
static_assert(std::is_trivially_copyable_v<Edge::Jump>);
static_assert(std::is_trivially_destructible_v<Edge::Jump>);
static_assert(std::is_trivially_copyable_v<Edge::Branch>);
static_assert(std::is_trivially_destructible_v<Edge::Branch>);
static_assert(std::is_trivially_copyable_v<Edge::Return>);
static_assert(std::is_trivially_destructible_v<Edge::Return>);
static_assert(std::is_trivially_copyable_v<Edge::AssertFail>);
static_assert(std::is_trivially_destructible_v<Edge::AssertFail>);
static_assert(std::is_trivially_copyable_v<Edge::Never>);
static_assert(std::is_trivially_destructible_v<Edge::Never>);
static_assert(std::is_trivially_copyable_v<Edge>);
static_assert(std::is_trivially_destructible_v<Edge>);

static_assert(std::is_trivially_copyable_v<LValue::Argument>);
static_assert(std::is_trivially_destructible_v<LValue::Argument>);
static_assert(std::is_trivially_copyable_v<LValue::Closure>);
static_assert(std::is_trivially_destructible_v<LValue::Closure>);
static_assert(std::is_trivially_copyable_v<LValue::Module>);
static_assert(std::is_trivially_destructible_v<LValue::Module>);
static_assert(std::is_trivially_copyable_v<LValue::Field>);
static_assert(std::is_trivially_destructible_v<LValue::Field>);
static_assert(std::is_trivially_copyable_v<LValue::TupleField>);
static_assert(std::is_trivially_destructible_v<LValue::TupleField>);
static_assert(std::is_trivially_copyable_v<LValue::Index>);
static_assert(std::is_trivially_destructible_v<LValue::Index>);
static_assert(std::is_trivially_copyable_v<LValue>);
static_assert(std::is_trivially_destructible_v<LValue>);

static_assert(std::is_trivially_copyable_v<Constant::Integer>);
static_assert(std::is_trivially_destructible_v<Constant::Integer>);
static_assert(std::is_trivially_copyable_v<Constant::Float>);
static_assert(std::is_trivially_destructible_v<Constant::Float>);
static_assert(std::is_trivially_copyable_v<Constant::String>);
static_assert(std::is_trivially_destructible_v<Constant::String>);
static_assert(std::is_trivially_copyable_v<Constant::Symbol>);
static_assert(std::is_trivially_destructible_v<Constant::Symbol>);
static_assert(std::is_trivially_copyable_v<Constant::Null>);
static_assert(std::is_trivially_destructible_v<Constant::Null>);
static_assert(std::is_trivially_copyable_v<Constant::True>);
static_assert(std::is_trivially_destructible_v<Constant::True>);
static_assert(std::is_trivially_copyable_v<Constant::False>);
static_assert(std::is_trivially_destructible_v<Constant::False>);
static_assert(std::is_trivially_copyable_v<Constant>);
static_assert(std::is_trivially_destructible_v<Constant>);

static_assert(std::is_trivially_copyable_v<RValue::UseLValue>);
static_assert(std::is_trivially_destructible_v<RValue::UseLValue>);
static_assert(std::is_trivially_copyable_v<RValue::UseLocal>);
static_assert(std::is_trivially_destructible_v<RValue::UseLocal>);
static_assert(std::is_trivially_copyable_v<RValue::Constant>);
static_assert(std::is_trivially_destructible_v<RValue::Constant>);
static_assert(std::is_trivially_copyable_v<RValue::OuterContext>);
static_assert(std::is_trivially_destructible_v<RValue::OuterContext>);
static_assert(std::is_trivially_copyable_v<RValue::BinaryOp>);
static_assert(std::is_trivially_destructible_v<RValue::BinaryOp>);
static_assert(std::is_trivially_copyable_v<RValue::UnaryOp>);
static_assert(std::is_trivially_destructible_v<RValue::UnaryOp>);
static_assert(std::is_trivially_copyable_v<RValue::Call>);
static_assert(std::is_trivially_destructible_v<RValue::Call>);
static_assert(std::is_trivially_copyable_v<RValue::MethodCall>);
static_assert(std::is_trivially_destructible_v<RValue::MethodCall>);
static_assert(std::is_trivially_copyable_v<RValue::Container>);
static_assert(std::is_trivially_destructible_v<RValue::Container>);
static_assert(std::is_trivially_copyable_v<RValue>);
static_assert(std::is_trivially_destructible_v<RValue>);

static_assert(std::is_trivially_copyable_v<Stmt::Assign>);
static_assert(std::is_trivially_destructible_v<Stmt::Assign>);
static_assert(std::is_trivially_copyable_v<Stmt::Define>);
static_assert(std::is_trivially_destructible_v<Stmt::Define>);
static_assert(std::is_trivially_copyable_v<Stmt::SetReturn>);
static_assert(std::is_trivially_destructible_v<Stmt::SetReturn>);
static_assert(std::is_trivially_copyable_v<Stmt::EnterScope>);
static_assert(std::is_trivially_destructible_v<Stmt::EnterScope>);
static_assert(std::is_trivially_copyable_v<Stmt::ExitScope>);
static_assert(std::is_trivially_destructible_v<Stmt::ExitScope>);
static_assert(std::is_trivially_copyable_v<Stmt>);
static_assert(std::is_trivially_destructible_v<Stmt>);
// [[[end]]]

} // namespace tiro::compiler::mir

TIRO_FORMAT_MEMBER(tiro::compiler::mir::BlockID)
TIRO_FORMAT_MEMBER(tiro::compiler::mir::ScopeID)
TIRO_FORMAT_MEMBER(tiro::compiler::mir::ParamID)
TIRO_FORMAT_MEMBER(tiro::compiler::mir::LocalID)
TIRO_FORMAT_MEMBER(tiro::compiler::mir::LocalListID)
TIRO_FORMAT_MEMBER(tiro::compiler::mir::Block)
TIRO_FORMAT_MEMBER(tiro::compiler::mir::Param)
TIRO_FORMAT_FREE_TO_STRING(tiro::compiler::mir::LocalType)
TIRO_FORMAT_MEMBER(tiro::compiler::mir::Local)
TIRO_FORMAT_FREE_TO_STRING(tiro::compiler::mir::EdgeType)
TIRO_FORMAT_FREE_TO_STRING(tiro::compiler::mir::BranchType)
TIRO_FORMAT_MEMBER(tiro::compiler::mir::Edge)
TIRO_FORMAT_FREE_TO_STRING(tiro::compiler::mir::LValueType)
TIRO_FORMAT_MEMBER(tiro::compiler::mir::LValue)
TIRO_FORMAT_FREE_TO_STRING(tiro::compiler::mir::ConstantType)
TIRO_FORMAT_MEMBER(tiro::compiler::mir::Constant)
TIRO_FORMAT_FREE_TO_STRING(tiro::compiler::mir::RValueType)
TIRO_FORMAT_MEMBER(tiro::compiler::mir::RValue)
TIRO_FORMAT_FREE_TO_STRING(tiro::compiler::mir::BinaryOpType)
TIRO_FORMAT_FREE_TO_STRING(tiro::compiler::mir::UnaryOpType)
TIRO_FORMAT_FREE_TO_STRING(tiro::compiler::mir::ContainerType)
TIRO_FORMAT_FREE_TO_STRING(tiro::compiler::mir::StmtType)
TIRO_FORMAT_MEMBER(tiro::compiler::mir::Stmt)

#endif // TIRO_MIR_NEW_TYPES_HPP
