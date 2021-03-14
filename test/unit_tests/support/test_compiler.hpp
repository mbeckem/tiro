#ifndef TIRO_TEST_TEST_COMPILER_HPP
#define TIRO_TEST_TEST_COMPILER_HPP

#include "compiler/compiler.hpp"

#include <memory>
#include <string_view>

namespace tiro::test_support {

/// Compiles the given source code and returns the full compiler result.
CompilerResult compile_result(std::string_view source, std::string_view module_name = "test");

/// Compiles the given source code and returns a bytecode module.
std::unique_ptr<BytecodeModule>
compile(std::string_view source, std::string_view module_name = "test");

} // namespace tiro::test_support

#endif // TIRO_TEST_TEST_COMPILER_HPP
