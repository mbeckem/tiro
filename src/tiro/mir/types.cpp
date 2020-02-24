#include "tiro/mir/types.hpp"

#include <type_traits>

namespace tiro::compiler::mir {

Function::Function(StringTable& strings) {
    entry_ = make(Block(strings.insert("entry")));
    exit_ = make(Block(strings.insert("exit")));

    (*this)[entry_]->edge(Edge::Return{});
}

Function::~Function() {}

BlockID Function::make(Block block) {
    return add_impl<BlockID>(blocks_, std::move(block));
}

ScopeID Function::make(Scope scope) {
    return add_impl<ScopeID>(scopes_, std::move(scope));
}

ParamID Function::make(Param param) {
    return add_impl<ParamID>(params_, std::move(param));
}

LocalID Function::make(Local local) {
    return add_impl<LocalID>(locals_, std::move(local));
}

LocalListID Function::make(LocalList local_list) {
    return add_impl<LocalListID>(local_lists_, std::move(local_list));
}

BlockID Function::entry() const {
    return entry_;
}

BlockID Function::exit() const {
    return exit_;
}

NotNull<VecPtr<Block>> Function::operator[](BlockID id) {
    TIRO_ASSERT(check_id(id, blocks_), "Invalid block id.");
    return TIRO_NN(VecPtr(blocks_, id.value()));
}

u32 Function::check_id_value(size_t vector_index) {
    return checked_cast<u32>(vector_index);
}

Param::Param(InternedString name)
    : name_(name) {
    TIRO_ASSERT(name, "Parameters must have valid names.");
}

InternedString Param::name() const {
    return name_;
}

void Param::format(FormatStream& stream) const {
    stream.format("Param({})", name());
}

/* [[[cog
    import mir
    codegen.implement_type(mir.EdgeType)
]]] */
std::string_view to_string(EdgeType type) {
    switch (type) {
    case EdgeType::None:
        return "None";
    case EdgeType::Jump:
        return "Jump";
    case EdgeType::Branch:
        return "Branch";
    case EdgeType::Return:
        return "Return";
    case EdgeType::AssertFail:
        return "AssertFail";
    case EdgeType::Never:
        return "Never";
    }
    TIRO_UNREACHABLE("Invalid EdgeType.");
}
// [[[end]]]

std::string_view to_string(BranchType type) {
    switch (type) {
#define TIRO_CASE(T)    \
    case BranchType::T: \
        return #T;

        TIRO_CASE(IfTrue)
        TIRO_CASE(IfFalse)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid branch type.");
}

/* [[[cog
    import mir
    codegen.implement_type(mir.Edge)
]]] */
Edge::Edge(const None& none)
    : type_(EdgeType::None)
    , none_(none) {}

Edge::Edge(const Jump& jump)
    : type_(EdgeType::Jump)
    , jump_(jump) {}

Edge::Edge(const Branch& branch)
    : type_(EdgeType::Branch)
    , branch_(branch) {}

Edge::Edge(const Return& ret)
    : type_(EdgeType::Return)
    , return_(ret) {}

Edge::Edge(const AssertFail& assert_fail)
    : type_(EdgeType::AssertFail)
    , assert_fail_(assert_fail) {}

Edge::Edge(const Never& never)
    : type_(EdgeType::Never)
    , never_(never) {}

const Edge::None& Edge::as_none() const {
    TIRO_ASSERT(
        type_ == EdgeType::None, "Bad member access on Edge: not a None.");
    return none_;
}

const Edge::Jump& Edge::as_jump() const {
    TIRO_ASSERT(
        type_ == EdgeType::Jump, "Bad member access on Edge: not a Jump.");
    return jump_;
}

const Edge::Branch& Edge::as_branch() const {
    TIRO_ASSERT(
        type_ == EdgeType::Branch, "Bad member access on Edge: not a Branch.");
    return branch_;
}

const Edge::Return& Edge::as_return() const {
    TIRO_ASSERT(
        type_ == EdgeType::Return, "Bad member access on Edge: not a Return.");
    return return_;
}

const Edge::AssertFail& Edge::as_assert_fail() const {
    TIRO_ASSERT(type_ == EdgeType::AssertFail,
        "Bad member access on Edge: not a AssertFail.");
    return assert_fail_;
}

const Edge::Never& Edge::as_never() const {
    TIRO_ASSERT(
        type_ == EdgeType::Never, "Bad member access on Edge: not a Never.");
    return never_;
}

void Edge::format(FormatStream& stream) const {
    struct Formatter {
        FormatStream& stream;

        void visit_none([[maybe_unused]] const None& none) {
            stream.format("None");
        }

        void visit_jump([[maybe_unused]] const Jump& jump) {
            stream.format("Jump(target: {})", jump.target);
        }

        void visit_branch([[maybe_unused]] const Branch& branch) {
            stream.format(
                "Branch(type: {}, value: {}, target: {}, fallthrough: {})",
                branch.type, branch.value, branch.target, branch.fallthrough);
        }

        void visit_return([[maybe_unused]] const Return& ret) {
            stream.format("Return");
        }

        void visit_assert_fail([[maybe_unused]] const AssertFail& assert_fail) {
            stream.format("AssertFail(message: {})", assert_fail.message);
        }

        void visit_never([[maybe_unused]] const Never& never) {
            stream.format("Never");
        }
    };

    Formatter formatter{stream};
    visit(formatter);
}
// [[[end]]]

Block::Block(InternedString label)
    : label_(label) {
    TIRO_ASSERT(label, "Basic blocks must have a valid label.");
}

Block::~Block() {}

void Block::append(const Stmt& stmt) {
    stmts_.push_back(stmt);
}

void Block::format(FormatStream& stream) const {
    stream.format("Block(label: {})");
}

void Scope::format(FormatStream& stream) const {
    stream.format("Scope(parent: {})", parent());
}

/* [[[cog
    import mir
    codegen.implement_type(mir.LValueType)
]]] */
std::string_view to_string(LValueType type) {
    switch (type) {
    case LValueType::Argument:
        return "Argument";
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
    import mir
    codegen.implement_type(mir.LValue)
]]] */
LValue::LValue(const Argument& argument)
    : type_(LValueType::Argument)
    , argument_(argument) {}

LValue::LValue(const Closure& closure)
    : type_(LValueType::Closure)
    , closure_(closure) {}

LValue::LValue(const Module& module)
    : type_(LValueType::Module)
    , module_(module) {}

LValue::LValue(const Field& field)
    : type_(LValueType::Field)
    , field_(field) {}

LValue::LValue(const TupleField& tuple_field)
    : type_(LValueType::TupleField)
    , tuple_field_(tuple_field) {}

LValue::LValue(const Index& index)
    : type_(LValueType::Index)
    , index_(index) {}

const LValue::Argument& LValue::as_argument() const {
    TIRO_ASSERT(type_ == LValueType::Argument,
        "Bad member access on LValue: not a Argument.");
    return argument_;
}

const LValue::Closure& LValue::as_closure() const {
    TIRO_ASSERT(type_ == LValueType::Closure,
        "Bad member access on LValue: not a Closure.");
    return closure_;
}

const LValue::Module& LValue::as_module() const {
    TIRO_ASSERT(type_ == LValueType::Module,
        "Bad member access on LValue: not a Module.");
    return module_;
}

const LValue::Field& LValue::as_field() const {
    TIRO_ASSERT(type_ == LValueType::Field,
        "Bad member access on LValue: not a Field.");
    return field_;
}

const LValue::TupleField& LValue::as_tuple_field() const {
    TIRO_ASSERT(type_ == LValueType::TupleField,
        "Bad member access on LValue: not a TupleField.");
    return tuple_field_;
}

const LValue::Index& LValue::as_index() const {
    TIRO_ASSERT(type_ == LValueType::Index,
        "Bad member access on LValue: not a Index.");
    return index_;
}

void LValue::format(FormatStream& stream) const {
    struct Formatter {
        FormatStream& stream;

        void visit_argument([[maybe_unused]] const Argument& argument) {
            stream.format("Argument(index: {})", argument.index);
        }

        void visit_closure([[maybe_unused]] const Closure& closure) {
            stream.format("Closure(context: {}, levels: {}, index: {})",
                closure.context, closure.levels, closure.index);
        }

        void visit_module([[maybe_unused]] const Module& module) {
            stream.format("Module(index: {})", module.index);
        }

        void visit_field([[maybe_unused]] const Field& field) {
            stream.format(
                "Field(object: {}, name: {})", field.object, field.name);
        }

        void visit_tuple_field([[maybe_unused]] const TupleField& tuple_field) {
            stream.format("TupleField(object: {}, index: {})",
                tuple_field.object, tuple_field.index);
        }

        void visit_index([[maybe_unused]] const Index& index) {
            stream.format(
                "Index(object: {}, index: {})", index.object, index.index);
        }
    };

    Formatter formatter{stream};
    visit(formatter);
}
// [[[end]]]

/* [[[cog
    import mir
    codegen.implement_type(mir.ConstantType)
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

/* [[[cog
    import mir
    codegen.implement_type(mir.Constant)
]]] */
Constant::Constant(const Integer& integer)
    : type_(ConstantType::Integer)
    , integer_(integer) {}

Constant::Constant(const Float& f)
    : type_(ConstantType::Float)
    , float_(f) {}

Constant::Constant(const String& string)
    : type_(ConstantType::String)
    , string_(string) {}

Constant::Constant(const Symbol& symbol)
    : type_(ConstantType::Symbol)
    , symbol_(symbol) {}

Constant::Constant(const Null& null)
    : type_(ConstantType::Null)
    , null_(null) {}

Constant::Constant(const True& t)
    : type_(ConstantType::True)
    , true_(t) {}

Constant::Constant(const False& f)
    : type_(ConstantType::False)
    , false_(f) {}

const Constant::Integer& Constant::as_integer() const {
    TIRO_ASSERT(type_ == ConstantType::Integer,
        "Bad member access on Constant: not a Integer.");
    return integer_;
}

const Constant::Float& Constant::as_float() const {
    TIRO_ASSERT(type_ == ConstantType::Float,
        "Bad member access on Constant: not a Float.");
    return float_;
}

const Constant::String& Constant::as_string() const {
    TIRO_ASSERT(type_ == ConstantType::String,
        "Bad member access on Constant: not a String.");
    return string_;
}

const Constant::Symbol& Constant::as_symbol() const {
    TIRO_ASSERT(type_ == ConstantType::Symbol,
        "Bad member access on Constant: not a Symbol.");
    return symbol_;
}

const Constant::Null& Constant::as_null() const {
    TIRO_ASSERT(type_ == ConstantType::Null,
        "Bad member access on Constant: not a Null.");
    return null_;
}

const Constant::True& Constant::as_true() const {
    TIRO_ASSERT(type_ == ConstantType::True,
        "Bad member access on Constant: not a True.");
    return true_;
}

const Constant::False& Constant::as_false() const {
    TIRO_ASSERT(type_ == ConstantType::False,
        "Bad member access on Constant: not a False.");
    return false_;
}

void Constant::format(FormatStream& stream) const {
    struct Formatter {
        FormatStream& stream;

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

        void visit_null([[maybe_unused]] const Null& null) {
            stream.format("Null");
        }

        void visit_true([[maybe_unused]] const True& t) {
            stream.format("True");
        }

        void visit_false([[maybe_unused]] const False& f) {
            stream.format("False");
        }
    };

    Formatter formatter{stream};
    visit(formatter);
}
// [[[end]]]

/* [[[cog
    import mir
    codegen.implement_type(mir.RValueType)
]]] */
std::string_view to_string(RValueType type) {
    switch (type) {
    case RValueType::UseLValue:
        return "UseLValue";
    case RValueType::UseLocal:
        return "UseLocal";
    case RValueType::Constant:
        return "Constant";
    case RValueType::OuterContext:
        return "OuterContext";
    case RValueType::BinaryOp:
        return "BinaryOp";
    case RValueType::UnaryOp:
        return "UnaryOp";
    case RValueType::Call:
        return "Call";
    case RValueType::MethodCall:
        return "MethodCall";
    case RValueType::Container:
        return "Container";
    }
    TIRO_UNREACHABLE("Invalid RValueType.");
}
// [[[end]]]

/* [[[cog
    import mir
    codegen.implement_type(mir.RValue)
]]] */
RValue::RValue(const UseLValue& use_lvalue)
    : type_(RValueType::UseLValue)
    , use_lvalue_(use_lvalue) {}

RValue::RValue(const UseLocal& use_local)
    : type_(RValueType::UseLocal)
    , use_local_(use_local) {}

RValue::RValue(const Constant& constant)
    : type_(RValueType::Constant)
    , constant_(constant) {}

RValue::RValue(const OuterContext& outer_context)
    : type_(RValueType::OuterContext)
    , outer_context_(outer_context) {}

RValue::RValue(const BinaryOp& binary_op)
    : type_(RValueType::BinaryOp)
    , binary_op_(binary_op) {}

RValue::RValue(const UnaryOp& unary_op)
    : type_(RValueType::UnaryOp)
    , unary_op_(unary_op) {}

RValue::RValue(const Call& call)
    : type_(RValueType::Call)
    , call_(call) {}

RValue::RValue(const MethodCall& method_call)
    : type_(RValueType::MethodCall)
    , method_call_(method_call) {}

RValue::RValue(const Container& container)
    : type_(RValueType::Container)
    , container_(container) {}

const RValue::UseLValue& RValue::as_use_lvalue() const {
    TIRO_ASSERT(type_ == RValueType::UseLValue,
        "Bad member access on RValue: not a UseLValue.");
    return use_lvalue_;
}

const RValue::UseLocal& RValue::as_use_local() const {
    TIRO_ASSERT(type_ == RValueType::UseLocal,
        "Bad member access on RValue: not a UseLocal.");
    return use_local_;
}

const RValue::Constant& RValue::as_constant() const {
    TIRO_ASSERT(type_ == RValueType::Constant,
        "Bad member access on RValue: not a Constant.");
    return constant_;
}

const RValue::OuterContext& RValue::as_outer_context() const {
    TIRO_ASSERT(type_ == RValueType::OuterContext,
        "Bad member access on RValue: not a OuterContext.");
    return outer_context_;
}

const RValue::BinaryOp& RValue::as_binary_op() const {
    TIRO_ASSERT(type_ == RValueType::BinaryOp,
        "Bad member access on RValue: not a BinaryOp.");
    return binary_op_;
}

const RValue::UnaryOp& RValue::as_unary_op() const {
    TIRO_ASSERT(type_ == RValueType::UnaryOp,
        "Bad member access on RValue: not a UnaryOp.");
    return unary_op_;
}

const RValue::Call& RValue::as_call() const {
    TIRO_ASSERT(
        type_ == RValueType::Call, "Bad member access on RValue: not a Call.");
    return call_;
}

const RValue::MethodCall& RValue::as_method_call() const {
    TIRO_ASSERT(type_ == RValueType::MethodCall,
        "Bad member access on RValue: not a MethodCall.");
    return method_call_;
}

const RValue::Container& RValue::as_container() const {
    TIRO_ASSERT(type_ == RValueType::Container,
        "Bad member access on RValue: not a Container.");
    return container_;
}

void RValue::format(FormatStream& stream) const {
    struct Formatter {
        FormatStream& stream;

        void visit_use_lvalue([[maybe_unused]] const UseLValue& use_lvalue) {
            stream.format("UseLValue(target: {})", use_lvalue.target);
        }

        void visit_use_local([[maybe_unused]] const UseLocal& use_local) {
            stream.format("UseLocal(target: {})", use_local.target);
        }

        void visit_constant([[maybe_unused]] const Constant& constant) {}

        void visit_outer_context(
            [[maybe_unused]] const OuterContext& outer_context) {
            stream.format("OuterContext");
        }

        void visit_binary_op([[maybe_unused]] const BinaryOp& binary_op) {
            stream.format("BinaryOp(op: {}, left: {}, right: {})", binary_op.op,
                binary_op.left, binary_op.right);
        }

        void visit_unary_op([[maybe_unused]] const UnaryOp& unary_op) {
            stream.format(
                "UnaryOp(op: {}, operand: {})", unary_op.op, unary_op.operand);
        }

        void visit_call([[maybe_unused]] const Call& call) {
            stream.format("Call(func: {}, args: {})", call.func, call.args);
        }

        void visit_method_call([[maybe_unused]] const MethodCall& method_call) {
            stream.format("MethodCall(object: {}, method: {}, args: {})",
                method_call.object, method_call.method, method_call.args);
        }

        void visit_container([[maybe_unused]] const Container& container) {
            stream.format("Container(container: {}, args: {})",
                container.container, container.args);
        }
    };

    Formatter formatter{stream};
    visit(formatter);
}
// [[[end]]]

std::string to_string(LocalType type) {
    switch (type) {
#define TIRO_CASE(T)   \
    case LocalType::T: \
        return #T;

        TIRO_CASE(Declared)
        TIRO_CASE(Temp)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid local type.");
}

Local Local::declared(InternedString name, ScopeID scope, const RValue& value) {
    TIRO_ASSERT(name, "Declared local variables must have valid names.");
    Local local(value);
    local.type_ = LocalType::Declared;
    local.name_ = name;
    local.scope_ = scope;
    return local;
}

Local Local::temp(ScopeID scope, const RValue& value) {
    Local local(value);
    local.type_ = LocalType::Temp;
    local.scope_ = scope;
    return local;
}

Local::Local(const RValue& value)
    : value_(value) {}

void Local::format(FormatStream& stream) const {
    switch (type()) {
    case LocalType::Declared:
        stream.format("Local(type: {}, name: {}, scope: {}, value: {})",
            LocalType::Declared, name(), scope(), value());
        return;
    case LocalType::Temp:
        stream.format("Local(type: {}, scope: {}, value: {})", LocalType::Temp,
            scope(), value());
        return;
    }

    TIRO_UNREACHABLE("Invalid local type.");
}

LocalList::LocalList() {}

LocalList::LocalList(std::initializer_list<LocalID> locals)
    : locals_(locals) {}

LocalList::~LocalList() {}

std::string_view to_string(BinaryOpType type) {
    switch (type) {
#define TIRO_CASE(X)      \
    case BinaryOpType::X: \
        return #X;

        TIRO_CASE(Plus)
        TIRO_CASE(Minus)
        TIRO_CASE(Multiply)
        TIRO_CASE(Divide)
        TIRO_CASE(Modulus)
        TIRO_CASE(Power)
        TIRO_CASE(LeftShift)
        TIRO_CASE(RightShift)
        TIRO_CASE(BitwiseAnd)
        TIRO_CASE(BitwiseOr)
        TIRO_CASE(BitwiseXor)
        TIRO_CASE(Less)
        TIRO_CASE(LessEquals)
        TIRO_CASE(Greater)
        TIRO_CASE(GreaterEquals)
        TIRO_CASE(Equals)
        TIRO_CASE(NotEquals)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid binary operation type.");
}

std::string_view to_string(UnaryOpType type) {
    switch (type) {
#define TIRO_CASE(X)     \
    case UnaryOpType::X: \
        return #X;

        TIRO_CASE(Plus)
        TIRO_CASE(Minus)
        TIRO_CASE(BitwiseNot)
        TIRO_CASE(LogicalNot)

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
    import mir
    codegen.implement_type(mir.StmtType)
]]] */
std::string_view to_string(StmtType type) {
    switch (type) {
    case StmtType::Assign:
        return "Assign";
    case StmtType::Define:
        return "Define";
    case StmtType::SetReturn:
        return "SetReturn";
    case StmtType::EnterScope:
        return "EnterScope";
    case StmtType::ExitScope:
        return "ExitScope";
    }
    TIRO_UNREACHABLE("Invalid StmtType.");
}
// [[[end]]]

/* [[[cog
    import mir
    codegen.implement_type(mir.Stmt)
]]] */
Stmt::Stmt(const Assign& assign)
    : type_(StmtType::Assign)
    , assign_(assign) {}

Stmt::Stmt(const Define& define)
    : type_(StmtType::Define)
    , define_(define) {}

Stmt::Stmt(const SetReturn& set_return)
    : type_(StmtType::SetReturn)
    , set_return_(set_return) {}

Stmt::Stmt(const EnterScope& enter_scope)
    : type_(StmtType::EnterScope)
    , enter_scope_(enter_scope) {}

Stmt::Stmt(const ExitScope& exit_scope)
    : type_(StmtType::ExitScope)
    , exit_scope_(exit_scope) {}

const Stmt::Assign& Stmt::as_assign() const {
    TIRO_ASSERT(
        type_ == StmtType::Assign, "Bad member access on Stmt: not a Assign.");
    return assign_;
}

const Stmt::Define& Stmt::as_define() const {
    TIRO_ASSERT(
        type_ == StmtType::Define, "Bad member access on Stmt: not a Define.");
    return define_;
}

const Stmt::SetReturn& Stmt::as_set_return() const {
    TIRO_ASSERT(type_ == StmtType::SetReturn,
        "Bad member access on Stmt: not a SetReturn.");
    return set_return_;
}

const Stmt::EnterScope& Stmt::as_enter_scope() const {
    TIRO_ASSERT(type_ == StmtType::EnterScope,
        "Bad member access on Stmt: not a EnterScope.");
    return enter_scope_;
}

const Stmt::ExitScope& Stmt::as_exit_scope() const {
    TIRO_ASSERT(type_ == StmtType::ExitScope,
        "Bad member access on Stmt: not a ExitScope.");
    return exit_scope_;
}

void Stmt::format(FormatStream& stream) const {
    struct Formatter {
        FormatStream& stream;

        void visit_assign([[maybe_unused]] const Assign& assign) {
            stream.format(
                "Assign(target: {}, value: {})", assign.target, assign.value);
        }

        void visit_define([[maybe_unused]] const Define& define) {
            stream.format("Define(local: {})", define.local);
        }

        void visit_set_return([[maybe_unused]] const SetReturn& set_return) {
            stream.format("SetReturn(value: {})", set_return.value);
        }

        void visit_enter_scope([[maybe_unused]] const EnterScope& enter_scope) {
            stream.format("EnterScope(scope: {})", enter_scope.scope);
        }

        void visit_exit_scope([[maybe_unused]] const ExitScope& exit_scope) {
            stream.format("ExitScope(scope: {})", exit_scope.scope);
        }
    };

    Formatter formatter{stream};
    visit(formatter);
}
// [[[end]]]

// Check that the most frequently used types are trivial:
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
