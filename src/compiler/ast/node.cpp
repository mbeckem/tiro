#include "compiler/ast/ast.hpp"

#include "common/safe_int.hpp"

#include <new>

namespace tiro {

std::string_view to_string(AstNodeType type) {
    switch (type) {
#define TIRO_CASE(Type)     \
    case AstNodeType::Type: \
        return #Type;

        /* [[[cog
            import cog
            from codegen.ast import walk_concrete_types
            names = sorted(node_type.name for node_type in walk_concrete_types())
            for name in names:
                cog.outl(f"TIRO_CASE({name})");
        ]]] */
        TIRO_CASE(ArrayLiteral)
        TIRO_CASE(AssertStmt)
        TIRO_CASE(BinaryExpr)
        TIRO_CASE(Binding)
        TIRO_CASE(BlockExpr)
        TIRO_CASE(BooleanLiteral)
        TIRO_CASE(BreakExpr)
        TIRO_CASE(CallExpr)
        TIRO_CASE(ContinueExpr)
        TIRO_CASE(DeclStmt)
        TIRO_CASE(ElementExpr)
        TIRO_CASE(EmptyStmt)
        TIRO_CASE(ExportModifier)
        TIRO_CASE(ExprStmt)
        TIRO_CASE(File)
        TIRO_CASE(FloatLiteral)
        TIRO_CASE(ForStmt)
        TIRO_CASE(FuncDecl)
        TIRO_CASE(FuncExpr)
        TIRO_CASE(IfExpr)
        TIRO_CASE(ImportDecl)
        TIRO_CASE(IntegerLiteral)
        TIRO_CASE(MapItem)
        TIRO_CASE(MapLiteral)
        TIRO_CASE(NullLiteral)
        TIRO_CASE(NumericIdentifier)
        TIRO_CASE(ParamDecl)
        TIRO_CASE(PropertyExpr)
        TIRO_CASE(ReturnExpr)
        TIRO_CASE(SetLiteral)
        TIRO_CASE(StringExpr)
        TIRO_CASE(StringGroupExpr)
        TIRO_CASE(StringIdentifier)
        TIRO_CASE(StringLiteral)
        TIRO_CASE(SymbolLiteral)
        TIRO_CASE(TupleBindingSpec)
        TIRO_CASE(TupleLiteral)
        TIRO_CASE(UnaryExpr)
        TIRO_CASE(VarBindingSpec)
        TIRO_CASE(VarDecl)
        TIRO_CASE(VarExpr)
        TIRO_CASE(WhileStmt)
        // [[[end]]]

#undef TIRO_CASE
    }
    TIRO_UNREACHABLE("Invalid node type.");
}

void format(AstNodeFlags flags, FormatStream& stream) {
    bool first = true;
    auto format_flag = [&](AstNodeFlags value, const char* name) {
        if ((flags & value) != AstNodeFlags::None) {
            if (first) {
                stream.format(", ");
                first = false;
            }
            stream.format("{}", name);
        }
    };

#define TIRO_FLAG(Name) (format_flag(AstNodeFlags::Name, #Name))

    stream.format("(");
    TIRO_FLAG(HasError);
    stream.format(")");

#undef TIRO_FLAG
}

AstNode::AstNode(AstNodeType type)
    : type_(type) {
    TIRO_DEBUG_ASSERT(
        type >= AstNodeType::FirstNode && type <= AstNodeType::LastNode, "Invalid node type.");
}

AstNode::~AstNode() = default;

void AstNode::do_traverse_children([[maybe_unused]] FunctionRef<void(AstNode*)> callback) {}

void AstNode::do_mutate_children([[maybe_unused]] MutableAstVisitor& visitor) {}

std::string_view to_string(AccessType access) {
    switch (access) {
    case AccessType::Normal:
        return "Normal";
    case AccessType::Optional:
        return "Optional";
    }
    TIRO_UNREACHABLE("Invalid access type.");
}

AstNodeMap::AstNodeMap() = default;

AstNodeMap::~AstNodeMap() = default;

AstNodeMap::AstNodeMap(AstNodeMap&&) noexcept = default;

AstNodeMap& AstNodeMap::operator=(AstNodeMap&&) noexcept = default;

void AstNodeMap::register_tree(AstNode* node) {
    if (!node)
        return;

    register_node(TIRO_NN(node));
    node->traverse_children([&](AstNode* child) { register_tree(child); });
}

void AstNodeMap::register_node(NotNull<AstNode*> node) {
    TIRO_DEBUG_ASSERT(node->id().valid(), "The node must have a valid id.");
    TIRO_DEBUG_ASSERT(nodes_.count(node->id()) == 0, "The node's id must be unique.");

    nodes_.emplace(node->id(), node);
}

bool AstNodeMap::remove_node(AstId id) {
    if (auto pos = nodes_.find(id); pos != nodes_.end()) {
        nodes_.erase(pos);
        return true;
    }
    return false;
}

AstNode* AstNodeMap::find_node(AstId id) const {
    TIRO_DEBUG_ASSERT(id, "The node id must be valid.");
    if (auto pos = nodes_.find(id); pos != nodes_.end())
        return pos->second;
    return nullptr;
}

NotNull<AstNode*> AstNodeMap::get_node(AstId id) const {
    auto node = find_node(id);
    return TIRO_NN(node);
}

} // namespace tiro
