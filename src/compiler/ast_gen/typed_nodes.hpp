#ifndef TIRO_COMPILER_AST_GEN_TYPED_NODES_HPP
#define TIRO_COMPILER_AST_GEN_TYPED_NODES_HPP

#include "common/ranges/generator_range.hpp"
#include "compiler/ast_gen/scanner.hpp"
#include "compiler/syntax/syntax_tree.hpp"

namespace tiro {
namespace typed_syntax {

// Helper base class for stateful item iteration.
class Seq {
protected:
    Seq(const SyntaxNodeScanner& scanner)
        : scanner_(scanner) {}

    template<typename Generator>
    auto iterate(Generator&& gen) const {
        auto wrapper = [gen = std::forward<Generator>(gen), sc = scanner_]() mutable {
            return gen(sc);
        };
        return GeneratorRange(std::move(wrapper));
    }

private:
    SyntaxNodeScanner scanner_;
};

// Helper base class for nodes that are treated as a simple sequence of node children
class NodeSeq : Seq {
public:
    NodeSeq(const SyntaxNodeScanner& scanner)
        : Seq(scanner) {}

    auto items() const {
        return iterate([&](auto& scanner) { return scanner.search_node(); });
    }
};

struct Root {
    SyntaxNodeId item; // type varies depending on context (e.g. File)

    static std::optional<Root> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

// nodes are syntax items (e.g. ImportItem)
struct File : NodeSeq {
    using NodeSeq::NodeSeq;

    static std::optional<File> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct Condition {
    SyntaxNodeId expr; // expr

    static std::optional<Condition> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct ImportPath : Seq {
    using Seq::Seq;

    // Items are identifiers separated by "."
    auto path() {
        return iterate([&](auto& scanner) { return scanner.search_token(TokenType::Identifier); });
    }

    static std::optional<ImportPath> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct Modifiers : Seq {
    using Seq::Seq;

    // keyword tokens
    auto items() {
        return iterate([&](auto& scanner) { return scanner.search_token(); });
    }

    static std::optional<Modifiers> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct Name {
    Token value; // identifier

    static std::optional<Name> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

//
// Variable declarations
//

struct Var : Seq {
    Var(const std::optional<SyntaxNodeId>& modifiers_, const Token& decl_,
        const SyntaxNodeScanner& sc_)
        : Seq(sc_)
        , modifiers(modifiers_)
        , decl(decl_) {}

    std::optional<SyntaxNodeId> modifiers; // modifiers
    Token decl;                            // const or var keyword

    // items are bindings
    auto bindings() {
        return iterate([](auto& scanner) { return scanner.search_node(SyntaxType::Binding); });
    }

    static std::optional<Var> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct Binding {
    SyntaxNodeId spec;                // BindingName or BindingTuple
    std::optional<SyntaxNodeId> init; // expr

    static std::optional<Binding> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct BindingName {
    Token name; // identifier

    static std::optional<BindingName> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct BindingTuple : Seq {
    using Seq::Seq;

    // items are identifiers
    auto names() {
        return iterate([](auto& scanner) { return scanner.search_token(TokenType::Identifier); });
    }

    static std::optional<BindingTuple> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

//
// Functions
//
struct Func {
    std::optional<SyntaxNodeId> modifiers; // modifiers
    std::optional<SyntaxNodeId> name;      // name
    SyntaxNodeId params;                   // param list
    bool body_is_value;                    // true if "=" was present before body
    SyntaxNodeId body;                     // expr

    static std::optional<Func> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

// Items are expressions
struct ArgList : NodeSeq {
    ArgList(const Token& paren_, const SyntaxNodeScanner& sc_)
        : NodeSeq(sc_)
        , paren(paren_) {}

    Token paren; // ( or ?(

    static std::optional<ArgList> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct ParamList : Seq {
    using Seq::Seq;

    // items are identifiers
    auto names() {
        return iterate([](auto& scanner) { return scanner.search_token(TokenType::Identifier); });
    }

    static std::optional<ParamList> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

//
// Expressions
//

struct VarExpr {
    Token identifier;

    static std::optional<VarExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct Literal {
    Token value;

    static std::optional<Literal> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct GroupedExpr {
    SyntaxNodeId expr;

    static std::optional<GroupedExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct ReturnExpr {
    std::optional<SyntaxNodeId> value;

    static std::optional<ReturnExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct FieldExpr {
    SyntaxNodeId instance;
    Token access; // . or ?.
    Token field;

    static std::optional<FieldExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct TupleFieldExpr {
    SyntaxNodeId instance;
    Token access; // . or ?.
    Token field;

    static std::optional<TupleFieldExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct IndexExpr {
    SyntaxNodeId instance;
    Token bracket; // [ or ?.
    SyntaxNodeId index;

    static std::optional<IndexExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct BinaryExpr {
    SyntaxNodeId lhs;
    Token op;
    SyntaxNodeId rhs;

    static std::optional<BinaryExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct UnaryExpr {
    Token op;
    SyntaxNodeId expr;

    static std::optional<UnaryExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

// Nodes are expressions
struct TupleExpr : NodeSeq {
    using NodeSeq::NodeSeq;

    static std::optional<TupleExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

// Nodes are record items
struct RecordExpr : NodeSeq {
    using NodeSeq::NodeSeq;

    static std::optional<RecordExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct RecordItem {
    SyntaxNodeId name;  // Name node
    SyntaxNodeId value; // expr

    static std::optional<RecordItem> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

// Nodes are expressions
struct ArrayExpr : NodeSeq {
    using NodeSeq::NodeSeq;

    static std::optional<ArrayExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

// Nodes are expressions
struct SetExpr : NodeSeq {
    using NodeSeq::NodeSeq;

    static std::optional<SetExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

// Nodes are map items
struct MapExpr : NodeSeq {
    using NodeSeq::NodeSeq;

    static std::optional<MapExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct MapItem {
    SyntaxNodeId key;   // expr
    SyntaxNodeId value; // expr

    static std::optional<MapItem> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

// Items are StringContent tokens or StringFormatItem/StringFormatBlock nodes
struct StringExpr : Seq {
    using Seq::Seq;

    auto items() const { return iterate(find_next_string_item); }

    static std::optional<StringExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);

private:
    static std::optional<SyntaxChild> find_next_string_item(SyntaxNodeScanner& scanner) {
        return scanner.search([&](const SyntaxChild& child) {
            return scanner.is_token_type(child, TokenType::StringContent) || scanner.is_node(child);
        });
    }
};

struct StringFormatItem {
    SyntaxNodeId expr; // expr

    static std::optional<StringFormatItem> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct StringFormatBlock {
    SyntaxNodeId expr; // expr

    static std::optional<StringFormatBlock> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

// Items are string expressions
struct StringGroup : NodeSeq {
    using NodeSeq::NodeSeq;

    static std::optional<StringGroup> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct IfExpr {
    SyntaxNodeId cond;
    SyntaxNodeId then_branch;
    std::optional<SyntaxNodeId> else_branch;

    static std::optional<IfExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

// Items are statements
struct BlockExpr : NodeSeq {
    using NodeSeq::NodeSeq;

    static std::optional<BlockExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct FuncExpr {
    SyntaxNodeId func; // func

    static std::optional<FuncExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct CallExpr {
    SyntaxNodeId func; // expr
    SyntaxNodeId args; // arglist

    static std::optional<CallExpr> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

//
// Statements
//

struct ExprStmt {
    SyntaxNodeId expr; // expr

    static std::optional<ExprStmt> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct DeferStmt {
    SyntaxNodeId expr; // expr

    static std::optional<DeferStmt> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct AssertStmt {
    SyntaxNodeId args; // arglist

    static std::optional<AssertStmt> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct VarStmt {
    SyntaxNodeId var; // var decl

    static std::optional<VarStmt> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct WhileStmt {
    SyntaxNodeId cond; // condition
    SyntaxNodeId body; // expr

    static std::optional<WhileStmt> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct ForStmt {
    SyntaxNodeId header; // for stmt header
    SyntaxNodeId body;   // expr

    static std::optional<ForStmt> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct ForStmtHeader {
    std::optional<SyntaxNodeId> decl; // var decl
    std::optional<SyntaxNodeId> cond; // cond
    std::optional<SyntaxNodeId> step; // expr

    static std::optional<ForStmtHeader> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct ForEachStmt {
    SyntaxNodeId header; // for each stmt header
    SyntaxNodeId body;   // expr

    static std::optional<ForEachStmt> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct ForEachStmtHeader {
    SyntaxNodeId spec; // binding name or binding tuple
    SyntaxNodeId expr; // init expr

    static std::optional<ForEachStmtHeader> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

//
// Items
//

struct FuncItem {
    SyntaxNodeId func; // func

    static std::optional<FuncItem> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct VarItem {
    SyntaxNodeId var; // var decl

    static std::optional<VarItem> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

struct ImportItem {
    SyntaxNodeId path; // import path
    std::optional<Token> alias;

    static std::optional<ImportItem> read(SyntaxNodeId node_id, const SyntaxTree& tree);
};

template<SyntaxType st>
struct SyntaxTypeToNodeType {
    using type = void;
};

#define TIRO_REGISTER_NODE(syntax_type)                    \
    template<>                                             \
    struct SyntaxTypeToNodeType<SyntaxType::syntax_type> { \
        using type = syntax_type;                          \
    };

TIRO_REGISTER_NODE(Root)
TIRO_REGISTER_NODE(File)
TIRO_REGISTER_NODE(Name)
TIRO_REGISTER_NODE(Condition)
TIRO_REGISTER_NODE(ImportPath)
TIRO_REGISTER_NODE(Modifiers)
TIRO_REGISTER_NODE(RecordItem)
TIRO_REGISTER_NODE(MapItem)
TIRO_REGISTER_NODE(Var)
TIRO_REGISTER_NODE(Binding)
TIRO_REGISTER_NODE(BindingName)
TIRO_REGISTER_NODE(BindingTuple)
TIRO_REGISTER_NODE(Func)
TIRO_REGISTER_NODE(ArgList)
TIRO_REGISTER_NODE(ParamList)
TIRO_REGISTER_NODE(Literal)
TIRO_REGISTER_NODE(ReturnExpr)
TIRO_REGISTER_NODE(VarExpr)
TIRO_REGISTER_NODE(UnaryExpr)
TIRO_REGISTER_NODE(BinaryExpr)
TIRO_REGISTER_NODE(FieldExpr)
TIRO_REGISTER_NODE(TupleFieldExpr)
TIRO_REGISTER_NODE(IndexExpr)
TIRO_REGISTER_NODE(CallExpr)
TIRO_REGISTER_NODE(GroupedExpr)
TIRO_REGISTER_NODE(TupleExpr)
TIRO_REGISTER_NODE(RecordExpr)
TIRO_REGISTER_NODE(ArrayExpr)
TIRO_REGISTER_NODE(SetExpr)
TIRO_REGISTER_NODE(MapExpr)
TIRO_REGISTER_NODE(IfExpr)
TIRO_REGISTER_NODE(BlockExpr)
TIRO_REGISTER_NODE(FuncExpr)
TIRO_REGISTER_NODE(StringExpr)
TIRO_REGISTER_NODE(StringFormatItem)
TIRO_REGISTER_NODE(StringFormatBlock)
TIRO_REGISTER_NODE(StringGroup)
TIRO_REGISTER_NODE(DeferStmt)
TIRO_REGISTER_NODE(AssertStmt)
TIRO_REGISTER_NODE(ExprStmt)
TIRO_REGISTER_NODE(VarStmt)
TIRO_REGISTER_NODE(WhileStmt)
TIRO_REGISTER_NODE(ForStmt)
TIRO_REGISTER_NODE(ForStmtHeader)
TIRO_REGISTER_NODE(ForEachStmt)
TIRO_REGISTER_NODE(ForEachStmtHeader)
TIRO_REGISTER_NODE(ImportItem)
TIRO_REGISTER_NODE(VarItem)
TIRO_REGISTER_NODE(FuncItem)

#undef TIRO_REGISTER_NODE

} // namespace typed_syntax
} // namespace tiro

#endif // TIRO_COMPILER_AST_GEN_TYPED_NODES_HPP
