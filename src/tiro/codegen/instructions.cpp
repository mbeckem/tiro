#include "tiro/codegen/instructions.hpp"

#include "tiro/codegen/code_builder.hpp"

#include <type_traits>

namespace tiro {

template<typename T>
static NotNull<T*> must(NotNull<Instruction*> instr) {
    TIRO_ASSERT(instr->type() == instruction_to_tag_v<T>,
        "Invalid cast: Instruction is not of the required type.");
    return NotNull<T*>(guaranteed_not_null, static_cast<T*>(instr.get()));
}

template<typename Visitor>
static decltype(auto) visit(NotNull<Instruction*> instr, Visitor&& v) {
    switch (instr->type()) {
#define TIRO_CASE(X)         \
    case InstructionType::X: \
        return v(must<X>(instr));

        TIRO_INSTRUCTION_TYPES(TIRO_CASE)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid instruction type.");
}

std::string_view to_string(InstructionType type) {
    switch (type) {
#define TIRO_STR(X)          \
    case InstructionType::X: \
        return #X;

        TIRO_INSTRUCTION_TYPES(TIRO_STR)

#undef TIRO_STR
    }

    TIRO_UNREACHABLE("Invalid instruction type.");
}

u32 stack_arguments(NotNull<Instruction*> instr) {
    return visit(instr, [&](auto p) { return p->stack_arguments(); });
}

u32 stack_results(NotNull<Instruction*> instr) {
    return visit(instr, [&](auto p) { return p->stack_results(); });
}

void emit_instruction(NotNull<Instruction*> instr, CodeBuilder& b) {
    return visit(instr, [&](auto p) { return p->emit_bytecode(b); });
}

std::string_view to_string(BranchInstruction instr) {
    switch (instr) {
    case BranchInstruction::JmpTrue:
        return "JmpTrue";
    case BranchInstruction::JmpTruePop:
        return "JmpTruePop";
    case BranchInstruction::JmpFalse:
        return "JmpFalse";
    case BranchInstruction::JmpFalsePop:
        return "JmpFalsePop";
    }

    TIRO_UNREACHABLE("Invalid branch instruction type.");
}

u32 stack_arguments(BranchInstruction instr) {
    switch (instr) {
    case BranchInstruction::JmpTrue:
    case BranchInstruction::JmpFalse:
        return 0;
    case BranchInstruction::JmpTruePop:
    case BranchInstruction::JmpFalsePop:
        return 1;
    }

    TIRO_UNREACHABLE("Invalid branch instruction type.");
}

void emit_instruction(
    BranchInstruction instr, BasicBlock* target, CodeBuilder& builder) {
    switch (instr) {
    case BranchInstruction::JmpTrue:
        return builder.jmp_true(target);
    case BranchInstruction::JmpTruePop:
        return builder.jmp_true_pop(target);
    case BranchInstruction::JmpFalse:
        return builder.jmp_false(target);
    case BranchInstruction::JmpFalsePop:
        return builder.jmp_false_pop(target);
    }

    TIRO_UNREACHABLE("Invalid branch instruction type.");
}

#define TIRO_INSTRUCTION_PROPERTIES(Name)                 \
    static_assert(std::is_trivially_copyable_v<Name>,     \
        "Instruction must be trivially copyable.");       \
    static_assert(std::is_trivially_destructible_v<Name>, \
        "Instructions must be trivially destructible.");  \
    static_assert(                                        \
        std::is_final_v<Name>, "Concrete instructions must be final.");

TIRO_INSTRUCTION_TYPES(TIRO_INSTRUCTION_PROPERTIES)
#undef TIRO_INSTRUCTION_PROPERTIES

#define TIRO_TEST_MAPPING(Name)                                              \
    static_assert(instruction_to_tag_v<Name> == InstructionType::Name,       \
        "Must map to the correct instruction type tag.");                    \
    static_assert(                                                           \
        std::is_same_v<instruction_from_tag_t<InstructionType::Name>, Name>, \
        "Must map from tag to the correct instruction type.");

TIRO_INSTRUCTION_TYPES(TIRO_TEST_MAPPING)
#undef TIRO_TEST_MAPPING

} // namespace tiro
