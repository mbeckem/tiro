#ifndef TIRO_CODEGEN_CODE_BUILDER_HPP
#define TIRO_CODEGEN_CODE_BUILDER_HPP

#include "tiro/compiler/binary.hpp"
#include "tiro/compiler/opcodes.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/core/safe_int.hpp"

#include <unordered_map>
#include <vector>

namespace tiro {

class CodeBuilder;
class BasicBlock;

// Improvement: could also manage function constants in this class.
class CodeBuilder final {
public:
    /// Constructs a CodeBuilder that will append instructions at the end of the given vector.
    CodeBuilder(std::vector<byte>& out);

    /// Defines the block at the current location.
    void define_block(NotNull<BasicBlock*> block);

    /// Call this after all instructions and blocks have been emitted.
    /// This makes sure that all used blocks are defined and that their
    /// jump destinations are filled in correctly.
    void finish();

public:
    // -- Instructions --
    //
    // All functions here emit the appropriate instruction at the current location.

    void load_null();
    void load_false();
    void load_true();
    void load_int(i64 i);
    void load_float(f64 d);

    void load_param(u32 i);
    void store_param(u32 i);
    void load_local(u32 i);
    void store_local(u32 i);
    void load_closure();
    void load_context(u32 n, u32 i);
    void store_context(u32 n, u32 i);

    void load_member(u32 i);
    void store_member(u32 i);
    void load_tuple_member(u32 i);
    void store_tuple_member(u32 i);
    void load_index();
    void store_index();
    void load_module(u32 i);
    void store_module(u32 i);
    void load_global(u32 i);

    void dup();
    void pop();
    void pop_n(u32 n);
    void rot_2();
    void rot_3();
    void rot_4();

    void add();
    void sub();
    void mul();
    void div();
    void mod();
    void pow();
    void lnot();
    void bnot();
    void upos();
    void uneg();

    void lsh();
    void rsh();
    void band();
    void bor();
    void bxor();

    void gt();
    void gte();
    void lt();
    void lte();
    void eq();
    void neq();

    void mk_array(u32 n);
    void mk_tuple(u32 n);
    void mk_set(u32 n);
    void mk_map(u32 n);
    void mk_context(u32 n);
    void mk_closure();

    void mk_builder();
    void builder_append();
    void builder_string();

    void jmp(BasicBlock* target);
    void jmp_true(BasicBlock* target);
    void jmp_true_pop(BasicBlock* target);
    void jmp_false(BasicBlock* target);
    void jmp_false_pop(BasicBlock* target);

    void call(u32 n);
    void load_method(u32 i);
    void call_method(u32 n);
    void ret();

    void assert_fail();

private:
    void emit_offset(BasicBlock* block);
    void emit_op(Opcode op);

private:
    BinaryWriter w_;
    // SafeInt<u32> balance_ = 0; // Number of values on the stack

    // Blocks that have been declared.
    std::unordered_map<BasicBlock*, u32> blocks_;

    // Blocks that have been used but not yet defined. The index points
    // to the location that must be overwritten with the blocks's real
    // jump destination (when defined).
    std::vector<std::pair<u32, BasicBlock*>> block_uses_;
};

} // namespace tiro

#endif // TIRO_CODEGEN_CODE_BUILDER_HPP
