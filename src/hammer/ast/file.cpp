#include "hammer/ast/file.hpp"

#include "hammer/ast/node_formatter.hpp"
#include "hammer/ast/scope.hpp"

#include <fmt/format.h>

namespace hammer::ast {

File::File()
    : Node(NodeKind::File)
    , Scope(ScopeKind::FileScope) {}

File::~File() {}

void File::dump_impl(NodeFormatter& fmt) const {
    Node::dump_impl(fmt);
    fmt.properties("name", file_name());

    for (size_t i = 0; i < items_.size(); ++i) {
        std::string name = fmt::format("child_{}", i);
        fmt.property(name, i);
    }
}

} // namespace hammer::ast
