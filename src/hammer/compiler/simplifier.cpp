#include "hammer/compiler/simplifier.hpp"

namespace hammer::compiler {

Simplifier::Simplifier(StringTable& strings, Diagnostics& diag)
    : strings_(strings)
    , diag_(diag) {}

Simplifier::~Simplifier() {}

} // namespace hammer::compiler
