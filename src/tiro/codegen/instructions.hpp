#ifndef TIRO_CODEGEN_INSTRUCTIONS_HPP
#define TIRO_CODEGEN_INSTRUCTIONS_HPP

#include "tiro/codegen/code_builder.hpp"
#include "tiro/core/arena.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/core/safe_int.hpp"

#include <new>

namespace tiro {

class BasicBlock;

#define TIRO_INSTRUCTION_TYPES(X) \
    X(LoadNull)                   \
    X(LoadFalse)                  \
    X(LoadTrue)                   \
    X(LoadInt)                    \
    X(LoadFloat)                  \
                                  \
    X(LoadParam)                  \
    X(StoreParam)                 \
    X(LoadLocal)                  \
    X(StoreLocal)                 \
    X(LoadClosure)                \
    X(LoadContext)                \
    X(StoreContext)               \
                                  \
    X(LoadMember)                 \
    X(StoreMember)                \
    X(LoadTupleMember)            \
    X(StoreTupleMember)           \
    X(LoadIndex)                  \
    X(StoreIndex)                 \
    X(LoadModule)                 \
    X(StoreModule)                \
    X(LoadGlobal)                 \
                                  \
    X(Dup)                        \
    X(Pop)                        \
    X(PopN)                       \
    X(Rot2)                       \
    X(Rot3)                       \
    X(Rot4)                       \
                                  \
    X(Add)                        \
    X(Sub)                        \
    X(Mul)                        \
    X(Div)                        \
    X(Mod)                        \
    X(Pow)                        \
    X(LNot)                       \
    X(BNot)                       \
    X(UPos)                       \
    X(UNeg)                       \
    X(LSh)                        \
    X(RSh)                        \
    X(BAnd)                       \
    X(BOr)                        \
    X(BXor)                       \
                                  \
    X(Gt)                         \
    X(Gte)                        \
    X(Lt)                         \
    X(Lte)                        \
    X(Eq)                         \
    X(NEq)                        \
                                  \
    X(MkArray)                    \
    X(MkTuple)                    \
    X(MkSet)                      \
    X(MkMap)                      \
    X(MkContext)                  \
    X(MkClosure)                  \
                                  \
    X(MkBuilder)                  \
    X(BuilderAppend)              \
    X(BuilderString)              \
                                  \
    X(Call)                       \
                                  \
    X(LoadMethod)                 \
    X(CallMethod)

enum class InstructionType : byte {
#define TIRO_DEFINE_ENUM(Name) Name,
    TIRO_INSTRUCTION_TYPES(TIRO_DEFINE_ENUM)
#undef TIRO_DEFINE_ENUM
};

std::string_view to_string(InstructionType type);

template<typename InstructionT>
struct InstructionToTypeTag;

template<InstructionType type>
struct InstructionFromTypeTag;

#define TIRO_DECLARE_TYPE(X)                                         \
    class X;                                                         \
                                                                     \
    template<>                                                       \
    struct InstructionToTypeTag<X> {                                 \
        static constexpr InstructionType value = InstructionType::X; \
    };                                                               \
                                                                     \
    template<>                                                       \
    struct InstructionFromTypeTag<InstructionType::X> {              \
        using type = X;                                              \
    };

TIRO_INSTRUCTION_TYPES(TIRO_DECLARE_TYPE)

#undef TIRO_DECLARE_TYPE

/// Provides the appropriate InstructionType tag value for the given
/// instruction class type.
template<typename InstructionT>
inline constexpr InstructionType instruction_to_tag_v =
    InstructionToTypeTag<InstructionT>::value;

/// Maps a InstructionType value to its Instruction class type.
template<InstructionType tag>
using instruction_from_tag_t = typename InstructionFromTypeTag<tag>::type;

/// Instruction classes represent bytecode instructions.
/// Note that this hierarchy does not include branching instructions.
class Instruction {
public:
    /// Returns the runtime type of this instruction. Must match
    /// the actual class type.
    InstructionType type() const noexcept { return type_; }

protected:
    explicit Instruction(InstructionType type)
        : type_(type) {}

private:
    InstructionType type_;
};

#define TIRO_STATIC_ARGS(N) \
    static constexpr u32 stack_arguments() noexcept { return (N); }

#define TIRO_STATIC_RESULTS(N) \
    static constexpr u32 stack_results() noexcept { return (N); }

#define TIRO_STATIC_STACK(PopCount, PushCount) \
    TIRO_STATIC_ARGS(PopCount)                 \
    TIRO_STATIC_RESULTS(PushCount)

class LoadNull final : public Instruction {
public:
    LoadNull()
        : Instruction(InstructionType::LoadNull) {}

    TIRO_STATIC_STACK(0, 1)

    void emit_bytecode(CodeBuilder& b) { b.load_null(); }
};

class LoadFalse final : public Instruction {
public:
    LoadFalse()
        : Instruction(InstructionType::LoadFalse) {}

    TIRO_STATIC_STACK(0, 1)

    void emit_bytecode(CodeBuilder& b) { b.load_false(); }
};

class LoadTrue final : public Instruction {
public:
    LoadTrue()
        : Instruction(InstructionType::LoadTrue) {}

    TIRO_STATIC_STACK(0, 1)

    void emit_bytecode(CodeBuilder& b) { b.load_true(); }
};

class LoadInt final : public Instruction {
public:
    explicit LoadInt(i64 value)
        : Instruction(InstructionType::LoadInt)
        , value_(value) {}

    i64 value() const noexcept { return value_; }

    TIRO_STATIC_STACK(0, 1)

    void emit_bytecode(CodeBuilder& b) { b.load_int(value_); }

private:
    i64 value_;
};

class LoadFloat final : public Instruction {
public:
    explicit LoadFloat(f64 value)
        : Instruction(InstructionType::LoadFloat)
        , value_(value) {}

    f64 value() const noexcept { return value_; }

    TIRO_STATIC_STACK(0, 1)

    void emit_bytecode(CodeBuilder& b) { b.load_float(value_); }

private:
    f64 value_;
};

class LoadParam final : public Instruction {
public:
    explicit LoadParam(u32 index)
        : Instruction(InstructionType::LoadParam)
        , index_(index) {}

    u32 index() const noexcept { return index_; }

    TIRO_STATIC_STACK(0, 1)

    void emit_bytecode(CodeBuilder& b) { b.load_param(index_); }

private:
    u32 index_;
};

class StoreParam final : public Instruction {
public:
    explicit StoreParam(u32 index)
        : Instruction(InstructionType::StoreParam)
        , index_(index) {}

    u32 index() const noexcept { return index_; }

    TIRO_STATIC_STACK(1, 0)

    void emit_bytecode(CodeBuilder& b) { b.store_param(index_); }

private:
    u32 index_;
};

class LoadLocal final : public Instruction {
public:
    explicit LoadLocal(u32 index)
        : Instruction(InstructionType::LoadLocal)
        , index_(index) {}

    u32 index() const noexcept { return index_; }

    TIRO_STATIC_STACK(0, 1)

    void emit_bytecode(CodeBuilder& b) { b.load_local(index_); }

private:
    u32 index_;
};

class StoreLocal final : public Instruction {
public:
    explicit StoreLocal(u32 index)
        : Instruction(InstructionType::StoreLocal)
        , index_(index) {}

    u32 index() const noexcept { return index_; }

    TIRO_STATIC_STACK(1, 0)

    void emit_bytecode(CodeBuilder& b) { b.store_local(index_); }

private:
    u32 index_;
};

class LoadClosure final : public Instruction {
public:
    LoadClosure()
        : Instruction(InstructionType::LoadClosure) {}

    TIRO_STATIC_STACK(0, 1)

    void emit_bytecode(CodeBuilder& b) { b.load_closure(); }
};

class LoadContext final : public Instruction {
public:
    explicit LoadContext(u32 level, u32 index)
        : Instruction(InstructionType::LoadContext)
        , level_(level)
        , index_(index) {}

    u32 level() const noexcept { return level_; }
    u32 index() const noexcept { return index_; }

    TIRO_STATIC_STACK(1, 1)

    void emit_bytecode(CodeBuilder& b) { b.load_context(level_, index_); }

private:
    u32 level_;
    u32 index_;
};

class StoreContext final : public Instruction {
public:
    explicit StoreContext(u32 level, u32 index)
        : Instruction(InstructionType::StoreContext)
        , level_(level)
        , index_(index) {}

    u32 level() const noexcept { return level_; }
    u32 index() const noexcept { return index_; }

    TIRO_STATIC_STACK(2, 0)

    void emit_bytecode(CodeBuilder& b) { b.store_context(level_, index_); }

private:
    u32 level_;
    u32 index_;
};

class LoadMember final : public Instruction {
public:
    explicit LoadMember(u32 index)
        : Instruction(InstructionType::LoadMember)
        , index_(index) {}

    u32 index() const noexcept { return index_; }

    TIRO_STATIC_STACK(1, 1)

    void emit_bytecode(CodeBuilder& b) { b.load_member(index_); }

private:
    u32 index_;
};

class StoreMember final : public Instruction {
public:
    explicit StoreMember(u32 index)
        : Instruction(InstructionType::StoreMember)
        , index_(index) {}

    u32 index() const noexcept { return index_; }

    TIRO_STATIC_STACK(2, 0)

    void emit_bytecode(CodeBuilder& b) { b.store_member(index_); }

private:
    u32 index_;
};

class LoadTupleMember final : public Instruction {
public:
    explicit LoadTupleMember(u32 index)
        : Instruction(InstructionType::LoadTupleMember)
        , index_(index) {}

    u32 index() const noexcept { return index_; }

    TIRO_STATIC_STACK(1, 1)

    void emit_bytecode(CodeBuilder& b) { b.load_tuple_member(index_); }

private:
    u32 index_;
};

class StoreTupleMember final : public Instruction {
public:
    explicit StoreTupleMember(u32 index)
        : Instruction(InstructionType::StoreTupleMember)
        , index_(index) {}

    u32 index() const noexcept { return index_; }

    TIRO_STATIC_STACK(2, 0)

    void emit_bytecode(CodeBuilder& b) { b.store_tuple_member(index_); }

private:
    u32 index_;
};

class LoadIndex final : public Instruction {
public:
    LoadIndex()
        : Instruction(InstructionType::LoadIndex) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.load_index(); }
};

class StoreIndex final : public Instruction {
public:
    StoreIndex()
        : Instruction(InstructionType::StoreIndex) {}

    TIRO_STATIC_STACK(3, 0)

    void emit_bytecode(CodeBuilder& b) { b.store_index(); }
};

class LoadModule final : public Instruction {
public:
    explicit LoadModule(u32 index)
        : Instruction(InstructionType::LoadModule)
        , index_(index) {}

    u32 index() const noexcept { return index_; }

    TIRO_STATIC_STACK(0, 1)

    void emit_bytecode(CodeBuilder& b) { b.load_module(index_); }

private:
    u32 index_;
};

class StoreModule final : public Instruction {
public:
    explicit StoreModule(u32 index)
        : Instruction(InstructionType::StoreModule)
        , index_(index) {}

    u32 index() const noexcept { return index_; }

    TIRO_STATIC_STACK(1, 0)

    void emit_bytecode(CodeBuilder& b) { b.store_module(index_); }

private:
    u32 index_;
};

class LoadGlobal final : public Instruction {
public:
    explicit LoadGlobal(u32 index)
        : Instruction(InstructionType::LoadGlobal)
        , index_(index) {}

    u32 index() const noexcept { return index_; }

    TIRO_STATIC_STACK(1, 0)

    void emit_bytecode(CodeBuilder& b) { b.load_global(index_); }

private:
    u32 index_;
};

class Dup final : public Instruction {
public:
    Dup()
        : Instruction(InstructionType::Dup) {}

    TIRO_STATIC_STACK(0, 1)

    void emit_bytecode(CodeBuilder& b) { b.dup(); }
};

class Pop final : public Instruction {
public:
    Pop()
        : Instruction(InstructionType::Pop) {}

    TIRO_STATIC_STACK(1, 0)

    void emit_bytecode(CodeBuilder& b) { b.pop(); }
};

class PopN final : public Instruction {
public:
    explicit PopN(u32 count)
        : Instruction(InstructionType::PopN)
        , count_(count) {}

    u32 count() const noexcept { return count_; }

    TIRO_STATIC_RESULTS(0)

    constexpr u32 stack_arguments() const { return count_; }

    void emit_bytecode(CodeBuilder& b) { b.pop_n(count_); }

private:
    u32 count_;
};

class Rot2 final : public Instruction {
public:
    Rot2()
        : Instruction(InstructionType::Rot2) {}

    TIRO_STATIC_STACK(0, 0)

    void emit_bytecode(CodeBuilder& b) { b.rot_2(); }
};

class Rot3 final : public Instruction {
public:
    Rot3()
        : Instruction(InstructionType::Rot3) {}

    TIRO_STATIC_STACK(0, 0)

    void emit_bytecode(CodeBuilder& b) { b.rot_3(); }
};

class Rot4 final : public Instruction {
public:
    Rot4()
        : Instruction(InstructionType::Rot4) {}

    TIRO_STATIC_STACK(0, 0)

    void emit_bytecode(CodeBuilder& b) { b.rot_4(); }
};

class Add final : public Instruction {
public:
    Add()
        : Instruction(InstructionType::Add) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.add(); }
};

class Sub final : public Instruction {
public:
    Sub()
        : Instruction(InstructionType::Sub) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.sub(); }
};

class Mul final : public Instruction {
public:
    Mul()
        : Instruction(InstructionType::Mul) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.mul(); }
};

class Div final : public Instruction {
public:
    Div()
        : Instruction(InstructionType::Div) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.div(); }
};

class Mod final : public Instruction {
public:
    Mod()
        : Instruction(InstructionType::Mod) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.mod(); }
};

class Pow final : public Instruction {
public:
    Pow()
        : Instruction(InstructionType::Pow) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.pow(); }
};

class LNot final : public Instruction {
public:
    LNot()
        : Instruction(InstructionType::LNot) {}

    TIRO_STATIC_STACK(1, 1)

    void emit_bytecode(CodeBuilder& b) { b.lnot(); }
};

class BNot final : public Instruction {
public:
    BNot()
        : Instruction(InstructionType::BNot) {}

    TIRO_STATIC_STACK(1, 1)

    void emit_bytecode(CodeBuilder& b) { b.bnot(); }
};

class UPos final : public Instruction {
public:
    UPos()
        : Instruction(InstructionType::UPos) {}

    TIRO_STATIC_STACK(1, 1)

    void emit_bytecode(CodeBuilder& b) { b.upos(); }
};

class UNeg final : public Instruction {
public:
    UNeg()
        : Instruction(InstructionType::UNeg) {}

    TIRO_STATIC_STACK(1, 1)

    void emit_bytecode(CodeBuilder& b) { b.uneg(); }
};

class LSh final : public Instruction {
public:
    LSh()
        : Instruction(InstructionType::LSh) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.lsh(); }
};

class RSh final : public Instruction {
public:
    RSh()
        : Instruction(InstructionType::RSh) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.rsh(); }
};

class BAnd final : public Instruction {
public:
    BAnd()
        : Instruction(InstructionType::BAnd) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.band(); }
};

class BOr final : public Instruction {
public:
    BOr()
        : Instruction(InstructionType::BOr) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.bor(); }
};

class BXor final : public Instruction {
public:
    BXor()
        : Instruction(InstructionType::BXor) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.bxor(); }
};

class Gt final : public Instruction {
public:
    Gt()
        : Instruction(InstructionType::Gt) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.gt(); }
};

class Gte final : public Instruction {
public:
    Gte()
        : Instruction(InstructionType::Gte) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.gte(); }
};

class Lt final : public Instruction {
public:
    Lt()
        : Instruction(InstructionType::Lt) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.lt(); }
};

class Lte final : public Instruction {
public:
    Lte()
        : Instruction(InstructionType::Lte) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.lte(); }
};

class Eq final : public Instruction {
public:
    Eq()
        : Instruction(InstructionType::Eq) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.eq(); }
};

class NEq final : public Instruction {
public:
    NEq()
        : Instruction(InstructionType::NEq) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.neq(); }
};

class MkArray final : public Instruction {
public:
    explicit MkArray(u32 count)
        : Instruction(InstructionType::MkArray)
        , count_(count) {}

    u32 count() const noexcept { return count_; }

    TIRO_STATIC_RESULTS(1)

    constexpr u32 stack_arguments() const noexcept { return count_; }

    void emit_bytecode(CodeBuilder& b) { b.mk_array(count_); }

private:
    u32 count_;
};

class MkTuple final : public Instruction {
public:
    explicit MkTuple(u32 count)
        : Instruction(InstructionType::MkTuple)
        , count_(count) {}

    u32 count() const noexcept { return count_; }

    TIRO_STATIC_RESULTS(1)

    constexpr u32 stack_arguments() const noexcept { return count_; }

    void emit_bytecode(CodeBuilder& b) { b.mk_tuple(count_); }

private:
    u32 count_;
};

class MkSet final : public Instruction {
public:
    explicit MkSet(u32 count)
        : Instruction(InstructionType::MkSet)
        , count_(count) {}

    u32 count() const noexcept { return count_; }

    TIRO_STATIC_RESULTS(1)

    constexpr u32 stack_arguments() const noexcept { return count_; }

    void emit_bytecode(CodeBuilder& b) { b.mk_set(count_); }

private:
    u32 count_;
};

class MkMap final : public Instruction {
public:
    explicit MkMap(u32 count)
        : Instruction(InstructionType::MkMap)
        , count_(count) {}

    u32 count() const noexcept { return count_; }

    TIRO_STATIC_RESULTS(1)

    u32 stack_arguments() const { return (SafeInt(count_) * 2).value(); }

    void emit_bytecode(CodeBuilder& b) { b.mk_map(count_); }

private:
    u32 count_;
};

class MkContext final : public Instruction {
public:
    explicit MkContext(u32 count)
        : Instruction(InstructionType::MkContext)
        , count_(count) {}

    u32 count() const noexcept { return count_; }

    TIRO_STATIC_STACK(1, 1)

    void emit_bytecode(CodeBuilder& b) { b.mk_context(count_); }

private:
    u32 count_;
};

class MkClosure final : public Instruction {
public:
    MkClosure()
        : Instruction(InstructionType::MkClosure) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.mk_closure(); }
};

class MkBuilder final : public Instruction {
public:
    MkBuilder()
        : Instruction(InstructionType::MkBuilder) {}

    TIRO_STATIC_STACK(0, 1)

    void emit_bytecode(CodeBuilder& b) { b.mk_builder(); }
};

class BuilderAppend final : public Instruction {
public:
    BuilderAppend()
        : Instruction(InstructionType::BuilderAppend) {}

    TIRO_STATIC_STACK(2, 1)

    void emit_bytecode(CodeBuilder& b) { b.builder_append(); }
};

class BuilderString final : public Instruction {
public:
    BuilderString()
        : Instruction(InstructionType::BuilderString) {}

    TIRO_STATIC_STACK(1, 1)

    void emit_bytecode(CodeBuilder& b) { b.builder_string(); }
};

class Call final : public Instruction {
public:
    explicit Call(u32 count)
        : Instruction(InstructionType::Call)
        , count_(count) {}

    u32 count() const noexcept { return count_; }

    TIRO_STATIC_RESULTS(1)

    u32 stack_arguments() const { return (SafeInt(count_) + 1).value(); }

    void emit_bytecode(CodeBuilder& b) { b.call(count_); }

private:
    u32 count_;
};

class LoadMethod final : public Instruction {
public:
    explicit LoadMethod(u32 index)
        : Instruction(InstructionType::LoadMethod)
        , index_(index) {}

    u32 index() const noexcept { return index_; }

    TIRO_STATIC_STACK(1, 2)

    void emit_bytecode(CodeBuilder& b) { b.load_method(index_); }

private:
    u32 index_;
};

class CallMethod final : public Instruction {
public:
    explicit CallMethod(u32 count)
        : Instruction(InstructionType::CallMethod)
        , count_(count) {}

    u32 count() const noexcept { return count_; }

    TIRO_STATIC_RESULTS(1)

    u32 stack_arguments() const { return (SafeInt(count_) + 2).value(); }

    void emit_bytecode(CodeBuilder& b) { b.call_method(count_); }

private:
    u32 count_;
};

/// Returns the number of arguments on the stack required by the given instruction.
u32 stack_arguments(NotNull<Instruction*> instr);

/// Returns the number of values returned on the stack by the given instruction.
u32 stack_results(NotNull<Instruction*> instr);

/// Emits the given instruction using the provided builder.
void emit_instruction(NotNull<Instruction*> instr, CodeBuilder& builder);

class InstructionStorage {
public:
    template<typename T, typename... Args>
    NotNull<T*> make(Args&&... args) {
        static_assert(std::is_base_of_v<Instruction, T>,
            "Must be a subclass of instruction.");

        void* storage = arena_.allocate(sizeof(T), alignof(T));
        auto result = TIRO_NN(new (storage) T(std::forward<Args>(args)...));
        TIRO_ASSERT(result->type() == instruction_to_tag_v<T>,
            "New instruction has invalid type tag.");
        return result;
    }

    void reset() { arena_.deallocate(); }

private:
    Arena arena_;
};

enum class BranchInstruction { JmpTrue, JmpTruePop, JmpFalse, JmpFalsePop };

std::string_view to_string(BranchInstruction instr);

/// Returns the number of stack values consumed by this instruction.
u32 stack_arguments(BranchInstruction instr);

void emit_instruction(
    BranchInstruction instr, BasicBlock* target, CodeBuilder& builder);

} // namespace tiro

#endif // TIRO_CODEGEN_INSTRUCTIONS_HPP
