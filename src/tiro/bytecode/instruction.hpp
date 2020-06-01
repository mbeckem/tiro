#ifndef TIRO_BYTECODE_BYTECODE_HPP
#define TIRO_BYTECODE_BYTECODE_HPP

#include "tiro/bytecode/fwd.hpp"
#include "tiro/bytecode/op.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/hash.hpp"

#include <string_view>

namespace tiro {

/* [[[cog
    from codegen.unions import define
    from codegen.bytecode import Instruction
    define(Instruction)
]]] */
/// Represents a bytecode instruction.
class BytecodeInstr final {
public:
    struct LoadNull final {
        BytecodeRegister target;

        explicit LoadNull(const BytecodeRegister& target_)
            : target(target_) {}
    };

    struct LoadFalse final {
        BytecodeRegister target;

        explicit LoadFalse(const BytecodeRegister& target_)
            : target(target_) {}
    };

    struct LoadTrue final {
        BytecodeRegister target;

        explicit LoadTrue(const BytecodeRegister& target_)
            : target(target_) {}
    };

    struct LoadInt final {
        i64 constant;
        BytecodeRegister target;

        LoadInt(const i64& constant_, const BytecodeRegister& target_)
            : constant(constant_)
            , target(target_) {}
    };

    struct LoadFloat final {
        f64 constant;
        BytecodeRegister target;

        LoadFloat(const f64& constant_, const BytecodeRegister& target_)
            : constant(constant_)
            , target(target_) {}
    };

    struct LoadParam final {
        BytecodeParam source;
        BytecodeRegister target;

        LoadParam(const BytecodeParam& source_, const BytecodeRegister& target_)
            : source(source_)
            , target(target_) {}
    };

    struct StoreParam final {
        BytecodeRegister source;
        BytecodeParam target;

        StoreParam(const BytecodeRegister& source_, const BytecodeParam& target_)
            : source(source_)
            , target(target_) {}
    };

    struct LoadModule final {
        BytecodeMemberId source;
        BytecodeRegister target;

        LoadModule(const BytecodeMemberId& source_, const BytecodeRegister& target_)
            : source(source_)
            , target(target_) {}
    };

    struct StoreModule final {
        BytecodeRegister source;
        BytecodeMemberId target;

        StoreModule(const BytecodeRegister& source_, const BytecodeMemberId& target_)
            : source(source_)
            , target(target_) {}
    };

    struct LoadMember final {
        BytecodeRegister object;
        BytecodeMemberId name;
        BytecodeRegister target;

        LoadMember(const BytecodeRegister& object_, const BytecodeMemberId& name_,
            const BytecodeRegister& target_)
            : object(object_)
            , name(name_)
            , target(target_) {}
    };

    struct StoreMember final {
        BytecodeRegister source;
        BytecodeRegister object;
        BytecodeMemberId name;

        StoreMember(const BytecodeRegister& source_, const BytecodeRegister& object_,
            const BytecodeMemberId& name_)
            : source(source_)
            , object(object_)
            , name(name_) {}
    };

    struct LoadTupleMember final {
        BytecodeRegister tuple;
        u32 index;
        BytecodeRegister target;

        LoadTupleMember(
            const BytecodeRegister& tuple_, const u32& index_, const BytecodeRegister& target_)
            : tuple(tuple_)
            , index(index_)
            , target(target_) {}
    };

    struct StoreTupleMember final {
        BytecodeRegister source;
        BytecodeRegister tuple;
        u32 index;

        StoreTupleMember(
            const BytecodeRegister& source_, const BytecodeRegister& tuple_, const u32& index_)
            : source(source_)
            , tuple(tuple_)
            , index(index_) {}
    };

    struct LoadIndex final {
        BytecodeRegister array;
        BytecodeRegister index;
        BytecodeRegister target;

        LoadIndex(const BytecodeRegister& array_, const BytecodeRegister& index_,
            const BytecodeRegister& target_)
            : array(array_)
            , index(index_)
            , target(target_) {}
    };

    struct StoreIndex final {
        BytecodeRegister source;
        BytecodeRegister array;
        BytecodeRegister index;

        StoreIndex(const BytecodeRegister& source_, const BytecodeRegister& array_,
            const BytecodeRegister& index_)
            : source(source_)
            , array(array_)
            , index(index_) {}
    };

    struct LoadClosure final {
        BytecodeRegister target;

        explicit LoadClosure(const BytecodeRegister& target_)
            : target(target_) {}
    };

    struct LoadEnv final {
        BytecodeRegister env;
        u32 level;
        u32 index;
        BytecodeRegister target;

        LoadEnv(const BytecodeRegister& env_, const u32& level_, const u32& index_,
            const BytecodeRegister& target_)
            : env(env_)
            , level(level_)
            , index(index_)
            , target(target_) {}
    };

    struct StoreEnv final {
        BytecodeRegister source;
        BytecodeRegister env;
        u32 level;
        u32 index;

        StoreEnv(const BytecodeRegister& source_, const BytecodeRegister& env_, const u32& level_,
            const u32& index_)
            : source(source_)
            , env(env_)
            , level(level_)
            , index(index_) {}
    };

    struct Add final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        Add(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Sub final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        Sub(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Mul final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        Mul(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Div final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        Div(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Mod final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        Mod(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Pow final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        Pow(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct UAdd final {
        BytecodeRegister value;
        BytecodeRegister target;

        UAdd(const BytecodeRegister& value_, const BytecodeRegister& target_)
            : value(value_)
            , target(target_) {}
    };

    struct UNeg final {
        BytecodeRegister value;
        BytecodeRegister target;

        UNeg(const BytecodeRegister& value_, const BytecodeRegister& target_)
            : value(value_)
            , target(target_) {}
    };

    struct LSh final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        LSh(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct RSh final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        RSh(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct BAnd final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        BAnd(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct BOr final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        BOr(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct BXor final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        BXor(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct BNot final {
        BytecodeRegister value;
        BytecodeRegister target;

        BNot(const BytecodeRegister& value_, const BytecodeRegister& target_)
            : value(value_)
            , target(target_) {}
    };

    struct Gt final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        Gt(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Gte final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        Gte(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Lt final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        Lt(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Lte final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        Lte(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Eq final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        Eq(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct NEq final {
        BytecodeRegister lhs;
        BytecodeRegister rhs;
        BytecodeRegister target;

        NEq(const BytecodeRegister& lhs_, const BytecodeRegister& rhs_,
            const BytecodeRegister& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct LNot final {
        BytecodeRegister value;
        BytecodeRegister target;

        LNot(const BytecodeRegister& value_, const BytecodeRegister& target_)
            : value(value_)
            , target(target_) {}
    };

    struct Array final {
        u32 count;
        BytecodeRegister target;

        Array(const u32& count_, const BytecodeRegister& target_)
            : count(count_)
            , target(target_) {}
    };

    struct Tuple final {
        u32 count;
        BytecodeRegister target;

        Tuple(const u32& count_, const BytecodeRegister& target_)
            : count(count_)
            , target(target_) {}
    };

    struct Set final {
        u32 count;
        BytecodeRegister target;

        Set(const u32& count_, const BytecodeRegister& target_)
            : count(count_)
            , target(target_) {}
    };

    struct Map final {
        u32 count;
        BytecodeRegister target;

        Map(const u32& count_, const BytecodeRegister& target_)
            : count(count_)
            , target(target_) {}
    };

    struct Env final {
        BytecodeRegister parent;
        u32 size;
        BytecodeRegister target;

        Env(const BytecodeRegister& parent_, const u32& size_, const BytecodeRegister& target_)
            : parent(parent_)
            , size(size_)
            , target(target_) {}
    };

    struct Closure final {
        BytecodeRegister tmpl;
        BytecodeRegister env;
        BytecodeRegister target;

        Closure(const BytecodeRegister& tmpl_, const BytecodeRegister& env_,
            const BytecodeRegister& target_)
            : tmpl(tmpl_)
            , env(env_)
            , target(target_) {}
    };

    struct Formatter final {
        BytecodeRegister target;

        explicit Formatter(const BytecodeRegister& target_)
            : target(target_) {}
    };

    struct AppendFormat final {
        BytecodeRegister value;
        BytecodeRegister formatter;

        AppendFormat(const BytecodeRegister& value_, const BytecodeRegister& formatter_)
            : value(value_)
            , formatter(formatter_) {}
    };

    struct FormatResult final {
        BytecodeRegister formatter;
        BytecodeRegister target;

        FormatResult(const BytecodeRegister& formatter_, const BytecodeRegister& target_)
            : formatter(formatter_)
            , target(target_) {}
    };

    struct Copy final {
        BytecodeRegister source;
        BytecodeRegister target;

        Copy(const BytecodeRegister& source_, const BytecodeRegister& target_)
            : source(source_)
            , target(target_) {}
    };

    struct Swap final {
        BytecodeRegister a;
        BytecodeRegister b;

        Swap(const BytecodeRegister& a_, const BytecodeRegister& b_)
            : a(a_)
            , b(b_) {}
    };

    struct Push final {
        BytecodeRegister value;

        explicit Push(const BytecodeRegister& value_)
            : value(value_) {}
    };

    struct Pop final {};

    struct PopTo final {
        BytecodeRegister target;

        explicit PopTo(const BytecodeRegister& target_)
            : target(target_) {}
    };

    struct Jmp final {
        BytecodeOffset offset;

        explicit Jmp(const BytecodeOffset& offset_)
            : offset(offset_) {}
    };

    struct JmpTrue final {
        BytecodeRegister condition;
        BytecodeOffset offset;

        JmpTrue(const BytecodeRegister& condition_, const BytecodeOffset& offset_)
            : condition(condition_)
            , offset(offset_) {}
    };

    struct JmpFalse final {
        BytecodeRegister condition;
        BytecodeOffset offset;

        JmpFalse(const BytecodeRegister& condition_, const BytecodeOffset& offset_)
            : condition(condition_)
            , offset(offset_) {}
    };

    struct Call final {
        BytecodeRegister function;
        u32 count;

        Call(const BytecodeRegister& function_, const u32& count_)
            : function(function_)
            , count(count_) {}
    };

    struct LoadMethod final {
        BytecodeRegister object;
        BytecodeMemberId name;
        BytecodeRegister thiz;
        BytecodeRegister method;

        LoadMethod(const BytecodeRegister& object_, const BytecodeMemberId& name_,
            const BytecodeRegister& thiz_, const BytecodeRegister& method_)
            : object(object_)
            , name(name_)
            , thiz(thiz_)
            , method(method_) {}
    };

    struct CallMethod final {
        BytecodeRegister method;
        u32 count;

        CallMethod(const BytecodeRegister& method_, const u32& count_)
            : method(method_)
            , count(count_) {}
    };

    struct Return final {
        BytecodeRegister value;

        explicit Return(const BytecodeRegister& value_)
            : value(value_) {}
    };

    struct AssertFail final {
        BytecodeRegister expr;
        BytecodeRegister message;

        AssertFail(const BytecodeRegister& expr_, const BytecodeRegister& message_)
            : expr(expr_)
            , message(message_) {}
    };

    static BytecodeInstr make_load_null(const BytecodeRegister& target);
    static BytecodeInstr make_load_false(const BytecodeRegister& target);
    static BytecodeInstr make_load_true(const BytecodeRegister& target);
    static BytecodeInstr make_load_int(const i64& constant, const BytecodeRegister& target);
    static BytecodeInstr make_load_float(const f64& constant, const BytecodeRegister& target);
    static BytecodeInstr
    make_load_param(const BytecodeParam& source, const BytecodeRegister& target);
    static BytecodeInstr
    make_store_param(const BytecodeRegister& source, const BytecodeParam& target);
    static BytecodeInstr
    make_load_module(const BytecodeMemberId& source, const BytecodeRegister& target);
    static BytecodeInstr
    make_store_module(const BytecodeRegister& source, const BytecodeMemberId& target);
    static BytecodeInstr make_load_member(const BytecodeRegister& object,
        const BytecodeMemberId& name, const BytecodeRegister& target);
    static BytecodeInstr make_store_member(const BytecodeRegister& source,
        const BytecodeRegister& object, const BytecodeMemberId& name);
    static BytecodeInstr make_load_tuple_member(
        const BytecodeRegister& tuple, const u32& index, const BytecodeRegister& target);
    static BytecodeInstr make_store_tuple_member(
        const BytecodeRegister& source, const BytecodeRegister& tuple, const u32& index);
    static BytecodeInstr make_load_index(const BytecodeRegister& array,
        const BytecodeRegister& index, const BytecodeRegister& target);
    static BytecodeInstr make_store_index(const BytecodeRegister& source,
        const BytecodeRegister& array, const BytecodeRegister& index);
    static BytecodeInstr make_load_closure(const BytecodeRegister& target);
    static BytecodeInstr make_load_env(const BytecodeRegister& env, const u32& level,
        const u32& index, const BytecodeRegister& target);
    static BytecodeInstr make_store_env(const BytecodeRegister& source, const BytecodeRegister& env,
        const u32& level, const u32& index);
    static BytecodeInstr make_add(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_sub(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_mul(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_div(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_mod(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_pow(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_uadd(const BytecodeRegister& value, const BytecodeRegister& target);
    static BytecodeInstr make_uneg(const BytecodeRegister& value, const BytecodeRegister& target);
    static BytecodeInstr make_lsh(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_rsh(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_band(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_bor(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_bxor(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_bnot(const BytecodeRegister& value, const BytecodeRegister& target);
    static BytecodeInstr make_gt(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_gte(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_lt(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_lte(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_eq(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_neq(
        const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target);
    static BytecodeInstr make_lnot(const BytecodeRegister& value, const BytecodeRegister& target);
    static BytecodeInstr make_array(const u32& count, const BytecodeRegister& target);
    static BytecodeInstr make_tuple(const u32& count, const BytecodeRegister& target);
    static BytecodeInstr make_set(const u32& count, const BytecodeRegister& target);
    static BytecodeInstr make_map(const u32& count, const BytecodeRegister& target);
    static BytecodeInstr
    make_env(const BytecodeRegister& parent, const u32& size, const BytecodeRegister& target);
    static BytecodeInstr make_closure(
        const BytecodeRegister& tmpl, const BytecodeRegister& env, const BytecodeRegister& target);
    static BytecodeInstr make_formatter(const BytecodeRegister& target);
    static BytecodeInstr
    make_append_format(const BytecodeRegister& value, const BytecodeRegister& formatter);
    static BytecodeInstr
    make_format_result(const BytecodeRegister& formatter, const BytecodeRegister& target);
    static BytecodeInstr make_copy(const BytecodeRegister& source, const BytecodeRegister& target);
    static BytecodeInstr make_swap(const BytecodeRegister& a, const BytecodeRegister& b);
    static BytecodeInstr make_push(const BytecodeRegister& value);
    static BytecodeInstr make_pop();
    static BytecodeInstr make_pop_to(const BytecodeRegister& target);
    static BytecodeInstr make_jmp(const BytecodeOffset& offset);
    static BytecodeInstr
    make_jmp_true(const BytecodeRegister& condition, const BytecodeOffset& offset);
    static BytecodeInstr
    make_jmp_false(const BytecodeRegister& condition, const BytecodeOffset& offset);
    static BytecodeInstr make_call(const BytecodeRegister& function, const u32& count);
    static BytecodeInstr make_load_method(const BytecodeRegister& object,
        const BytecodeMemberId& name, const BytecodeRegister& thiz, const BytecodeRegister& method);
    static BytecodeInstr make_call_method(const BytecodeRegister& method, const u32& count);
    static BytecodeInstr make_return(const BytecodeRegister& value);
    static BytecodeInstr
    make_assert_fail(const BytecodeRegister& expr, const BytecodeRegister& message);

    BytecodeInstr(LoadNull load_null);
    BytecodeInstr(LoadFalse load_false);
    BytecodeInstr(LoadTrue load_true);
    BytecodeInstr(LoadInt load_int);
    BytecodeInstr(LoadFloat load_float);
    BytecodeInstr(LoadParam load_param);
    BytecodeInstr(StoreParam store_param);
    BytecodeInstr(LoadModule load_module);
    BytecodeInstr(StoreModule store_module);
    BytecodeInstr(LoadMember load_member);
    BytecodeInstr(StoreMember store_member);
    BytecodeInstr(LoadTupleMember load_tuple_member);
    BytecodeInstr(StoreTupleMember store_tuple_member);
    BytecodeInstr(LoadIndex load_index);
    BytecodeInstr(StoreIndex store_index);
    BytecodeInstr(LoadClosure load_closure);
    BytecodeInstr(LoadEnv load_env);
    BytecodeInstr(StoreEnv store_env);
    BytecodeInstr(Add add);
    BytecodeInstr(Sub sub);
    BytecodeInstr(Mul mul);
    BytecodeInstr(Div div);
    BytecodeInstr(Mod mod);
    BytecodeInstr(Pow pow);
    BytecodeInstr(UAdd uadd);
    BytecodeInstr(UNeg uneg);
    BytecodeInstr(LSh lsh);
    BytecodeInstr(RSh rsh);
    BytecodeInstr(BAnd band);
    BytecodeInstr(BOr bor);
    BytecodeInstr(BXor bxor);
    BytecodeInstr(BNot bnot);
    BytecodeInstr(Gt gt);
    BytecodeInstr(Gte gte);
    BytecodeInstr(Lt lt);
    BytecodeInstr(Lte lte);
    BytecodeInstr(Eq eq);
    BytecodeInstr(NEq neq);
    BytecodeInstr(LNot lnot);
    BytecodeInstr(Array array);
    BytecodeInstr(Tuple tuple);
    BytecodeInstr(Set set);
    BytecodeInstr(Map map);
    BytecodeInstr(Env env);
    BytecodeInstr(Closure closure);
    BytecodeInstr(Formatter formatter);
    BytecodeInstr(AppendFormat append_format);
    BytecodeInstr(FormatResult format_result);
    BytecodeInstr(Copy copy);
    BytecodeInstr(Swap swap);
    BytecodeInstr(Push push);
    BytecodeInstr(Pop pop);
    BytecodeInstr(PopTo pop_to);
    BytecodeInstr(Jmp jmp);
    BytecodeInstr(JmpTrue jmp_true);
    BytecodeInstr(JmpFalse jmp_false);
    BytecodeInstr(Call call);
    BytecodeInstr(LoadMethod load_method);
    BytecodeInstr(CallMethod call_method);
    BytecodeInstr(Return ret);
    BytecodeInstr(AssertFail assert_fail);

    BytecodeOp type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const LoadNull& as_load_null() const;
    const LoadFalse& as_load_false() const;
    const LoadTrue& as_load_true() const;
    const LoadInt& as_load_int() const;
    const LoadFloat& as_load_float() const;
    const LoadParam& as_load_param() const;
    const StoreParam& as_store_param() const;
    const LoadModule& as_load_module() const;
    const StoreModule& as_store_module() const;
    const LoadMember& as_load_member() const;
    const StoreMember& as_store_member() const;
    const LoadTupleMember& as_load_tuple_member() const;
    const StoreTupleMember& as_store_tuple_member() const;
    const LoadIndex& as_load_index() const;
    const StoreIndex& as_store_index() const;
    const LoadClosure& as_load_closure() const;
    const LoadEnv& as_load_env() const;
    const StoreEnv& as_store_env() const;
    const Add& as_add() const;
    const Sub& as_sub() const;
    const Mul& as_mul() const;
    const Div& as_div() const;
    const Mod& as_mod() const;
    const Pow& as_pow() const;
    const UAdd& as_uadd() const;
    const UNeg& as_uneg() const;
    const LSh& as_lsh() const;
    const RSh& as_rsh() const;
    const BAnd& as_band() const;
    const BOr& as_bor() const;
    const BXor& as_bxor() const;
    const BNot& as_bnot() const;
    const Gt& as_gt() const;
    const Gte& as_gte() const;
    const Lt& as_lt() const;
    const Lte& as_lte() const;
    const Eq& as_eq() const;
    const NEq& as_neq() const;
    const LNot& as_lnot() const;
    const Array& as_array() const;
    const Tuple& as_tuple() const;
    const Set& as_set() const;
    const Map& as_map() const;
    const Env& as_env() const;
    const Closure& as_closure() const;
    const Formatter& as_formatter() const;
    const AppendFormat& as_append_format() const;
    const FormatResult& as_format_result() const;
    const Copy& as_copy() const;
    const Swap& as_swap() const;
    const Push& as_push() const;
    const Pop& as_pop() const;
    const PopTo& as_pop_to() const;
    const Jmp& as_jmp() const;
    const JmpTrue& as_jmp_true() const;
    const JmpFalse& as_jmp_false() const;
    const Call& as_call() const;
    const LoadMethod& as_load_method() const;
    const CallMethod& as_call_method() const;
    const Return& as_return() const;
    const AssertFail& as_assert_fail() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto) visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    BytecodeOp type_;
    union {
        LoadNull load_null_;
        LoadFalse load_false_;
        LoadTrue load_true_;
        LoadInt load_int_;
        LoadFloat load_float_;
        LoadParam load_param_;
        StoreParam store_param_;
        LoadModule load_module_;
        StoreModule store_module_;
        LoadMember load_member_;
        StoreMember store_member_;
        LoadTupleMember load_tuple_member_;
        StoreTupleMember store_tuple_member_;
        LoadIndex load_index_;
        StoreIndex store_index_;
        LoadClosure load_closure_;
        LoadEnv load_env_;
        StoreEnv store_env_;
        Add add_;
        Sub sub_;
        Mul mul_;
        Div div_;
        Mod mod_;
        Pow pow_;
        UAdd uadd_;
        UNeg uneg_;
        LSh lsh_;
        RSh rsh_;
        BAnd band_;
        BOr bor_;
        BXor bxor_;
        BNot bnot_;
        Gt gt_;
        Gte gte_;
        Lt lt_;
        Lte lte_;
        Eq eq_;
        NEq neq_;
        LNot lnot_;
        Array array_;
        Tuple tuple_;
        Set set_;
        Map map_;
        Env env_;
        Closure closure_;
        Formatter formatter_;
        AppendFormat append_format_;
        FormatResult format_result_;
        Copy copy_;
        Swap swap_;
        Push push_;
        Pop pop_;
        PopTo pop_to_;
        Jmp jmp_;
        JmpTrue jmp_true_;
        JmpFalse jmp_false_;
        Call call_;
        LoadMethod load_method_;
        CallMethod call_method_;
        Return return_;
        AssertFail assert_fail_;
    };
};
// [[[end]]]

/* [[[cog
    from codegen.unions import implement_inlines
    from codegen.bytecode import Instruction
    implement_inlines(Instruction)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto) BytecodeInstr::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case BytecodeOp::LoadNull:
        return vis.visit_load_null(self.load_null_, std::forward<Args>(args)...);
    case BytecodeOp::LoadFalse:
        return vis.visit_load_false(self.load_false_, std::forward<Args>(args)...);
    case BytecodeOp::LoadTrue:
        return vis.visit_load_true(self.load_true_, std::forward<Args>(args)...);
    case BytecodeOp::LoadInt:
        return vis.visit_load_int(self.load_int_, std::forward<Args>(args)...);
    case BytecodeOp::LoadFloat:
        return vis.visit_load_float(self.load_float_, std::forward<Args>(args)...);
    case BytecodeOp::LoadParam:
        return vis.visit_load_param(self.load_param_, std::forward<Args>(args)...);
    case BytecodeOp::StoreParam:
        return vis.visit_store_param(self.store_param_, std::forward<Args>(args)...);
    case BytecodeOp::LoadModule:
        return vis.visit_load_module(self.load_module_, std::forward<Args>(args)...);
    case BytecodeOp::StoreModule:
        return vis.visit_store_module(self.store_module_, std::forward<Args>(args)...);
    case BytecodeOp::LoadMember:
        return vis.visit_load_member(self.load_member_, std::forward<Args>(args)...);
    case BytecodeOp::StoreMember:
        return vis.visit_store_member(self.store_member_, std::forward<Args>(args)...);
    case BytecodeOp::LoadTupleMember:
        return vis.visit_load_tuple_member(self.load_tuple_member_, std::forward<Args>(args)...);
    case BytecodeOp::StoreTupleMember:
        return vis.visit_store_tuple_member(self.store_tuple_member_, std::forward<Args>(args)...);
    case BytecodeOp::LoadIndex:
        return vis.visit_load_index(self.load_index_, std::forward<Args>(args)...);
    case BytecodeOp::StoreIndex:
        return vis.visit_store_index(self.store_index_, std::forward<Args>(args)...);
    case BytecodeOp::LoadClosure:
        return vis.visit_load_closure(self.load_closure_, std::forward<Args>(args)...);
    case BytecodeOp::LoadEnv:
        return vis.visit_load_env(self.load_env_, std::forward<Args>(args)...);
    case BytecodeOp::StoreEnv:
        return vis.visit_store_env(self.store_env_, std::forward<Args>(args)...);
    case BytecodeOp::Add:
        return vis.visit_add(self.add_, std::forward<Args>(args)...);
    case BytecodeOp::Sub:
        return vis.visit_sub(self.sub_, std::forward<Args>(args)...);
    case BytecodeOp::Mul:
        return vis.visit_mul(self.mul_, std::forward<Args>(args)...);
    case BytecodeOp::Div:
        return vis.visit_div(self.div_, std::forward<Args>(args)...);
    case BytecodeOp::Mod:
        return vis.visit_mod(self.mod_, std::forward<Args>(args)...);
    case BytecodeOp::Pow:
        return vis.visit_pow(self.pow_, std::forward<Args>(args)...);
    case BytecodeOp::UAdd:
        return vis.visit_uadd(self.uadd_, std::forward<Args>(args)...);
    case BytecodeOp::UNeg:
        return vis.visit_uneg(self.uneg_, std::forward<Args>(args)...);
    case BytecodeOp::LSh:
        return vis.visit_lsh(self.lsh_, std::forward<Args>(args)...);
    case BytecodeOp::RSh:
        return vis.visit_rsh(self.rsh_, std::forward<Args>(args)...);
    case BytecodeOp::BAnd:
        return vis.visit_band(self.band_, std::forward<Args>(args)...);
    case BytecodeOp::BOr:
        return vis.visit_bor(self.bor_, std::forward<Args>(args)...);
    case BytecodeOp::BXor:
        return vis.visit_bxor(self.bxor_, std::forward<Args>(args)...);
    case BytecodeOp::BNot:
        return vis.visit_bnot(self.bnot_, std::forward<Args>(args)...);
    case BytecodeOp::Gt:
        return vis.visit_gt(self.gt_, std::forward<Args>(args)...);
    case BytecodeOp::Gte:
        return vis.visit_gte(self.gte_, std::forward<Args>(args)...);
    case BytecodeOp::Lt:
        return vis.visit_lt(self.lt_, std::forward<Args>(args)...);
    case BytecodeOp::Lte:
        return vis.visit_lte(self.lte_, std::forward<Args>(args)...);
    case BytecodeOp::Eq:
        return vis.visit_eq(self.eq_, std::forward<Args>(args)...);
    case BytecodeOp::NEq:
        return vis.visit_neq(self.neq_, std::forward<Args>(args)...);
    case BytecodeOp::LNot:
        return vis.visit_lnot(self.lnot_, std::forward<Args>(args)...);
    case BytecodeOp::Array:
        return vis.visit_array(self.array_, std::forward<Args>(args)...);
    case BytecodeOp::Tuple:
        return vis.visit_tuple(self.tuple_, std::forward<Args>(args)...);
    case BytecodeOp::Set:
        return vis.visit_set(self.set_, std::forward<Args>(args)...);
    case BytecodeOp::Map:
        return vis.visit_map(self.map_, std::forward<Args>(args)...);
    case BytecodeOp::Env:
        return vis.visit_env(self.env_, std::forward<Args>(args)...);
    case BytecodeOp::Closure:
        return vis.visit_closure(self.closure_, std::forward<Args>(args)...);
    case BytecodeOp::Formatter:
        return vis.visit_formatter(self.formatter_, std::forward<Args>(args)...);
    case BytecodeOp::AppendFormat:
        return vis.visit_append_format(self.append_format_, std::forward<Args>(args)...);
    case BytecodeOp::FormatResult:
        return vis.visit_format_result(self.format_result_, std::forward<Args>(args)...);
    case BytecodeOp::Copy:
        return vis.visit_copy(self.copy_, std::forward<Args>(args)...);
    case BytecodeOp::Swap:
        return vis.visit_swap(self.swap_, std::forward<Args>(args)...);
    case BytecodeOp::Push:
        return vis.visit_push(self.push_, std::forward<Args>(args)...);
    case BytecodeOp::Pop:
        return vis.visit_pop(self.pop_, std::forward<Args>(args)...);
    case BytecodeOp::PopTo:
        return vis.visit_pop_to(self.pop_to_, std::forward<Args>(args)...);
    case BytecodeOp::Jmp:
        return vis.visit_jmp(self.jmp_, std::forward<Args>(args)...);
    case BytecodeOp::JmpTrue:
        return vis.visit_jmp_true(self.jmp_true_, std::forward<Args>(args)...);
    case BytecodeOp::JmpFalse:
        return vis.visit_jmp_false(self.jmp_false_, std::forward<Args>(args)...);
    case BytecodeOp::Call:
        return vis.visit_call(self.call_, std::forward<Args>(args)...);
    case BytecodeOp::LoadMethod:
        return vis.visit_load_method(self.load_method_, std::forward<Args>(args)...);
    case BytecodeOp::CallMethod:
        return vis.visit_call_method(self.call_method_, std::forward<Args>(args)...);
    case BytecodeOp::Return:
        return vis.visit_return(self.return_, std::forward<Args>(args)...);
    case BytecodeOp::AssertFail:
        return vis.visit_assert_fail(self.assert_fail_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid BytecodeInstr type.");
}
// [[[end]]]

} // namespace tiro

TIRO_ENABLE_MEMBER_FORMAT(tiro::BytecodeInstr)

#endif // TIRO_BYTECODE_BYTECODE_HPP
