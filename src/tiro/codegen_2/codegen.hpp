#ifndef TIRO_CODEGEN_2_CODEGEN_HPP
#define TIRO_CODEGEN_2_CODEGEN_HPP

#include "tiro/bytecode/fwd.hpp"
#include "tiro/mir/fwd.hpp"

namespace tiro {

class CodeGenerator final {
public:
    explicit CodeGenerator();
    ~CodeGenerator();

    CodeGenerator(const CodeGenerator&) = delete;
    CodeGenerator& operator=(const CodeGenerator&) = delete;

private:
    const mir::Module& module_;
};

} // namespace tiro

#endif // TIRO_CODEGEN_2_CODEGEN_HPP
