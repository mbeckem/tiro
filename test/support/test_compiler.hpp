#ifndef TIRO_TEST_TEST_COMPILER_HPP
#define TIRO_TEST_TEST_COMPILER_HPP

#include "compiler/bytecode/module.hpp"

#include <memory>
#include <string_view>

namespace tiro {

/// Compiles the given source code and returns a bytecode module.
std::unique_ptr<BytecodeModule> test_compile(std::string_view source);

} // namespace tiro

#endif // TIRO_TEST_TEST_COMPILER_HPP
