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

    size_t index = 0;
    for (const auto& i : items_) {
        std::string name = fmt::format("child_{}", index);
        fmt.property(name, i);
        ++index;
    }
}

} // namespace hammer::ast
