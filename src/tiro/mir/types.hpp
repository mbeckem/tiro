#ifndef TIRO_MIR_TYPES_HPP
#define TIRO_MIR_TYPES_HPP

#include "tiro/mir/fwd.hpp"
#include "tiro/mir/vec_ptr.hpp"

#include "tiro/compiler/string_table.hpp"
#include "tiro/core/format_stream.hpp"
#include "tiro/core/id_type.hpp"
#include "tiro/core/not_null.hpp"

namespace tiro::compiler::mir {

TIRO_DEFINE_ID(BlockID, u32)
TIRO_DEFINE_ID(ScopeID, u32)
TIRO_DEFINE_ID(ParamID, u32)
TIRO_DEFINE_ID(LocalID, u32)
TIRO_DEFINE_ID(LocalListID, u32)

class Function final {
public:
    Function(StringTable& strings);
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
    std::vector<LValue> lvalues_;
    std::vector<RValue> rvalues_;
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

/// Represents the type of an edge.
enum class EdgeType : u8 {
    /// The block has no outgoing edge. This is the initial value after a new block has been created.
    /// It must be changed to one of the valid edge types when construction is complete.
    None,

    /// A single successor block, reached through an unconditional jump.
    Jump,

    /// A conditional jump with two successor blocks.
    Branch,

    /// The block returns from the function.
    Return,

    /// An assertion failure is an unconditional hard exit.
    AssertFail,

    /// The block never returns (e.g. contains a statement that never terminates).
    Never,

    // TODO: Will probably need a Call type. Calls are rvalues in the graph right now -
    // this will no longer be correct when function calls can throw.
};

std::string_view to_string(EdgeType type);

/// Represents the type of a conditional jump.
enum class BranchType : u8 {
    IfTrue,
    IfFalse,
};

std::string_view to_string(BranchType type);

/// Represents an edge that connects two basic blocks.
class Edge final {
public:
// Format: type name, accessor name, field name
#define TIRO_MIR_EDGES(X)                    \
    X(None, none, none_)                     \
    X(Jump, jump, jump_)                     \
    X(Branch, branch, branch_)               \
    X(Return, ret, ret_)                     \
    X(AssertFail, assert_fail, assert_fail_) \
    X(Never, never, never_)

    struct None {};

    struct Jump {
        BlockID target;
    };

    struct Branch {
        BranchType type;
        LocalID value;
        BlockID target;
        BlockID fallthrough;
    };

    struct Return {};

    struct AssertFail {
        LocalID message;
    };

    struct Never {};

#define TIRO_CONSTRUCTOR(type_name, accessor_name, field_name) \
    Edge(const type_name& arg)                                 \
        : type_(EdgeType::type_name)                           \
        , field_name(arg) {}

    TIRO_MIR_EDGES(TIRO_CONSTRUCTOR)

#undef TIRO_CONSTRUCTOR

    EdgeType type() const noexcept { return type_; }

#define TIRO_ACCESSOR(type_name, accessor_name, field_name) \
    const type_name& accessor_name() const {                \
        check_access(EdgeType::type_name);                  \
        return field_name;                                  \
    }

    TIRO_MIR_EDGES(TIRO_ACCESSOR)

#undef TIRO_ACCESSOR

    void format(FormatStream& stream);

private:
    void check_access(EdgeType required) const {
        TIRO_ASSERT(type_ == required,
            "Invalid access: Edge is not of the requested type.");
    }

private:
    EdgeType type_;
    union {
#define TIRO_MEMBER(type_name, accessor_name, field_name) type_name field_name;
        TIRO_MIR_EDGES(TIRO_MEMBER)
#undef TIRO_MEMBER
    };
};

template<typename Visitor>
inline decltype(auto) visit(const Edge& edge, Visitor&& visitor);

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

/// Represents the type of an lvalue.
enum class LValueType : u8 {
    /// Reference to a function argument.
    Argument,

    /// Reference to a variable captured from an outer scope.
    Closure,

    /// Reference to a variable at module scope.
    Module,

    /// Reference to the field of an object (i.e. `object.foo`).
    Field,

    /// Referencce to a tuple field of a tuple (i.e. `tuple.3`).
    TupleField,

    /// Reference to an index of an array (or a map), i.e. `thing[foo]`.
    Index,
};

std::string_view to_string(LValueType type);

/// LValues can appear as the left hand side of an assignment.
/// They are associated with a mutable storage location.
/// LValues do not use SSA form since they may reference memory shared
/// with other parts of the program.
class LValue final {
public:
// Format: type name, accessor name, field name.
#define TIRO_MIR_LVALUES(X)                  \
    X(Argument, argument, argument_)         \
    X(Closure, closure, closure_)            \
    X(Module, module, module_)               \
    X(Field, field, field_)                  \
    X(TupleField, tuple_field, tuple_field_) \
    X(Index, index, index_)

    struct Argument {
        u32 index; ///< Argument index in parameter list.
    };

    struct Closure {
        /// The context to search. Either a local variable or the function's outer context.
        LocalID context;

        /// Levels to "go up" the closure hierarchy. 0 is the closure context itself.
        u32 levels;

        /// Index into the closure context.
        u32 index;
    };

    struct Module {
        u32 index; ///< Index into the module.
    };

    struct Field {
        LocalID object;      ///< Dereferenced object.
        InternedString name; ///< Field name to access.
    };

    struct TupleField {
        LocalID object; ///< Dereferenced tuple object.
        u32 index;      ///< Index of the tuple member.
    };

    struct Index {
        LocalID object; ///< Dereferenced arraylike object.
        LocalID index;  ///< Index into the array.
    };

// Declare lvalue constructors.
#define TIRO_CONSTRUCTOR(type_name, accessor_name, field_name) \
    LValue(const type_name& arg)                               \
        : type_(LValueType::type_name)                         \
        , field_name(arg) {}

    TIRO_MIR_LVALUES(TIRO_CONSTRUCTOR)

#undef TIRO_CONSTRUCTOR

    LValueType type() const noexcept { return type_; }

// Declare lvalue accessors.
#define TIRO_ACCESSOR(type_name, accessor_name, field_name) \
    const type_name& accessor_name() const {                \
        check_access(LValueType::type_name);                \
        return field_name;                                  \
    }

    TIRO_MIR_LVALUES(TIRO_ACCESSOR)

#undef TIRO_ACCESSOR

    void format(FormatStream& stream) const;

private:
    void check_access(LValueType type) const;

private:
    LValueType type_;
    union {
#define TIRO_MEMBER(type_name, accessor_name, field_name) type_name field_name;
        TIRO_MIR_LVALUES(TIRO_MEMBER)
#undef TIRO_MEMBER
    };
};

template<typename Visitor>
inline decltype(auto) visit(const LValue& value, Visitor&& visitor);

/// Represents the type of a constant.
enum class ConstantType : u8 {
    Integer,
    Float,
    String,
    Symbol,
    Null,
    True,
    False
    // TODO: Constant arrays, tuples, sets and maps
};

std::string_view to_string(ConstantType type);

/// Represents a compile time constant.
class Constant {
public:
// Format: type name, accessor name, variable name
#define TIRO_MIR_CONSTANTS(X)     \
    X(Integer, as_int, int_)      \
    X(Float, as_float, float_)    \
    X(String, as_string, string_) \
    X(Symbol, as_symbol, symbol_) \
    X(Null, as_null, null_)       \
    X(True, as_true, true_)       \
    X(False, as_false, false_)

    struct Integer {
        i64 value;
    };

    struct Float {
        f64 value;
    };

    struct String {
        InternedString value;
    };

    struct Symbol {
        InternedString value;
    };

    struct Null {};
    struct True {};
    struct False {};

#define TIRO_CONSTRUCTOR(type_name, acc, field) \
    Constant(const type_name& arg)              \
        : type_(ConstantType::type_name)        \
        , field(arg) {}

    TIRO_MIR_CONSTANTS(TIRO_CONSTRUCTOR)

#undef TIRO_CONSTRUCTOR

    ConstantType type() const noexcept { return type_; }

#define TIRO_ACCESSOR(type_name, acc, field)   \
    const type_name& acc() const {             \
        check_access(ConstantType::type_name); \
        return field;                          \
    }

    TIRO_MIR_CONSTANTS(TIRO_ACCESSOR)

#undef TIRO_ACCESSOR

    void format(FormatStream& stream) const;

private:
    void check_access(ConstantType expected) const;

private:
    ConstantType type_;
    union {
#define TIRO_MEMBER(type_name, acc, field) type_name field;
        TIRO_MIR_CONSTANTS(TIRO_MEMBER)
#undef TIRO_MEMBER
    };
};

template<typename Visitor>
inline decltype(auto) visit(const Constant& constant, Visitor&& visitor);

/// Represents the type of an rvalue.
enum class RValueType : u8 {
    /// References an lvalue to produce a value.
    UseLValue,

    /// References a local variable.
    UseLocal,

    /// Constant value (usually from the module constants pool, but may be inline).
    Constant,

    /// Function's outer closure context.
    OuterContext,

    /// Simple binary operation.
    BinaryOp,

    /// Simple unary operation.
    UnaryOp,

    /// Function call expression, i.e. `f()`.
    Call,

    /// Method call expression, i.e `a.b()`.
    MethodCall,

    /// Construct a container from the argument list,
    /// such as an array, a tuple or a map.
    Container,
};

std::string_view to_string(RValueType type);

/// Represents an rvalue.
/// RValues can appear as the right hand side of an assignment or definition.
///
/// RValues at this compilation stage to not allow inner control flow. Nested
/// language-level expressions that contain loops or conditionals are split up
/// so that only "simple" expressions remain.
class RValue final {
public:
// Format: type name, accessor name, field name.
#define TIRO_MIR_RVALUES(X)                        \
    X(UseLValue, use_lvalue, use_lvalue_)          \
    X(UseLocal, use_local, use_local_)             \
    X(Constant, constant, constant_)               \
    X(OuterContext, outer_context, outer_context_) \
    X(BinaryOp, binary_op, binary_op_)             \
    X(UnaryOp, unary_op, unary_op_)                \
    X(Call, call, call_)                           \
    X(MethodCall, method_call, method_call_)       \
    X(Container, container, container_)

    struct UseLValue {
        /// Dereferenced lvalue.
        LValue target;
    };

    struct UseLocal {
        LocalID target;
    };

    using Constant = mir::Constant;

    struct OuterContext {};

    struct BinaryOp {
        BinaryOpType op; ///< Operator.
        LocalID left;    ///< Left operand.
        LocalID right;   ///< Right operand.
    };

    struct UnaryOp {
        UnaryOpType op;  ///< Operator.
        LocalID operand; ///< Operand.
    };

    struct Call {
        /// Function to be called.
        LocalID function;

        /// List of function arguments.
        LocalListID arguments;
    };

    struct MethodCall {
        /// Object which method we're going to invoke.
        LocalID object;

        /// Name of the method to be called.
        InternedString method;

        /// List of method arguments.
        LocalListID arguments;
    };

    struct Container {
        /// Container type we're going to construct.
        ContainerType container;

        /// Arguments for the container constructor (list of elements,
        /// or list of key/value-pairs in the case of Map).
        LocalListID arguments;
    };

// Declare rvalue constructors.
#define TIRO_CONSTRUCTOR(type_name, accessor_name, field_name) \
    RValue(const type_name& arg)                               \
        : type_(RValueType::type_name)                         \
        , field_name(arg) {}

    TIRO_MIR_RVALUES(TIRO_CONSTRUCTOR)

#undef TIRO_CONSTRUCTOR

    RValueType type() const noexcept { return type_; }

// Declare rvalue accessors.
#define TIRO_ACCESSOR(type_name, accessor_name, field_name) \
    const type_name& accessor_name() const {                \
        check_access(RValueType::type_name);                \
        return field_name;                                  \
    }

    TIRO_MIR_RVALUES(TIRO_ACCESSOR)

#undef TIRO_ACCESSOR

    void format(FormatStream& stream) const;

private:
    void check_access(RValueType type) const;

private:
    RValueType type_;
    union {
#define TIRO_MEMBER(type_name, accessor_name, field_name) type_name field_name;
        TIRO_MIR_RVALUES(TIRO_MEMBER)
#undef TIRO_MEMBER
    };
};

template<typename Visitor>
inline decltype(auto) visit(const RValue& value, Visitor&& visitor);

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

/// Represents the type of a statement.
enum class StmtType : u8 {
    Assign,
    Define,
    SetReturn,
    EnterScope,
    ExitScope,
};

std::string to_string(StmtType type);

/// Represents a statement, i.e. a single instruction inside a basic block.
class Stmt final {
public:
// Format: type name, accessor name, field name.
#define TIRO_MIR_STMTS(X)                    \
    X(Assign, assign, assign_)               \
    X(Define, define, define_)               \
    X(SetReturn, set_return, set_return_)    \
    X(EnterScope, enter_scope, enter_scope_) \
    X(ExitScope, exit_scope, exit_scope_)

    struct Assign {
        LValue target;
        RValue value;
    };

    struct Define {
        LocalID local;
    };

    struct SetReturn {
        LocalID value;
    };

    struct EnterScope {
        ScopeID scope;
    };

    struct ExitScope {
        ScopeID scope;
    };

#define TIRO_CONSTRUCTOR(type_name, accessor, field_name) \
    Stmt(const type_name& arg)                            \
        : type_(StmtType::type_name)                      \
        , field_name(arg) {}

    TIRO_MIR_STMTS(TIRO_CONSTRUCTOR)

#undef TIRO_CONSTRUCTOR

    StmtType type() const noexcept { return type_; }

#define TIRO_ACCESSOR(type_name, accessor, field_name) \
    const type_name& accessor() const {                \
        check_access(StmtType::type_name);             \
        return field_name;                             \
    }

    TIRO_MIR_STMTS(TIRO_ACCESSOR)

#undef TIRO_ACCESSOR

private:
    void check_access(StmtType expected) const {
        TIRO_ASSERT(type_ == expected,
            "Invalid access: Stmt is not of the requested type.");
    }

private:
    StmtType type_;
    union {
#define TIRO_MEMBER(type_name, accessor, field_name) type_name field_name;
        TIRO_MIR_STMTS(TIRO_MEMBER)
#undef TIRO_MEMBER
    };
};

template<typename Visitor>
inline decltype(auto) visit(const Stmt& stmt, Visitor&& visitor);

template<typename Visitor>
inline decltype(auto) visit(const Edge& edge, Visitor&& visitor) {
    switch (edge.type()) {
#define TIRO_CASE(type_name, accessor_name, field_name) \
case EdgeType::type_name:                               \
    return visitor(edge.accessor_name());

        TIRO_MIR_EDGES(TIRO_CASE)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid edge type.");
}

template<typename Visitor>
inline decltype(auto) visit(const LValue& value, Visitor&& visitor) {
    switch (value.type()) {
#define TIRO_CASE(type_name, accessor_name, field_name) \
case LValueType::type_name:                             \
    return visitor(value.accessor_name());

        TIRO_MIR_LVALUES(TIRO_CASE)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid lvalue type.");
}

template<typename Visitor>
inline decltype(auto) visit(const Constant& constant, Visitor&& visitor) {
    switch (constant.type()) {
#define TIRO_CASE(type_name, accessor_name, field_name) \
case ConstantType::type_name:                           \
    return visitor(constant.accessor_name());

        TIRO_MIR_CONSTANTS(TIRO_CASE)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid constant type.");
}

template<typename Visitor>
inline decltype(auto) visit(const RValue& value, Visitor&& visitor) {
    switch (value.type()) {
#define TIRO_CASE(type_name, accessor_name, field_name) \
case RValueType::type_name:                             \
    return visitor(value.accessor_name());

        TIRO_MIR_RVALUES(TIRO_CASE)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid rvalue type.");
}

template<typename Visitor>
inline decltype(auto) visit(const Stmt& stmt, Visitor&& visitor) {
    switch (stmt.type()) {
#define TIRO_CASE(type_name, accessor_name, field_name) \
case StmtType::type_name:                               \
    return visitor(stmt.accessor_name());

        TIRO_MIR_STMTS(TIRO_CASE)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid rvalue type.");
}

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

#endif // TIRO_MIR_TYPES_HPP
