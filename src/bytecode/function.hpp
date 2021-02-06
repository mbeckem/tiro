#ifndef TIRO_BYTECODE_FUNCTION_HPP
#define TIRO_BYTECODE_FUNCTION_HPP

#include "bytecode/entities.hpp"
#include "bytecode/fwd.hpp"
#include "bytecode/op.hpp"
#include "common/adt/span.hpp"
#include "common/defs.hpp"

#include <vector>

namespace tiro {

enum class BytecodeFunctionType : u8 {
    Normal,  // Normal function
    Closure, // Function requires closure environment
};

std::string_view to_string(BytecodeFunctionType type);

// Represents an entry in the exception handler table of a function.
struct ExceptionHandler final {
    // Start byte offset into the function's code (inclusive).
    BytecodeOffset from;

    // End byte offset into the function's code (exclusive).
    BytecodeOffset to;

    // Jump destination (byte offset of exception handler start).
    BytecodeOffset target;

    ExceptionHandler() = default;

    ExceptionHandler(BytecodeOffset from_, BytecodeOffset to_, BytecodeOffset target_)
        : from(from_)
        , to(to_)
        , target(target_) {}
};

// Represents a function that has been compiled to bytecode.
class BytecodeFunction final {
public:
    BytecodeFunction();
    ~BytecodeFunction();

    BytecodeFunction(BytecodeFunction&&) noexcept;
    BytecodeFunction& operator=(BytecodeFunction&&) noexcept;

    // Name can be invalid for anonymous entries.
    BytecodeMemberId name() const { return name_; }
    void name(BytecodeMemberId value) { name_ = value; }

    BytecodeFunctionType type() const { return type_; }
    void type(BytecodeFunctionType t) { type_ = t; }

    u32 params() const { return params_; }
    void params(u32 count) { params_ = count; }

    u32 locals() const { return locals_; }
    void locals(u32 count) { locals_ = count; }

    std::vector<byte>& code() { return code_; }
    Span<const byte> code() const { return code_; }

    std::vector<ExceptionHandler>& handlers() { return handlers_; }
    Span<const ExceptionHandler> handlers() const { return handlers_; }

private:
    BytecodeMemberId name_;
    BytecodeFunctionType type_ = BytecodeFunctionType::Normal;
    u32 params_ = 0;
    u32 locals_ = 0;
    std::vector<byte> code_;
    std::vector<ExceptionHandler> handlers_;
};

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::BytecodeFunctionType)

#endif // TIRO_BYTECODE_FUNCTION_HPP
