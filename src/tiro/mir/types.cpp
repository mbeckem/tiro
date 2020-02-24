#include "tiro/mir/types.hpp"

#include <fmt/format.h>

#include <new>

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

std::string_view to_string(EdgeType type) {
    switch (type) {
#define TIRO_CASE(type_name, accessor_name, field_name) \
    case EdgeType::type_name:                           \
        return #type_name;

        TIRO_MIR_EDGES(TIRO_CASE)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid edge type.");
}

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

void Edge::format(FormatStream& stream) {
    stream.format("{}", to_string(type()));

    switch (type_) {
    case EdgeType::None:
        stream.format("None");
        break;
    case EdgeType::Jump:
        stream.format("Jump({})", jump().target);
        break;
    case EdgeType::Branch:
        stream.format("Branch(type: {}, value: {}, to: {}, fallthrough: {})",
            branch().type, branch().value, branch().target,
            branch().fallthrough);
        break;
    case EdgeType::Return:
        stream.format("Return");
        break;
    case EdgeType::AssertFail:
        stream.format("AssertFail(Message: {})", assert_fail().message);
        break;
    case EdgeType::Never:
        stream.format("Never");
        break;
    default:
        break;
    }
}

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

std::string_view to_string(LValueType type) {
    switch (type) {
#define TIRO_CASE(type_name, accessor_name, field_name) \
    case LValueType::type_name:                         \
        return #type_name;

        TIRO_MIR_LVALUES(TIRO_CASE)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid lvalue type.");
}

void LValue::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void operator()(const Argument& arg) {
            stream.format("Argument({})", arg.index);
        }

        void operator()(const Closure& cl) {
            stream.format(
                "Closure(levels: {}, index: {})", cl.levels, cl.index);
        }

        void operator()(const Module& mod) {
            stream.format("Module({})", mod.index);
        }

        void operator()(const Field& field) {
            stream.format(
                "Field(object: {}, field: {})", field.object, field.name);
        }

        void operator()(const TupleField& field) {
            stream.format(
                "TupleField(object: {}, field: {})", field.object, field.index);
        }

        void operator()(const Index& index) {
            stream.format(
                "Index(object: {}, index: {})", index.object, index.index);
        }
    } visitor{stream};
    visit(*this, visitor);
}

void LValue::check_access(LValueType type) const {
    TIRO_ASSERT(type_ == type, "Invalid LValue access: bad requested type.");
}

std::string_view to_string(ConstantType type) {
    switch (type) {
#define TIRO_CASE(type_name, accessor_name, field_name) \
    case ConstantType::type_name:                       \
        return #type_name;

        TIRO_MIR_CONSTANTS(TIRO_CASE)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid constant type.");
}

void Constant::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void operator()(const Integer& i) {
            stream.format("Integer({})", i.value);
        }

        void operator()(const Float& f) { stream.format("Float({})", f.value); }

        void operator()(const String& s) {
            stream.format("String({})", s.value);
        }

        void operator()(const Symbol& s) {
            stream.format("Symbol({})", s.value);
        }

        void operator()(const Null&) { stream.format("Null"); }

        void operator()(const True&) { stream.format("True"); }

        void operator()(const False&) { stream.format("False"); }

    } visitor{stream};
    visit(*this, visitor);
}

void Constant::check_access(ConstantType type) const {
    TIRO_ASSERT(type_ == type, "Invalid Constant access: bad requested type.");
}

std::string_view to_string(RValueType type) {
    switch (type) {
#define TIRO_CASE(type_name, accessor_name, field_name) \
    case RValueType::type_name:                         \
        return #type_name;

        TIRO_MIR_RVALUES(TIRO_CASE)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid rvalue type.");
}

void RValue::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void operator()(const UseLValue& lv) {
            stream.format("UseLValue(target: {})", lv.target);
        }

        void operator()(const UseLocal& loc) {
            stream.format("UseLocal(target: {})", loc.target);
        }

        void operator()(const Constant& c) { c.format(stream); }

        void operator()(const OuterContext&) { stream.format("OuterContext"); }

        void operator()(const BinaryOp& op) {
            stream.format("BinaryOp(op: {}, left: {}, right: {})", op.op,
                op.left, op.right);
        }

        void operator()(const UnaryOp& op) {
            stream.format("UnaryOp(op: {}, operand: {})", op.op, op.operand);
        }

        void operator()(const Call& call) {
            stream.format(
                "Call(function: {}, args: {})", call.function, call.arguments);
        }

        void operator()(const MethodCall& call) {
            stream.format("MethodCall(object: {}, method: {}, args: {})",
                call.object, call.method, call.arguments);
        }

        void operator()(const Container& cont) {
            stream.format("Container(type: {}, args: {})", cont.container,
                cont.arguments);
        }

    } visitor{stream};
    visit(*this, visitor);
}

void RValue::check_access(RValueType type) const {
    TIRO_ASSERT(type_ == type, "Invalid RValue access: bad requested type.");
}

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

#define TIRO_CHECK_TRIVIAL(type_name)                       \
    static_assert(std::is_trivially_copyable_v<type_name>); \
    static_assert(std::is_trivially_destructible_v<type_name>);

TIRO_CHECK_TRIVIAL(Param)
TIRO_CHECK_TRIVIAL(Local)

#define TIRO_CHECK_EDGES(type_name, _1, _2) TIRO_CHECK_TRIVIAL(Edge::type_name)
TIRO_MIR_EDGES(TIRO_CHECK_EDGES)
TIRO_CHECK_TRIVIAL(Edge)
#undef TIRO_CHECK_EDGES

#define TIRO_CHECK_LVALUES(type_name, _1, _2) \
    TIRO_CHECK_TRIVIAL(LValue::type_name)
TIRO_MIR_LVALUES(TIRO_CHECK_LVALUES)
TIRO_CHECK_TRIVIAL(LValue);
#undef TIRO_CHECK_LVALUES

#define TIRO_CHECK_CONSTANTS(type_name, _1, _2) \
    TIRO_CHECK_TRIVIAL(Constant::type_name)
TIRO_MIR_CONSTANTS(TIRO_CHECK_CONSTANTS)
TIRO_CHECK_TRIVIAL(Constant)
#undef TIRO_CHECK_CONSTANTS

#define TIRO_CHECK_RVALUES(type_name, _1, _2) \
    TIRO_CHECK_TRIVIAL(RValue::type_name)
TIRO_MIR_RVALUES(TIRO_CHECK_RVALUES)
TIRO_CHECK_TRIVIAL(RValue);
#undef TIRO_CHECK_RVALUES

#define TIRO_CHECK_STMTS(type_name, _1, _2) TIRO_CHECK_TRIVIAL(Stmt::type_name)
TIRO_MIR_STMTS(TIRO_CHECK_STMTS)
TIRO_CHECK_TRIVIAL(Stmt)
#undef TIRO_CHECK_STMTS

#undef TIRO_CHECK_TRIVIAL

} // namespace tiro::compiler::mir
