#ifndef HAMMER_COMPILER_CODE_BUILDER_HPP
#define HAMMER_COMPILER_CODE_BUILDER_HPP

#include "hammer/core/defs.hpp"
#include "hammer/compiler/binary.hpp"
#include "hammer/compiler/opcodes.hpp"

#include <vector>

namespace hammer {

class CodeBuilder;
class LabelID;
class LabelGroup;

class LabelID {
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

class LabelGroup {
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

// TODO could also manage function constants in this class.
class CodeBuilder {
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
    void load_float(double d);

    void load_const(u32 i);
    void load_param(u32 i);
    void store_param(u32 i);
    void load_local(u32 i);
    void store_local(u32 i);
    void load_env(u32 n, u32 i);
    void store_env(u32 n, u32 i);
    void load_member(u32 i);
    void store_member(u32 i);
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

    void gt();
    void gte();
    void lt();
    void lte();
    void eq();
    void neq();

    void jmp(LabelID target);
    void jmp_true(LabelID target);
    void jmp_true_pop(LabelID target);
    void jmp_false(LabelID target);
    void jmp_false_pop(LabelID target);
    void call(u32 n);
    void ret();

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

} // namespace hammer

#endif // HAMMER_COMPILER_CODE_BUILDER_HPP
