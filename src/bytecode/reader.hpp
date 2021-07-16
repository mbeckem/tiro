#ifndef TIRO_BYTECODE_READER_HPP
#define TIRO_BYTECODE_READER_HPP

#include "bytecode/instruction.hpp"
#include "common/defs.hpp"
#include "common/memory/binary.hpp"

#include <string_view>
#include <variant>

namespace tiro {

enum class BytecodeReaderError : int {
    InvalidOpcode,
    IncompleteInstruction,
    End,
};

std::string_view message(BytecodeReaderError error);

/// Reads bytecode instructions from a byte span.
///
/// Note that the bytecode interpreter uses its own implementation
/// for performance reasons.
/// This class is useful as a general purpose instruction decoder.
class BytecodeReader {
public:
    explicit BytecodeReader(Span<const byte> bytecode);

    /// The byte offset of the next instruction start.
    size_t pos() const { return r_.pos(); }

    /// The number of remaining bytes.
    size_t remaining() const { return r_.remaining(); }

    /// The total number of bytes.
    size_t size() const { return r_.size(); }

    /// Decodes the next instruction.
    std::variant<BytecodeInstr, BytecodeReaderError> read();

private:
    CheckedBinaryReader r_;
};

} // namespace tiro

#endif // TIRO_BYTECODE_READER_HPP
