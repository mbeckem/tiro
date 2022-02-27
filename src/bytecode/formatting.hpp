#ifndef TIRO_BYTECODE_FORMATTING_HPP
#define TIRO_BYTECODE_FORMATTING_HPP

#include "bytecode/fwd.hpp"
#include "common/fwd.hpp"

namespace tiro {

void format_record_schema(const BytecodeRecordSchema& tmpl, FormatStream& stream);
void format_function(const BytecodeFunction& func, FormatStream& stream);
void format_module(const BytecodeModule& module, FormatStream& stream);

} // namespace tiro

#endif // TIRO_BYTECODE_FORMATTING_HPP
