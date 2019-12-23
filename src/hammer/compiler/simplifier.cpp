#include "hammer/compiler/simplifier.hpp"

#include "hammer/ast/expr.hpp"
#include "hammer/ast/literal.hpp"
#include "hammer/ast/root.hpp"
#include "hammer/core/defs.hpp"

namespace hammer {

Simplifier::Simplifier(StringTable& strings, Diagnostics& diag)
    : strings_(strings)
    , diag_(diag) {}

Simplifier::~Simplifier() {}

void Simplifier::simplify([[maybe_unused]] ast::Root* root) {
    HAMMER_ASSERT_NOT_NULL(root);
    HAMMER_ASSERT(root->child(), "Root must have a child.");
}

} // namespace hammer
