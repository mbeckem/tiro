#ifndef TIRO_COMPILER_BYTECODE_GEN_BYTECODE_BUILDER_HPP
#define TIRO_COMPILER_BYTECODE_GEN_BYTECODE_BUILDER_HPP

#include "bytecode/fwd.hpp"
#include "bytecode/instruction.hpp"
#include "bytecode/module.hpp"
#include "common/adt/index_map.hpp"
#include "common/defs.hpp"
#include "common/hash.hpp"
#include "common/id_type.hpp"
#include "common/memory/binary.hpp"
#include "compiler/ir/entities.hpp"
#include "compiler/ir/fwd.hpp"

#include <type_traits>
#include <vector>

namespace tiro {

class BytecodeBuilder final {
public:
    explicit BytecodeBuilder(BytecodeFunction& output, size_t total_label_count);

    /// Returns an offset value that represents the given target block.
    /// The value used to emit jumps to the block, even before it has been defined.
    BytecodeOffset use_label(ir::BlockId label);

    /// Marks the start of the given block at the current position.
    /// Jumps that refer to that block will receive the correct location.
    void define_label(ir::BlockId label);

    /// Marks the current byte offset as the start of a section that has the given
    /// handler as its exception handler. Use an invalid BlockId to signal "no handler",
    /// which is also the starting value.
    void start_handler(ir::BlockId handler_label);

    /// Emit a single instruction. Jumps and module member accesses are tracked
    /// for later patching.
    void emit(const BytecodeInstr& ins);

    /// Complete bytecode construction. Call this after all instructions
    /// have been emitted. All required block labels must be defined
    /// when this function is called, because it will patch all label references.
    void finish();

    /// Returns the list of module references that have been emitted by the compilation process.
    std::vector<std::tuple<u32, BytecodeMemberId>> take_module_refs() {
        return std::move(module_refs_);
    }

private:
    void finish_handler();
    void simplify_handlers();

    template<typename... Args>
    void write(const Args&... args) {
        (write_impl(args), ...);
    }

    void write_impl(BytecodeOp op);
    void write_impl(BytecodeParam param);
    void write_impl(BytecodeRegister local);
    void write_impl(BytecodeOffset offset);
    void write_impl(BytecodeMemberId mod);
    void write_impl(u32 value);
    void write_impl(i64 value);
    void write_impl(f64 value);

    // Compile time error on implicit conversions:
    template<typename T>
    void write_impl(T) = delete;

    u32 pos() const;

private:
    std::vector<ExceptionHandler>& handlers_;
    BinaryWriter wr_;
    IndexMap<std::optional<u32>, IdMapper<BytecodeOffset>> labels_;
    std::vector<std::tuple<u32, BytecodeOffset>> label_refs_;
    std::vector<std::tuple<u32, BytecodeMemberId>> module_refs_;
    ir::BlockId handler_;
    u32 handler_start_ = 0;
};

} // namespace tiro

#endif // TIRO_COMPILER_BYTECODE_GEN_BYTECODE_BUILDER_HPP
