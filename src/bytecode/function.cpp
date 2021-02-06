#include "bytecode/function.hpp"

namespace tiro {

std::string_view to_string(BytecodeFunctionType type) {
    switch (type) {
    case BytecodeFunctionType::Normal:
        return "Normal";
    case BytecodeFunctionType::Closure:
        return "Closure";
    }

    TIRO_UNREACHABLE("Invalid function type.");
}

BytecodeFunction::BytecodeFunction() {}

BytecodeFunction::~BytecodeFunction() {}

BytecodeFunction::BytecodeFunction(BytecodeFunction&&) noexcept = default;

BytecodeFunction& BytecodeFunction::operator=(BytecodeFunction&&) noexcept = default;

} // namespace tiro
