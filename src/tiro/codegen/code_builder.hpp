#ifndef TIRO_CODEGEN_CODE_BUILDER_HPP
#define TIRO_CODEGEN_CODE_BUILDER_HPP

#include "tiro/compiler/binary.hpp"
#include "tiro/compiler/opcodes.hpp"
#include "tiro/core/defs.hpp"

#include <vector>

namespace tiro::compiler {

class CodeBuilder;
class LabelID;
class LabelGroup;

class LabelID final {
public:
    LabelID()
        : value_(u32(-1)) {}

    explicit operator bool() const { return value_ != u32(-1); }

private:
    friend CodeBuilder;

    LabelID(u32 value)
        : value_(value) {}

    u32 value_;
};

class LabelGroup final {
public:
    LabelGroup(CodeBuilder& builder);

    // Generates a new label within this label group.
    // All labels of a group share a common suffix that makes
    // them unique.
    LabelID gen(std::string_view name);

private:
    CodeBuilder* builder_;
    u32 unique_;
};

// Improvement: could also manage function constants in this class.
class CodeBuilder final {
public:
    /// Constructs a CodeBuilder that will append instructions at the end of the given vector.
    CodeBuilder(std::vector<byte>& out);

    /// Defines the label at the current position of the builder.
    void define_label(LabelID label);

    /// Call this after all instructions and labels have been emitted.
    /// This makes sure that all used labels are defined and that their
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

    void jmp(LabelID target);
    void jmp_true(LabelID target);
    void jmp_true_pop(LabelID target);
    void jmp_false(LabelID target);
    void jmp_false_pop(LabelID target);

    void call(u32 n);
    void load_method(u32 i);
    void call_method(u32 n);
    void ret();

    void assert_fail();

private:
    friend LabelGroup;

    struct LabelDef {
        std::string name;
        u32 location = u32(-1); // -1 == undefined
    };

    u32 next_unique();
    LabelID create_label(std::string name);
    void check_label(LabelID id) const;
    void emit_offset(LabelID label);
    void emit_op(Opcode op);

private:
    BinaryWriter w_;
    u32 next_unique_ = 1;

    // Labels that have been declared.
    std::vector<LabelDef> labels_;

    // Labels that have been used. The index points
    // to the location that must be overwritten with the label's real
    // jump destination (when defined).
    std::vector<std::pair<u32, LabelID>> label_uses_;
};

} // namespace tiro::compiler

#endif // TIRO_CODEGEN_CODE_BUILDER_HPP
