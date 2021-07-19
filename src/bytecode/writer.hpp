#ifndef TIRO_BYTECODE_WRITER_HPP
#define TIRO_BYTECODE_WRITER_HPP

#include "bytecode/function.hpp"
#include "bytecode/fwd.hpp"
#include "common/defs.hpp"
#include "common/entities/entity_id.hpp"
#include "common/hash.hpp"
#include "common/memory/binary.hpp"

#include <absl/container/flat_hash_map.h>
#include <absl/container/inlined_vector.h>

#include <vector>

namespace tiro {

TIRO_DEFINE_ENTITY_ID(BytecodeLabel, u32)

class BytecodeWriter final {
public:
    explicit BytecodeWriter(BytecodeFunction& output);

    /// Marks the start of the given label at the current position.
    /// Jumps that refer to that label will receive the correct location.
    /// Every label used in any kind of jump instruction must be defined at some point.
    /// \pre `label` must be valid
    void define_label(BytecodeLabel label);

    /// Marks the current byte offset as the start of a section that has the given
    /// handler as its exception handler. Use an invalid BytecodeLabel to signal "no handler",
    /// which is also the starting value.
    /// \pre `handler_label` must be valid
    void start_handler(BytecodeLabel handler_label);

    /// Complete bytecode construction. Call this after all instructions
    /// have been emitted. All required block labels must be defined
    /// when this function is called, because it will patch all label references.
    void finish();

    /// Returns the list of module references that have been emitted by the compilation process.
    std::vector<std::tuple<u32, BytecodeMemberId>> take_module_refs() {
        return std::move(module_refs_);
    }

    void load_null(BytecodeRegister target) { write(BytecodeOp::LoadNull, target); }
    void load_false(BytecodeRegister target) { write(BytecodeOp::LoadFalse, target); }
    void load_true(BytecodeRegister target) { write(BytecodeOp::LoadTrue, target); }
    void load_int(i64 value, BytecodeRegister target) { write(BytecodeOp::LoadInt, value, target); }
    void load_float(f64 value, BytecodeRegister target) {
        write(BytecodeOp::LoadFloat, value, target);
    }

    void load_param(BytecodeParam source, BytecodeRegister target) {
        write(BytecodeOp::LoadParam, source, target);
    }

    void store_param(BytecodeRegister source, BytecodeParam target) {
        write(BytecodeOp::StoreParam, source, target);
    }

    void load_module(BytecodeMemberId source, BytecodeRegister target) {
        write(BytecodeOp::LoadModule, source, target);
    }

    void store_module(BytecodeRegister source, BytecodeMemberId target) {
        write(BytecodeOp::StoreModule, source, target);
    }

    void load_member(BytecodeRegister object, BytecodeMemberId name, BytecodeRegister target) {
        write(BytecodeOp::LoadMember, object, name, target);
    }

    void store_member(BytecodeRegister source, BytecodeRegister object, BytecodeMemberId name) {
        write(BytecodeOp::StoreMember, source, object, name);
    }

    void load_tuple_member(BytecodeRegister tuple, u32 index, BytecodeRegister target) {
        write(BytecodeOp::LoadTupleMember, tuple, index, target);
    }

    void store_tuple_member(BytecodeRegister source, BytecodeRegister tuple, u32 index) {
        write(BytecodeOp::StoreTupleMember, source, tuple, index);
    }

    void load_index(BytecodeRegister array, BytecodeRegister index, BytecodeRegister target) {
        write(BytecodeOp::LoadIndex, array, index, target);
    }

    void store_index(BytecodeRegister source, BytecodeRegister array, BytecodeRegister index) {
        write(BytecodeOp::StoreIndex, source, array, index);
    }

    void load_closure(BytecodeRegister target) { write(BytecodeOp::LoadClosure, target); }

    void load_env(BytecodeRegister env, u32 level, u32 index, BytecodeRegister target) {
        write(BytecodeOp::LoadEnv, env, level, index, target);
    }

    void store_env(BytecodeRegister source, BytecodeRegister env, u32 level, u32 index) {
        write(BytecodeOp::StoreEnv, source, env, level, index);
    }

    void add(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::Add, lhs, rhs, target);
    }

    void sub(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::Sub, lhs, rhs, target);
    }

    void mul(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::Mul, lhs, rhs, target);
    }

    void div(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::Div, lhs, rhs, target);
    }

    void mod(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::Mod, lhs, rhs, target);
    }

    void pow(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::Pow, lhs, rhs, target);
    }

    void uadd(BytecodeRegister value, BytecodeRegister target) {
        write(BytecodeOp::UAdd, value, target);
    }

    void uneg(BytecodeRegister value, BytecodeRegister target) {
        write(BytecodeOp::UNeg, value, target);
    }

    void lsh(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::LSh, lhs, rhs, target);
    }

    void rsh(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::RSh, lhs, rhs, target);
    }

    void band(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::BAnd, lhs, rhs, target);
    }

    void bor(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::BOr, lhs, rhs, target);
    }

    void bxor(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::BXor, lhs, rhs, target);
    }

    void bnot(BytecodeRegister value, BytecodeRegister target) {
        write(BytecodeOp::BNot, value, target);
    }

    void gt(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::Gt, lhs, rhs, target);
    }

    void gte(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::Gte, lhs, rhs, target);
    }

    void lt(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::Lt, lhs, rhs, target);
    }

    void lte(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::Lte, lhs, rhs, target);
    }

    void eq(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::Eq, lhs, rhs, target);
    }

    void neq(BytecodeRegister lhs, BytecodeRegister rhs, BytecodeRegister target) {
        write(BytecodeOp::NEq, lhs, rhs, target);
    }

    void lnot(BytecodeRegister value, BytecodeRegister target) {
        write(BytecodeOp::LNot, value, target);
    }

    void array(u32 count, BytecodeRegister target) { write(BytecodeOp::Array, count, target); }

    void tuple(u32 count, BytecodeRegister target) { write(BytecodeOp::Tuple, count, target); }

    void set(u32 count, BytecodeRegister target) { write(BytecodeOp::Set, count, target); }

    void map(u32 count, BytecodeRegister target) { write(BytecodeOp::Map, count, target); }

    void env(BytecodeRegister parent, u32 size, BytecodeRegister target) {
        write(BytecodeOp::Env, parent, size, target);
    }

    void closure(BytecodeMemberId tmpl, BytecodeRegister env, BytecodeRegister target) {
        write(BytecodeOp::Closure, tmpl, env, target);
    }

    void record(BytecodeMemberId tmpl, BytecodeRegister target) {
        write(BytecodeOp::Record, tmpl, target);
    }

    void iterator(BytecodeRegister container, BytecodeRegister target) {
        write(BytecodeOp::Iterator, container, target);
    }

    void iterator_next(BytecodeRegister iterator, BytecodeRegister valid, BytecodeRegister value) {
        write(BytecodeOp::IteratorNext, iterator, valid, value);
    }

    void formatter(BytecodeRegister target) { write(BytecodeOp::Formatter, target); }

    void append_format(BytecodeRegister value, BytecodeRegister formatter) {
        write(BytecodeOp::AppendFormat, value, formatter);
    }

    void format_result(BytecodeRegister formatter, BytecodeRegister target) {
        write(BytecodeOp::FormatResult, formatter, target);
    }

    void copy(BytecodeRegister source, BytecodeRegister target) {
        write(BytecodeOp::Copy, source, target);
    }

    void swap(BytecodeRegister a, BytecodeRegister b) { write(BytecodeOp::Swap, a, b); }

    void push(BytecodeRegister value) { write(BytecodeOp::Push, value); }

    void pop() { write(BytecodeOp::Pop); }

    void pop_to(BytecodeRegister target) { write(BytecodeOp::PopTo, target); }

    void jmp(BytecodeOffset offset) { write(BytecodeOp::Jmp, offset); }
    void jmp(BytecodeLabel label) { write(BytecodeOp::Jmp, label); }

    void jmp_true(BytecodeRegister condition, BytecodeOffset offset) {
        write(BytecodeOp::JmpTrue, condition, offset);
    }
    void jmp_true(BytecodeRegister condition, BytecodeLabel label) {
        write(BytecodeOp::JmpTrue, condition, label);
    }

    void jmp_false(BytecodeRegister condition, BytecodeOffset offset) {
        write(BytecodeOp::JmpFalse, condition, offset);
    }
    void jmp_false(BytecodeRegister condition, BytecodeLabel label) {
        write(BytecodeOp::JmpFalse, condition, label);
    }

    void jmp_null(BytecodeRegister condition, BytecodeOffset offset) {
        write(BytecodeOp::JmpNull, condition, offset);
    }
    void jmp_null(BytecodeRegister condition, BytecodeLabel label) {
        write(BytecodeOp::JmpNull, condition, label);
    }

    void jmp_not_null(BytecodeRegister condition, BytecodeOffset offset) {
        write(BytecodeOp::JmpNotNull, condition, offset);
    }
    void jmp_not_null(BytecodeRegister condition, BytecodeLabel label) {
        write(BytecodeOp::JmpNotNull, condition, label);
    }

    void call(BytecodeRegister function, u32 count) { write(BytecodeOp::Call, function, count); }

    void load_method(BytecodeRegister object, BytecodeMemberId name, BytecodeRegister thiz,
        BytecodeRegister method) {
        write(BytecodeOp::LoadMethod, object, name, thiz, method);
    }

    void call_method(BytecodeRegister method, u32 count) {
        write(BytecodeOp::CallMethod, method, count);
    }

    void ret(BytecodeRegister value) { write(BytecodeOp::Return, value); }

    void rethrow() { write(BytecodeOp::Rethrow); }

    void assert_fail(BytecodeRegister expr, BytecodeRegister message) {
        write(BytecodeOp::AssertFail, expr, message);
    }

private:
    struct HandlerEntry {
        BytecodeOffset from;
        BytecodeOffset to;
        BytecodeLabel target;
    };

    void finish_handler();
    void simplify_handlers(std::vector<ExceptionHandler>& handlers);

    template<typename... Args>
    void write(const Args&... args) {
        (write_impl(args), ...);
    }

    void write_impl(BytecodeOp op);
    void write_impl(BytecodeParam param);
    void write_impl(BytecodeRegister local);
    void write_impl(BytecodeOffset offset);
    void write_impl(BytecodeLabel label);
    void write_impl(BytecodeMemberId mod);
    void write_impl(u32 value);
    void write_impl(i64 value);
    void write_impl(f64 value);

    // Compile time error on implicit conversions:
    template<typename T>
    void write_impl(T) = delete;

    u32 pos() const;

private:
    BytecodeFunction& output_;
    BinaryWriter wr_;
    absl::InlinedVector<HandlerEntry, 4> handlers_;

    // maps bytecode label id to its actual offset in the emitted code
    absl::flat_hash_map<BytecodeLabel, u32, UseHasher> label_defs_;

    // contains label references `(position, label_id) yet to be resolved in finish().
    std::vector<std::tuple<u32, BytecodeLabel>> label_refs_;

    // contains module member references to be resolved when linking
    std::vector<std::tuple<u32, BytecodeMemberId>> module_refs_;

    // current exception handler state
    BytecodeLabel handler_;
    u32 handler_start_ = 0;
};

} // namespace tiro

#endif // TIRO_BYTECODE_WRITER_HPP
