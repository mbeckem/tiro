#ifndef TIRO_BYTECODE_BYTECODE_HPP
#define TIRO_BYTECODE_BYTECODE_HPP

#include "tiro/bytecode/fwd.hpp"
#include "tiro/bytecode/opcode.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/core/id_type.hpp"

#include <string_view>

namespace tiro {

TIRO_DEFINE_ID(CompiledLocalID, u32)
TIRO_DEFINE_ID(CompiledParamID, u32)
TIRO_DEFINE_ID(CompiledModuleMemberID, u32)
TIRO_DEFINE_ID(CompiledOffset, u32)

/* [[[cog
    import unions
    import bytecode
    unions.define_type(bytecode.Instruction)
]]] */
/// Represents a bytecode instruction.
class Instruction final {
public:
    struct LoadNull final {
        CompiledLocalID target;

        explicit LoadNull(const CompiledLocalID& target_)
            : target(target_) {}
    };

    struct LoadFalse final {
        CompiledLocalID target;

        explicit LoadFalse(const CompiledLocalID& target_)
            : target(target_) {}
    };

    struct LoadTrue final {
        CompiledLocalID target;

        explicit LoadTrue(const CompiledLocalID& target_)
            : target(target_) {}
    };

    struct LoadInt final {
        i64 value;
        CompiledLocalID target;

        LoadInt(const i64& value_, const CompiledLocalID& target_)
            : value(value_)
            , target(target_) {}
    };

    struct LoadFloat final {
        f64 value;
        CompiledLocalID target;

        LoadFloat(const f64& value_, const CompiledLocalID& target_)
            : value(value_)
            , target(target_) {}
    };

    struct LoadParam final {
        CompiledParamID source;
        CompiledLocalID target;

        LoadParam(
            const CompiledParamID& source_, const CompiledLocalID& target_)
            : source(source_)
            , target(target_) {}
    };

    struct StoreParam final {
        CompiledLocalID source;
        CompiledParamID target;

        StoreParam(
            const CompiledLocalID& source_, const CompiledParamID& target_)
            : source(source_)
            , target(target_) {}
    };

    struct LoadModule final {
        CompiledModuleMemberID source;
        CompiledLocalID target;

        LoadModule(const CompiledModuleMemberID& source_,
            const CompiledLocalID& target_)
            : source(source_)
            , target(target_) {}
    };

    struct StoreModule final {
        CompiledLocalID source;
        CompiledModuleMemberID target;

        StoreModule(const CompiledLocalID& source_,
            const CompiledModuleMemberID& target_)
            : source(source_)
            , target(target_) {}
    };

    struct LoadMember final {
        CompiledLocalID object;
        CompiledModuleMemberID name;
        CompiledLocalID target;

        LoadMember(const CompiledLocalID& object_,
            const CompiledModuleMemberID& name_, const CompiledLocalID& target_)
            : object(object_)
            , name(name_)
            , target(target_) {}
    };

    struct StoreMember final {
        CompiledLocalID source;
        CompiledLocalID object;
        CompiledModuleMemberID name;

        StoreMember(const CompiledLocalID& source_,
            const CompiledLocalID& object_, const CompiledModuleMemberID& name_)
            : source(source_)
            , object(object_)
            , name(name_) {}
    };

    struct LoadTupleMember final {
        CompiledLocalID tuple;
        u32 index;
        CompiledLocalID target;

        LoadTupleMember(const CompiledLocalID& tuple_, const u32& index_,
            const CompiledLocalID& target_)
            : tuple(tuple_)
            , index(index_)
            , target(target_) {}
    };

    struct StoreTupleMember final {
        CompiledLocalID source;
        CompiledLocalID tuple;
        u32 index;

        StoreTupleMember(const CompiledLocalID& source_,
            const CompiledLocalID& tuple_, const u32& index_)
            : source(source_)
            , tuple(tuple_)
            , index(index_) {}
    };

    struct LoadIndex final {
        CompiledLocalID array;
        CompiledLocalID index;
        CompiledLocalID target;

        LoadIndex(const CompiledLocalID& array_, const CompiledLocalID& index_,
            const CompiledLocalID& target_)
            : array(array_)
            , index(index_)
            , target(target_) {}
    };

    struct StoreIndex final {
        CompiledLocalID source;
        CompiledLocalID array;
        CompiledLocalID index;

        StoreIndex(const CompiledLocalID& source_,
            const CompiledLocalID& array_, const CompiledLocalID& index_)
            : source(source_)
            , array(array_)
            , index(index_) {}
    };

    struct LoadClosure final {
        CompiledLocalID target;

        explicit LoadClosure(const CompiledLocalID& target_)
            : target(target_) {}
    };

    struct LoadEnv final {
        CompiledLocalID env;
        u32 level;
        u32 index;
        CompiledLocalID target;

        LoadEnv(const CompiledLocalID& env_, const u32& level_,
            const u32& index_, const CompiledLocalID& target_)
            : env(env_)
            , level(level_)
            , index(index_)
            , target(target_) {}
    };

    struct StoreEnv final {
        CompiledLocalID source;
        CompiledLocalID env;
        u32 level;
        u32 index;

        StoreEnv(const CompiledLocalID& source_, const CompiledLocalID& env_,
            const u32& level_, const u32& index_)
            : source(source_)
            , env(env_)
            , level(level_)
            , index(index_) {}
    };

    struct Add final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        Add(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Sub final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        Sub(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Mul final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        Mul(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Div final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        Div(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Mod final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        Mod(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Pow final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        Pow(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct UAdd final {
        CompiledLocalID value;
        CompiledLocalID target;

        UAdd(const CompiledLocalID& value_, const CompiledLocalID& target_)
            : value(value_)
            , target(target_) {}
    };

    struct UNeg final {
        CompiledLocalID value;
        CompiledLocalID target;

        UNeg(const CompiledLocalID& value_, const CompiledLocalID& target_)
            : value(value_)
            , target(target_) {}
    };

    struct LSh final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        LSh(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct RSh final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        RSh(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct BAnd final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        BAnd(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct BOr final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        BOr(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct BXor final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        BXor(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct BNot final {
        CompiledLocalID value;
        CompiledLocalID target;

        BNot(const CompiledLocalID& value_, const CompiledLocalID& target_)
            : value(value_)
            , target(target_) {}
    };

    struct Gt final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        Gt(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Gte final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        Gte(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Lt final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        Lt(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Lte final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        Lte(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Eq final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        Eq(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct NEq final {
        CompiledLocalID lhs;
        CompiledLocalID rhs;
        CompiledLocalID target;

        NEq(const CompiledLocalID& lhs_, const CompiledLocalID& rhs_,
            const CompiledLocalID& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct LNot final {
        CompiledLocalID value;
        CompiledLocalID target;

        LNot(const CompiledLocalID& value_, const CompiledLocalID& target_)
            : value(value_)
            , target(target_) {}
    };

    struct Array final {
        u32 count;
        CompiledLocalID target;

        Array(const u32& count_, const CompiledLocalID& target_)
            : count(count_)
            , target(target_) {}
    };

    struct Tuple final {
        u32 count;
        CompiledLocalID target;

        Tuple(const u32& count_, const CompiledLocalID& target_)
            : count(count_)
            , target(target_) {}
    };

    struct Set final {
        u32 count;
        CompiledLocalID target;

        Set(const u32& count_, const CompiledLocalID& target_)
            : count(count_)
            , target(target_) {}
    };

    struct Map final {
        u32 count;
        CompiledLocalID target;

        Map(const u32& count_, const CompiledLocalID& target_)
            : count(count_)
            , target(target_) {}
    };

    struct Env final {
        CompiledLocalID parent;
        u32 size;
        CompiledLocalID target;

        Env(const CompiledLocalID& parent_, const u32& size_,
            const CompiledLocalID& target_)
            : parent(parent_)
            , size(size_)
            , target(target_) {}
    };

    struct Closure final {
        CompiledLocalID tmpl;
        CompiledLocalID env;
        CompiledLocalID target;

        Closure(const CompiledLocalID& tmpl_, const CompiledLocalID& env_,
            const CompiledLocalID& target_)
            : tmpl(tmpl_)
            , env(env_)
            , target(target_) {}
    };

    struct Formatter final {
        CompiledLocalID target;

        explicit Formatter(const CompiledLocalID& target_)
            : target(target_) {}
    };

    struct AppendFormat final {
        CompiledLocalID value;
        CompiledLocalID formatter;

        AppendFormat(
            const CompiledLocalID& value_, const CompiledLocalID& formatter_)
            : value(value_)
            , formatter(formatter_) {}
    };

    struct FormatResult final {
        CompiledLocalID formatter;
        CompiledLocalID target;

        FormatResult(
            const CompiledLocalID& formatter_, const CompiledLocalID& target_)
            : formatter(formatter_)
            , target(target_) {}
    };

    struct Copy final {
        CompiledLocalID source;
        CompiledLocalID target;

        Copy(const CompiledLocalID& source_, const CompiledLocalID& target_)
            : source(source_)
            , target(target_) {}
    };

    struct Swap final {
        CompiledLocalID a;
        CompiledLocalID b;

        Swap(const CompiledLocalID& a_, const CompiledLocalID& b_)
            : a(a_)
            , b(b_) {}
    };

    struct Push final {
        CompiledLocalID value;

        explicit Push(const CompiledLocalID& value_)
            : value(value_) {}
    };

    struct Pop final {};

    struct PopTo final {
        CompiledLocalID target;

        explicit PopTo(const CompiledLocalID& target_)
            : target(target_) {}
    };

    struct Jmp final {
        CompiledOffset target;

        explicit Jmp(const CompiledOffset& target_)
            : target(target_) {}
    };

    struct JmpTrue final {
        CompiledLocalID value;
        CompiledOffset target;

        JmpTrue(const CompiledLocalID& value_, const CompiledOffset& target_)
            : value(value_)
            , target(target_) {}
    };

    struct JmpFalse final {
        CompiledLocalID value;
        CompiledOffset target;

        JmpFalse(const CompiledLocalID& value_, const CompiledOffset& target_)
            : value(value_)
            , target(target_) {}
    };

    struct Call final {
        CompiledLocalID function;
        u32 count;

        Call(const CompiledLocalID& function_, const u32& count_)
            : function(function_)
            , count(count_) {}
    };

    struct LoadMethod final {
        CompiledLocalID object;
        CompiledModuleMemberID name;
        CompiledLocalID thiz;
        CompiledLocalID method;

        LoadMethod(const CompiledLocalID& object_,
            const CompiledModuleMemberID& name_, const CompiledLocalID& thiz_,
            const CompiledLocalID& method_)
            : object(object_)
            , name(name_)
            , thiz(thiz_)
            , method(method_) {}
    };

    struct CallMethod final {
        CompiledLocalID method;
        u32 count;

        CallMethod(const CompiledLocalID& method_, const u32& count_)
            : method(method_)
            , count(count_) {}
    };

    struct Return final {
        CompiledLocalID value;

        explicit Return(const CompiledLocalID& value_)
            : value(value_) {}
    };

    struct AssertFail final {
        CompiledLocalID expr;
        CompiledLocalID message;

        AssertFail(
            const CompiledLocalID& expr_, const CompiledLocalID& message_)
            : expr(expr_)
            , message(message_) {}
    };

    static Instruction make_load_null(const CompiledLocalID& target);
    static Instruction make_load_false(const CompiledLocalID& target);
    static Instruction make_load_true(const CompiledLocalID& target);
    static Instruction
    make_load_int(const i64& value, const CompiledLocalID& target);
    static Instruction
    make_load_float(const f64& value, const CompiledLocalID& target);
    static Instruction make_load_param(
        const CompiledParamID& source, const CompiledLocalID& target);
    static Instruction make_store_param(
        const CompiledLocalID& source, const CompiledParamID& target);
    static Instruction make_load_module(
        const CompiledModuleMemberID& source, const CompiledLocalID& target);
    static Instruction make_store_module(
        const CompiledLocalID& source, const CompiledModuleMemberID& target);
    static Instruction make_load_member(const CompiledLocalID& object,
        const CompiledModuleMemberID& name, const CompiledLocalID& target);
    static Instruction make_store_member(const CompiledLocalID& source,
        const CompiledLocalID& object, const CompiledModuleMemberID& name);
    static Instruction make_load_tuple_member(const CompiledLocalID& tuple,
        const u32& index, const CompiledLocalID& target);
    static Instruction make_store_tuple_member(const CompiledLocalID& source,
        const CompiledLocalID& tuple, const u32& index);
    static Instruction make_load_index(const CompiledLocalID& array,
        const CompiledLocalID& index, const CompiledLocalID& target);
    static Instruction make_store_index(const CompiledLocalID& source,
        const CompiledLocalID& array, const CompiledLocalID& index);
    static Instruction make_load_closure(const CompiledLocalID& target);
    static Instruction make_load_env(const CompiledLocalID& env,
        const u32& level, const u32& index, const CompiledLocalID& target);
    static Instruction make_store_env(const CompiledLocalID& source,
        const CompiledLocalID& env, const u32& level, const u32& index);
    static Instruction make_add(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction make_sub(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction make_mul(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction make_div(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction make_mod(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction make_pow(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction
    make_uadd(const CompiledLocalID& value, const CompiledLocalID& target);
    static Instruction
    make_uneg(const CompiledLocalID& value, const CompiledLocalID& target);
    static Instruction make_lsh(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction make_rsh(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction make_band(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction make_bor(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction make_bxor(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction
    make_bnot(const CompiledLocalID& value, const CompiledLocalID& target);
    static Instruction make_gt(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction make_gte(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction make_lt(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction make_lte(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction make_eq(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction make_neq(const CompiledLocalID& lhs,
        const CompiledLocalID& rhs, const CompiledLocalID& target);
    static Instruction
    make_lnot(const CompiledLocalID& value, const CompiledLocalID& target);
    static Instruction
    make_array(const u32& count, const CompiledLocalID& target);
    static Instruction
    make_tuple(const u32& count, const CompiledLocalID& target);
    static Instruction
    make_set(const u32& count, const CompiledLocalID& target);
    static Instruction
    make_map(const u32& count, const CompiledLocalID& target);
    static Instruction make_env(const CompiledLocalID& parent, const u32& size,
        const CompiledLocalID& target);
    static Instruction make_closure(const CompiledLocalID& tmpl,
        const CompiledLocalID& env, const CompiledLocalID& target);
    static Instruction make_formatter(const CompiledLocalID& target);
    static Instruction make_append_format(
        const CompiledLocalID& value, const CompiledLocalID& formatter);
    static Instruction make_format_result(
        const CompiledLocalID& formatter, const CompiledLocalID& target);
    static Instruction
    make_copy(const CompiledLocalID& source, const CompiledLocalID& target);
    static Instruction
    make_swap(const CompiledLocalID& a, const CompiledLocalID& b);
    static Instruction make_push(const CompiledLocalID& value);
    static Instruction make_pop();
    static Instruction make_pop_to(const CompiledLocalID& target);
    static Instruction make_jmp(const CompiledOffset& target);
    static Instruction
    make_jmp_true(const CompiledLocalID& value, const CompiledOffset& target);
    static Instruction
    make_jmp_false(const CompiledLocalID& value, const CompiledOffset& target);
    static Instruction
    make_call(const CompiledLocalID& function, const u32& count);
    static Instruction make_load_method(const CompiledLocalID& object,
        const CompiledModuleMemberID& name, const CompiledLocalID& thiz,
        const CompiledLocalID& method);
    static Instruction
    make_call_method(const CompiledLocalID& method, const u32& count);
    static Instruction make_return(const CompiledLocalID& value);
    static Instruction make_assert_fail(
        const CompiledLocalID& expr, const CompiledLocalID& message);

    Instruction(const LoadNull& load_null);
    Instruction(const LoadFalse& load_false);
    Instruction(const LoadTrue& load_true);
    Instruction(const LoadInt& load_int);
    Instruction(const LoadFloat& load_float);
    Instruction(const LoadParam& load_param);
    Instruction(const StoreParam& store_param);
    Instruction(const LoadModule& load_module);
    Instruction(const StoreModule& store_module);
    Instruction(const LoadMember& load_member);
    Instruction(const StoreMember& store_member);
    Instruction(const LoadTupleMember& load_tuple_member);
    Instruction(const StoreTupleMember& store_tuple_member);
    Instruction(const LoadIndex& load_index);
    Instruction(const StoreIndex& store_index);
    Instruction(const LoadClosure& load_closure);
    Instruction(const LoadEnv& load_env);
    Instruction(const StoreEnv& store_env);
    Instruction(const Add& add);
    Instruction(const Sub& sub);
    Instruction(const Mul& mul);
    Instruction(const Div& div);
    Instruction(const Mod& mod);
    Instruction(const Pow& pow);
    Instruction(const UAdd& uadd);
    Instruction(const UNeg& uneg);
    Instruction(const LSh& lsh);
    Instruction(const RSh& rsh);
    Instruction(const BAnd& band);
    Instruction(const BOr& bor);
    Instruction(const BXor& bxor);
    Instruction(const BNot& bnot);
    Instruction(const Gt& gt);
    Instruction(const Gte& gte);
    Instruction(const Lt& lt);
    Instruction(const Lte& lte);
    Instruction(const Eq& eq);
    Instruction(const NEq& neq);
    Instruction(const LNot& lnot);
    Instruction(const Array& array);
    Instruction(const Tuple& tuple);
    Instruction(const Set& set);
    Instruction(const Map& map);
    Instruction(const Env& env);
    Instruction(const Closure& closure);
    Instruction(const Formatter& formatter);
    Instruction(const AppendFormat& append_format);
    Instruction(const FormatResult& format_result);
    Instruction(const Copy& copy);
    Instruction(const Swap& swap);
    Instruction(const Push& push);
    Instruction(const Pop& pop);
    Instruction(const PopTo& pop_to);
    Instruction(const Jmp& jmp);
    Instruction(const JmpTrue& jmp_true);
    Instruction(const JmpFalse& jmp_false);
    Instruction(const Call& call);
    Instruction(const LoadMethod& load_method);
    Instruction(const CallMethod& call_method);
    Instruction(const Return& ret);
    Instruction(const AssertFail& assert_fail);

    Opcode type() const noexcept { return type_; }

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
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto)
    visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    Opcode type_;
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
    import unions
    import bytecode
    unions.define_inlines(bytecode.Instruction)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto)
Instruction::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case Opcode::LoadNull:
        return vis.visit_load_null(
            self.load_null_, std::forward<Args>(args)...);
    case Opcode::LoadFalse:
        return vis.visit_load_false(
            self.load_false_, std::forward<Args>(args)...);
    case Opcode::LoadTrue:
        return vis.visit_load_true(
            self.load_true_, std::forward<Args>(args)...);
    case Opcode::LoadInt:
        return vis.visit_load_int(self.load_int_, std::forward<Args>(args)...);
    case Opcode::LoadFloat:
        return vis.visit_load_float(
            self.load_float_, std::forward<Args>(args)...);
    case Opcode::LoadParam:
        return vis.visit_load_param(
            self.load_param_, std::forward<Args>(args)...);
    case Opcode::StoreParam:
        return vis.visit_store_param(
            self.store_param_, std::forward<Args>(args)...);
    case Opcode::LoadModule:
        return vis.visit_load_module(
            self.load_module_, std::forward<Args>(args)...);
    case Opcode::StoreModule:
        return vis.visit_store_module(
            self.store_module_, std::forward<Args>(args)...);
    case Opcode::LoadMember:
        return vis.visit_load_member(
            self.load_member_, std::forward<Args>(args)...);
    case Opcode::StoreMember:
        return vis.visit_store_member(
            self.store_member_, std::forward<Args>(args)...);
    case Opcode::LoadTupleMember:
        return vis.visit_load_tuple_member(
            self.load_tuple_member_, std::forward<Args>(args)...);
    case Opcode::StoreTupleMember:
        return vis.visit_store_tuple_member(
            self.store_tuple_member_, std::forward<Args>(args)...);
    case Opcode::LoadIndex:
        return vis.visit_load_index(
            self.load_index_, std::forward<Args>(args)...);
    case Opcode::StoreIndex:
        return vis.visit_store_index(
            self.store_index_, std::forward<Args>(args)...);
    case Opcode::LoadClosure:
        return vis.visit_load_closure(
            self.load_closure_, std::forward<Args>(args)...);
    case Opcode::LoadEnv:
        return vis.visit_load_env(self.load_env_, std::forward<Args>(args)...);
    case Opcode::StoreEnv:
        return vis.visit_store_env(
            self.store_env_, std::forward<Args>(args)...);
    case Opcode::Add:
        return vis.visit_add(self.add_, std::forward<Args>(args)...);
    case Opcode::Sub:
        return vis.visit_sub(self.sub_, std::forward<Args>(args)...);
    case Opcode::Mul:
        return vis.visit_mul(self.mul_, std::forward<Args>(args)...);
    case Opcode::Div:
        return vis.visit_div(self.div_, std::forward<Args>(args)...);
    case Opcode::Mod:
        return vis.visit_mod(self.mod_, std::forward<Args>(args)...);
    case Opcode::Pow:
        return vis.visit_pow(self.pow_, std::forward<Args>(args)...);
    case Opcode::UAdd:
        return vis.visit_uadd(self.uadd_, std::forward<Args>(args)...);
    case Opcode::UNeg:
        return vis.visit_uneg(self.uneg_, std::forward<Args>(args)...);
    case Opcode::LSh:
        return vis.visit_lsh(self.lsh_, std::forward<Args>(args)...);
    case Opcode::RSh:
        return vis.visit_rsh(self.rsh_, std::forward<Args>(args)...);
    case Opcode::BAnd:
        return vis.visit_band(self.band_, std::forward<Args>(args)...);
    case Opcode::BOr:
        return vis.visit_bor(self.bor_, std::forward<Args>(args)...);
    case Opcode::BXor:
        return vis.visit_bxor(self.bxor_, std::forward<Args>(args)...);
    case Opcode::BNot:
        return vis.visit_bnot(self.bnot_, std::forward<Args>(args)...);
    case Opcode::Gt:
        return vis.visit_gt(self.gt_, std::forward<Args>(args)...);
    case Opcode::Gte:
        return vis.visit_gte(self.gte_, std::forward<Args>(args)...);
    case Opcode::Lt:
        return vis.visit_lt(self.lt_, std::forward<Args>(args)...);
    case Opcode::Lte:
        return vis.visit_lte(self.lte_, std::forward<Args>(args)...);
    case Opcode::Eq:
        return vis.visit_eq(self.eq_, std::forward<Args>(args)...);
    case Opcode::NEq:
        return vis.visit_neq(self.neq_, std::forward<Args>(args)...);
    case Opcode::LNot:
        return vis.visit_lnot(self.lnot_, std::forward<Args>(args)...);
    case Opcode::Array:
        return vis.visit_array(self.array_, std::forward<Args>(args)...);
    case Opcode::Tuple:
        return vis.visit_tuple(self.tuple_, std::forward<Args>(args)...);
    case Opcode::Set:
        return vis.visit_set(self.set_, std::forward<Args>(args)...);
    case Opcode::Map:
        return vis.visit_map(self.map_, std::forward<Args>(args)...);
    case Opcode::Env:
        return vis.visit_env(self.env_, std::forward<Args>(args)...);
    case Opcode::Closure:
        return vis.visit_closure(self.closure_, std::forward<Args>(args)...);
    case Opcode::Formatter:
        return vis.visit_formatter(
            self.formatter_, std::forward<Args>(args)...);
    case Opcode::AppendFormat:
        return vis.visit_append_format(
            self.append_format_, std::forward<Args>(args)...);
    case Opcode::FormatResult:
        return vis.visit_format_result(
            self.format_result_, std::forward<Args>(args)...);
    case Opcode::Copy:
        return vis.visit_copy(self.copy_, std::forward<Args>(args)...);
    case Opcode::Swap:
        return vis.visit_swap(self.swap_, std::forward<Args>(args)...);
    case Opcode::Push:
        return vis.visit_push(self.push_, std::forward<Args>(args)...);
    case Opcode::Pop:
        return vis.visit_pop(self.pop_, std::forward<Args>(args)...);
    case Opcode::PopTo:
        return vis.visit_pop_to(self.pop_to_, std::forward<Args>(args)...);
    case Opcode::Jmp:
        return vis.visit_jmp(self.jmp_, std::forward<Args>(args)...);
    case Opcode::JmpTrue:
        return vis.visit_jmp_true(self.jmp_true_, std::forward<Args>(args)...);
    case Opcode::JmpFalse:
        return vis.visit_jmp_false(
            self.jmp_false_, std::forward<Args>(args)...);
    case Opcode::Call:
        return vis.visit_call(self.call_, std::forward<Args>(args)...);
    case Opcode::LoadMethod:
        return vis.visit_load_method(
            self.load_method_, std::forward<Args>(args)...);
    case Opcode::CallMethod:
        return vis.visit_call_method(
            self.call_method_, std::forward<Args>(args)...);
    case Opcode::Return:
        return vis.visit_return(self.return_, std::forward<Args>(args)...);
    case Opcode::AssertFail:
        return vis.visit_assert_fail(
            self.assert_fail_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid Instruction type.");
}
// [[[end]]]

} // namespace tiro

TIRO_ENABLE_MEMBER_FORMAT(tiro::Instruction)

#endif // TIRO_BYTECODE_BYTECODE_HPP
