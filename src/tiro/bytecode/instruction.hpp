#ifndef TIRO_BYTECODE_BYTECODE_HPP
#define TIRO_BYTECODE_BYTECODE_HPP

#include "tiro/bytecode/fwd.hpp"
#include "tiro/bytecode/opcode.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/core/id_type.hpp"

#include <string_view>

namespace tiro::compiler::bytecode {

TIRO_DEFINE_ID(LocalIndex, u32)
TIRO_DEFINE_ID(ParamIndex, u32)
TIRO_DEFINE_ID(ModuleIndex, u32)

/* [[[cog
    import unions
    import bytecode
    unions.define_type(bytecode.Instruction)
]]] */
/// Represents a bytecode instruction.
class Instruction final {
public:
    struct LoadNull final {
        LocalIndex target;

        explicit LoadNull(const LocalIndex& target_)
            : target(target_) {}
    };

    struct LoadFalse final {
        LocalIndex target;

        explicit LoadFalse(const LocalIndex& target_)
            : target(target_) {}
    };

    struct LoadTrue final {
        LocalIndex target;

        explicit LoadTrue(const LocalIndex& target_)
            : target(target_) {}
    };

    struct LoadInt final {
        i64 value;
        LocalIndex target;

        LoadInt(const i64& value_, const LocalIndex& target_)
            : value(value_)
            , target(target_) {}
    };

    struct LoadFloat final {
        f64 value;
        LocalIndex target;

        LoadFloat(const f64& value_, const LocalIndex& target_)
            : value(value_)
            , target(target_) {}
    };

    struct LoadParam final {
        ParamIndex source;
        LocalIndex target;

        LoadParam(const ParamIndex& source_, const LocalIndex& target_)
            : source(source_)
            , target(target_) {}
    };

    struct StoreParam final {
        LocalIndex source;
        ParamIndex target;

        StoreParam(const LocalIndex& source_, const ParamIndex& target_)
            : source(source_)
            , target(target_) {}
    };

    struct LoadModule final {
        ModuleIndex source;
        LocalIndex target;

        LoadModule(const ModuleIndex& source_, const LocalIndex& target_)
            : source(source_)
            , target(target_) {}
    };

    struct StoreModule final {
        LocalIndex source;
        ModuleIndex target;

        StoreModule(const LocalIndex& source_, const ModuleIndex& target_)
            : source(source_)
            , target(target_) {}
    };

    struct LoadMember final {
        LocalIndex object;
        ModuleIndex name;
        LocalIndex target;

        LoadMember(const LocalIndex& object_, const ModuleIndex& name_,
            const LocalIndex& target_)
            : object(object_)
            , name(name_)
            , target(target_) {}
    };

    struct StoreMember final {
        LocalIndex source;
        LocalIndex object;
        LocalIndex name;

        StoreMember(const LocalIndex& source_, const LocalIndex& object_,
            const LocalIndex& name_)
            : source(source_)
            , object(object_)
            , name(name_) {}
    };

    struct LoadTupleMember final {
        LocalIndex tuple;
        u32 index;
        LocalIndex target;

        LoadTupleMember(const LocalIndex& tuple_, const u32& index_,
            const LocalIndex& target_)
            : tuple(tuple_)
            , index(index_)
            , target(target_) {}
    };

    struct StoreTupleMember final {
        LocalIndex source;
        LocalIndex tuple;
        u32 index;

        StoreTupleMember(const LocalIndex& source_, const LocalIndex& tuple_,
            const u32& index_)
            : source(source_)
            , tuple(tuple_)
            , index(index_) {}
    };

    struct LoadIndex final {
        LocalIndex array;
        LocalIndex index;
        LocalIndex target;

        LoadIndex(const LocalIndex& array_, const LocalIndex& index_,
            const LocalIndex& target_)
            : array(array_)
            , index(index_)
            , target(target_) {}
    };

    struct StoreIndex final {
        LocalIndex source;
        LocalIndex array;
        LocalIndex index;

        StoreIndex(const LocalIndex& source_, const LocalIndex& array_,
            const LocalIndex& index_)
            : source(source_)
            , array(array_)
            , index(index_) {}
    };

    struct LoadClosure final {
        LocalIndex target;

        explicit LoadClosure(const LocalIndex& target_)
            : target(target_) {}
    };

    struct LoadEnv final {
        LocalIndex env;
        u32 level;
        u32 index;
        LocalIndex target;

        LoadEnv(const LocalIndex& env_, const u32& level_, const u32& index_,
            const LocalIndex& target_)
            : env(env_)
            , level(level_)
            , index(index_)
            , target(target_) {}
    };

    struct StoreEnv final {
        LocalIndex source;
        LocalIndex env;
        u32 level;
        u32 index;

        StoreEnv(const LocalIndex& source_, const LocalIndex& env_,
            const u32& level_, const u32& index_)
            : source(source_)
            , env(env_)
            , level(level_)
            , index(index_) {}
    };

    struct Add final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        Add(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Sub final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        Sub(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Mul final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        Mul(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Div final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        Div(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Mod final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        Mod(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Pow final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        Pow(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct UAdd final {
        LocalIndex value;
        LocalIndex target;

        UAdd(const LocalIndex& value_, const LocalIndex& target_)
            : value(value_)
            , target(target_) {}
    };

    struct UNeg final {
        LocalIndex value;
        LocalIndex target;

        UNeg(const LocalIndex& value_, const LocalIndex& target_)
            : value(value_)
            , target(target_) {}
    };

    struct LSh final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        LSh(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct RSh final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        RSh(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct BAnd final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        BAnd(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct BOr final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        BOr(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct BXor final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        BXor(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct BNot final {
        LocalIndex value;
        LocalIndex target;

        BNot(const LocalIndex& value_, const LocalIndex& target_)
            : value(value_)
            , target(target_) {}
    };

    struct Gt final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        Gt(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Gte final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        Gte(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Lt final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        Lt(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Lte final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        Lte(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Eq final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        Eq(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct Neq final {
        LocalIndex lhs;
        LocalIndex rhs;
        LocalIndex target;

        Neq(const LocalIndex& lhs_, const LocalIndex& rhs_,
            const LocalIndex& target_)
            : lhs(lhs_)
            , rhs(rhs_)
            , target(target_) {}
    };

    struct LNot final {
        LocalIndex value;
        LocalIndex target;

        LNot(const LocalIndex& value_, const LocalIndex& target_)
            : value(value_)
            , target(target_) {}
    };

    struct Array final {
        u32 count;
        LocalIndex target;

        Array(const u32& count_, const LocalIndex& target_)
            : count(count_)
            , target(target_) {}
    };

    struct Tuple final {
        u32 count;
        LocalIndex target;

        Tuple(const u32& count_, const LocalIndex& target_)
            : count(count_)
            , target(target_) {}
    };

    struct Set final {
        u32 count;
        LocalIndex target;

        Set(const u32& count_, const LocalIndex& target_)
            : count(count_)
            , target(target_) {}
    };

    struct Map final {
        u32 count;
        LocalIndex target;

        Map(const u32& count_, const LocalIndex& target_)
            : count(count_)
            , target(target_) {}
    };

    struct Env final {
        LocalIndex parent;
        u32 size;
        LocalIndex target;

        Env(const LocalIndex& parent_, const u32& size_,
            const LocalIndex& target_)
            : parent(parent_)
            , size(size_)
            , target(target_) {}
    };

    struct Closure final {
        ModuleIndex tmpl;
        LocalIndex env;
        LocalIndex target;

        Closure(const ModuleIndex& tmpl_, const LocalIndex& env_,
            const LocalIndex& target_)
            : tmpl(tmpl_)
            , env(env_)
            , target(target_) {}
    };

    struct Formatter final {
        LocalIndex target;

        explicit Formatter(const LocalIndex& target_)
            : target(target_) {}
    };

    struct AppendFormat final {
        LocalIndex value;
        LocalIndex formatter;

        AppendFormat(const LocalIndex& value_, const LocalIndex& formatter_)
            : value(value_)
            , formatter(formatter_) {}
    };

    struct FormatResult final {
        LocalIndex formatter;
        LocalIndex target;

        FormatResult(const LocalIndex& formatter_, const LocalIndex& target_)
            : formatter(formatter_)
            , target(target_) {}
    };

    struct Copy final {
        LocalIndex source;
        LocalIndex target;

        Copy(const LocalIndex& source_, const LocalIndex& target_)
            : source(source_)
            , target(target_) {}
    };

    struct Swap final {
        LocalIndex a;
        LocalIndex b;

        Swap(const LocalIndex& a_, const LocalIndex& b_)
            : a(a_)
            , b(b_) {}
    };

    struct Push final {
        LocalIndex value;

        explicit Push(const LocalIndex& value_)
            : value(value_) {}
    };

    struct Pop final {};

    struct Jmp final {
        u32 offset;

        explicit Jmp(const u32& offset_)
            : offset(offset_) {}
    };

    struct JmpTrue final {
        LocalIndex value;
        u32 offset;

        JmpTrue(const LocalIndex& value_, const u32& offset_)
            : value(value_)
            , offset(offset_) {}
    };

    struct JmpFalse final {
        LocalIndex value;
        u32 offset;

        JmpFalse(const LocalIndex& value_, const u32& offset_)
            : value(value_)
            , offset(offset_) {}
    };

    struct Call final {
        LocalIndex function;
        u32 count;
        LocalIndex target;

        Call(const LocalIndex& function_, const u32& count_,
            const LocalIndex& target_)
            : function(function_)
            , count(count_)
            , target(target_) {}
    };

    struct LoadMethod final {
        LocalIndex object;
        ModuleIndex name;
        LocalIndex thiz;
        LocalIndex method;

        LoadMethod(const LocalIndex& object_, const ModuleIndex& name_,
            const LocalIndex& thiz_, const LocalIndex& method_)
            : object(object_)
            , name(name_)
            , thiz(thiz_)
            , method(method_) {}
    };

    struct CallMethod final {
        LocalIndex thiz;
        LocalIndex method;
        u32 count;

        CallMethod(const LocalIndex& thiz_, const LocalIndex& method_,
            const u32& count_)
            : thiz(thiz_)
            , method(method_)
            , count(count_) {}
    };

    struct AssertFail final {
        LocalIndex expr;
        LocalIndex message;

        AssertFail(const LocalIndex& expr_, const LocalIndex& message_)
            : expr(expr_)
            , message(message_) {}
    };

    static Instruction make_load_null(const LocalIndex& target);
    static Instruction make_load_false(const LocalIndex& target);
    static Instruction make_load_true(const LocalIndex& target);
    static Instruction
    make_load_int(const i64& value, const LocalIndex& target);
    static Instruction
    make_load_float(const f64& value, const LocalIndex& target);
    static Instruction
    make_load_param(const ParamIndex& source, const LocalIndex& target);
    static Instruction
    make_store_param(const LocalIndex& source, const ParamIndex& target);
    static Instruction
    make_load_module(const ModuleIndex& source, const LocalIndex& target);
    static Instruction
    make_store_module(const LocalIndex& source, const ModuleIndex& target);
    static Instruction make_load_member(const LocalIndex& object,
        const ModuleIndex& name, const LocalIndex& target);
    static Instruction make_store_member(const LocalIndex& source,
        const LocalIndex& object, const LocalIndex& name);
    static Instruction make_load_tuple_member(
        const LocalIndex& tuple, const u32& index, const LocalIndex& target);
    static Instruction make_store_tuple_member(
        const LocalIndex& source, const LocalIndex& tuple, const u32& index);
    static Instruction make_load_index(const LocalIndex& array,
        const LocalIndex& index, const LocalIndex& target);
    static Instruction make_store_index(const LocalIndex& source,
        const LocalIndex& array, const LocalIndex& index);
    static Instruction make_load_closure(const LocalIndex& target);
    static Instruction make_load_env(const LocalIndex& env, const u32& level,
        const u32& index, const LocalIndex& target);
    static Instruction make_store_env(const LocalIndex& source,
        const LocalIndex& env, const u32& level, const u32& index);
    static Instruction make_add(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction make_sub(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction make_mul(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction make_div(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction make_mod(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction make_pow(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction
    make_uadd(const LocalIndex& value, const LocalIndex& target);
    static Instruction
    make_uneg(const LocalIndex& value, const LocalIndex& target);
    static Instruction make_lsh(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction make_rsh(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction make_band(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction make_bor(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction make_bxor(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction
    make_bnot(const LocalIndex& value, const LocalIndex& target);
    static Instruction make_gt(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction make_gte(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction make_lt(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction make_lte(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction make_eq(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction make_neq(
        const LocalIndex& lhs, const LocalIndex& rhs, const LocalIndex& target);
    static Instruction
    make_lnot(const LocalIndex& value, const LocalIndex& target);
    static Instruction make_array(const u32& count, const LocalIndex& target);
    static Instruction make_tuple(const u32& count, const LocalIndex& target);
    static Instruction make_set(const u32& count, const LocalIndex& target);
    static Instruction make_map(const u32& count, const LocalIndex& target);
    static Instruction make_env(
        const LocalIndex& parent, const u32& size, const LocalIndex& target);
    static Instruction make_closure(const ModuleIndex& tmpl,
        const LocalIndex& env, const LocalIndex& target);
    static Instruction make_formatter(const LocalIndex& target);
    static Instruction
    make_append_format(const LocalIndex& value, const LocalIndex& formatter);
    static Instruction
    make_format_result(const LocalIndex& formatter, const LocalIndex& target);
    static Instruction
    make_copy(const LocalIndex& source, const LocalIndex& target);
    static Instruction make_swap(const LocalIndex& a, const LocalIndex& b);
    static Instruction make_push(const LocalIndex& value);
    static Instruction make_pop();
    static Instruction make_jmp(const u32& offset);
    static Instruction
    make_jmp_true(const LocalIndex& value, const u32& offset);
    static Instruction
    make_jmp_false(const LocalIndex& value, const u32& offset);
    static Instruction make_call(
        const LocalIndex& function, const u32& count, const LocalIndex& target);
    static Instruction
    make_load_method(const LocalIndex& object, const ModuleIndex& name,
        const LocalIndex& thiz, const LocalIndex& method);
    static Instruction make_call_method(
        const LocalIndex& thiz, const LocalIndex& method, const u32& count);
    static Instruction
    make_assert_fail(const LocalIndex& expr, const LocalIndex& message);

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
    Instruction(const Neq& neq);
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
    Instruction(const Jmp& jmp);
    Instruction(const JmpTrue& jmp_true);
    Instruction(const JmpFalse& jmp_false);
    Instruction(const Call& call);
    Instruction(const LoadMethod& load_method);
    Instruction(const CallMethod& call_method);
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
    const Neq& as_neq() const;
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
    const Jmp& as_jmp() const;
    const JmpTrue& as_jmp_true() const;
    const JmpFalse& as_jmp_false() const;
    const Call& as_call() const;
    const LoadMethod& as_load_method() const;
    const CallMethod& as_call_method() const;
    const AssertFail& as_assert_fail() const;

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

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
        Neq neq_;
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
        Jmp jmp_;
        JmpTrue jmp_true_;
        JmpFalse jmp_false_;
        Call call_;
        LoadMethod load_method_;
        CallMethod call_method_;
        AssertFail assert_fail_;
    };
};
// [[[end]]]

/* [[[cog
    import unions
    import bytecode
    unions.define_inlines(bytecode.Instruction)
]]] */
template<typename Self, typename Visitor>
decltype(auto) Instruction::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case Opcode::LoadNull:
        return vis.visit_load_null(self.load_null_);
    case Opcode::LoadFalse:
        return vis.visit_load_false(self.load_false_);
    case Opcode::LoadTrue:
        return vis.visit_load_true(self.load_true_);
    case Opcode::LoadInt:
        return vis.visit_load_int(self.load_int_);
    case Opcode::LoadFloat:
        return vis.visit_load_float(self.load_float_);
    case Opcode::LoadParam:
        return vis.visit_load_param(self.load_param_);
    case Opcode::StoreParam:
        return vis.visit_store_param(self.store_param_);
    case Opcode::LoadModule:
        return vis.visit_load_module(self.load_module_);
    case Opcode::StoreModule:
        return vis.visit_store_module(self.store_module_);
    case Opcode::LoadMember:
        return vis.visit_load_member(self.load_member_);
    case Opcode::StoreMember:
        return vis.visit_store_member(self.store_member_);
    case Opcode::LoadTupleMember:
        return vis.visit_load_tuple_member(self.load_tuple_member_);
    case Opcode::StoreTupleMember:
        return vis.visit_store_tuple_member(self.store_tuple_member_);
    case Opcode::LoadIndex:
        return vis.visit_load_index(self.load_index_);
    case Opcode::StoreIndex:
        return vis.visit_store_index(self.store_index_);
    case Opcode::LoadClosure:
        return vis.visit_load_closure(self.load_closure_);
    case Opcode::LoadEnv:
        return vis.visit_load_env(self.load_env_);
    case Opcode::StoreEnv:
        return vis.visit_store_env(self.store_env_);
    case Opcode::Add:
        return vis.visit_add(self.add_);
    case Opcode::Sub:
        return vis.visit_sub(self.sub_);
    case Opcode::Mul:
        return vis.visit_mul(self.mul_);
    case Opcode::Div:
        return vis.visit_div(self.div_);
    case Opcode::Mod:
        return vis.visit_mod(self.mod_);
    case Opcode::Pow:
        return vis.visit_pow(self.pow_);
    case Opcode::UAdd:
        return vis.visit_uadd(self.uadd_);
    case Opcode::UNeg:
        return vis.visit_uneg(self.uneg_);
    case Opcode::LSh:
        return vis.visit_lsh(self.lsh_);
    case Opcode::RSh:
        return vis.visit_rsh(self.rsh_);
    case Opcode::BAnd:
        return vis.visit_band(self.band_);
    case Opcode::BOr:
        return vis.visit_bor(self.bor_);
    case Opcode::BXor:
        return vis.visit_bxor(self.bxor_);
    case Opcode::BNot:
        return vis.visit_bnot(self.bnot_);
    case Opcode::Gt:
        return vis.visit_gt(self.gt_);
    case Opcode::Gte:
        return vis.visit_gte(self.gte_);
    case Opcode::Lt:
        return vis.visit_lt(self.lt_);
    case Opcode::Lte:
        return vis.visit_lte(self.lte_);
    case Opcode::Eq:
        return vis.visit_eq(self.eq_);
    case Opcode::Neq:
        return vis.visit_neq(self.neq_);
    case Opcode::LNot:
        return vis.visit_lnot(self.lnot_);
    case Opcode::Array:
        return vis.visit_array(self.array_);
    case Opcode::Tuple:
        return vis.visit_tuple(self.tuple_);
    case Opcode::Set:
        return vis.visit_set(self.set_);
    case Opcode::Map:
        return vis.visit_map(self.map_);
    case Opcode::Env:
        return vis.visit_env(self.env_);
    case Opcode::Closure:
        return vis.visit_closure(self.closure_);
    case Opcode::Formatter:
        return vis.visit_formatter(self.formatter_);
    case Opcode::AppendFormat:
        return vis.visit_append_format(self.append_format_);
    case Opcode::FormatResult:
        return vis.visit_format_result(self.format_result_);
    case Opcode::Copy:
        return vis.visit_copy(self.copy_);
    case Opcode::Swap:
        return vis.visit_swap(self.swap_);
    case Opcode::Push:
        return vis.visit_push(self.push_);
    case Opcode::Pop:
        return vis.visit_pop(self.pop_);
    case Opcode::Jmp:
        return vis.visit_jmp(self.jmp_);
    case Opcode::JmpTrue:
        return vis.visit_jmp_true(self.jmp_true_);
    case Opcode::JmpFalse:
        return vis.visit_jmp_false(self.jmp_false_);
    case Opcode::Call:
        return vis.visit_call(self.call_);
    case Opcode::LoadMethod:
        return vis.visit_load_method(self.load_method_);
    case Opcode::CallMethod:
        return vis.visit_call_method(self.call_method_);
    case Opcode::AssertFail:
        return vis.visit_assert_fail(self.assert_fail_);
    }
    TIRO_UNREACHABLE("Invalid Instruction type.");
}
// [[[end]]]

} // namespace tiro::compiler::bytecode

TIRO_ENABLE_MEMBER_FORMAT(tiro::compiler::bytecode::Instruction)

#endif // TIRO_BYTECODE_BYTECODE_HPP
