#ifndef HAMMER_COMPILER_COMPILER_HPP
#define HAMMER_COMPILER_COMPILER_HPP

#include "hammer/ast/node.hpp"
#include "hammer/ast/root.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/output.hpp"

namespace hammer {

class Compiler {
public:
    explicit Compiler(std::string_view file_name, std::string_view file_content);

    const StringTable& strings() const { return strings_; }
    const Diagnostics& diag() const { return diag_; }

    const ast::Root& ast_root() const;

    void parse();
    void analyze();
    std::unique_ptr<CompiledModule> codegen();

private:
    std::string_view file_name_;
    std::string_view file_content_;

    StringTable strings_;
    Diagnostics diag_;

    // True if parsing completed. The ast may be (partially) invalid because of errors, but we can
    // still do analysis of the "good" parts.
    bool parsed_ = false;

    // True if analayze() was run. Codegen is possible if parse + analyze were executed
    // and if there were no errors reported in diag_.
    bool analyzed_ = false;

    // Set after parsing was done.
    std::unique_ptr<ast::Root> root_;
};

} // namespace hammer

#endif // HAMMER_COMPILER_COMPILER_HPP
