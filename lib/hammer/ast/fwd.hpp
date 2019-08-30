#ifndef HAMMER_AST_FWD_HPP
#define HAMMER_AST_FWD_HPP

#include "hammer/core/defs.hpp"

namespace hammer {

class StringTable;

} // namespace hammer

namespace hammer::ast {

enum class NodeKind : i8;

class Node;

// Forward declare all node types.
#define HAMMER_AST_TYPE(Type, Parent) class Type;
#include "hammer/ast/node_types.inc"

template<NodeKind kind>
struct NodeKindToType;

template<typename NodeType>
struct NodeTypeToKind;

template<typename NodeType>
struct NodeTypeToChildTypes;

class NodeFormatter;

enum class UnaryOperator : int;
enum class BinaryOperator : int;

} // namespace hammer::ast

#endif // HAMMER_AST_FWD_HPP
