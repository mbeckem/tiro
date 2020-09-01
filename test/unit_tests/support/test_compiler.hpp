#ifndef TIRO_TEST_TEST_COMPILER_HPP
#define TIRO_TEST_TEST_COMPILER_HPP

#include "compiler/compiler.hpp"

#include <memory>
#include <string_view>

namespace tiro {

/// Compiles the given source code and returns the full compiler result.
CompilerResult test_compile_result(std::string_view source);

/// Compiles the given source code and returns a bytecode module.
std::unique_ptr<BytecodeModule> test_compile(std::string_view source);

} // namespace tiro

#endif // TIRO_TEST_TEST_COMPILER_HPP
