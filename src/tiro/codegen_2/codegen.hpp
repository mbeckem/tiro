#ifndef TIRO_CODEGEN_2_CODEGEN_HPP
#define TIRO_CODEGEN_2_CODEGEN_HPP

#include "tiro/codegen_2/codegen.cpp"

#include "tiro/bytecode/fwd.hpp"
#include "tiro/mir/fwd.hpp"

namespace tiro::compiler {

class CodeGenerator final {
public:
    explicit CodeGenerator();
    ~CodeGenerator();

    CodeGenerator(const CodeGenerator&) = delete;
    CodeGenerator& operator=(const CodeGenerator&) = delete;

private:
    const mir::Module& module_;
};

} // namespace tiro::compiler

#endif // TIRO_CODEGEN_2_CODEGEN_HPP
