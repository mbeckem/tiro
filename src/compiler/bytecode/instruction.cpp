#include "compiler/bytecode/instruction.hpp"

namespace tiro {

/* [[[cog
    from codegen.unions import implement
    from codegen.bytecode import Instruction
    implement(Instruction)
]]] */
BytecodeInstr BytecodeInstr::make_load_null(const BytecodeRegister& target) {
    return {LoadNull{target}};
}

BytecodeInstr BytecodeInstr::make_load_false(const BytecodeRegister& target) {
    return {LoadFalse{target}};
}

BytecodeInstr BytecodeInstr::make_load_true(const BytecodeRegister& target) {
    return {LoadTrue{target}};
}

BytecodeInstr BytecodeInstr::make_load_int(const i64& constant, const BytecodeRegister& target) {
    return {LoadInt{constant, target}};
}

BytecodeInstr BytecodeInstr::make_load_float(const f64& constant, const BytecodeRegister& target) {
    return {LoadFloat{constant, target}};
}

BytecodeInstr
BytecodeInstr::make_load_param(const BytecodeParam& source, const BytecodeRegister& target) {
    return {LoadParam{source, target}};
}

BytecodeInstr
BytecodeInstr::make_store_param(const BytecodeRegister& source, const BytecodeParam& target) {
    return {StoreParam{source, target}};
}

BytecodeInstr
BytecodeInstr::make_load_module(const BytecodeMemberId& source, const BytecodeRegister& target) {
    return {LoadModule{source, target}};
}

BytecodeInstr
BytecodeInstr::make_store_module(const BytecodeRegister& source, const BytecodeMemberId& target) {
    return {StoreModule{source, target}};
}

BytecodeInstr BytecodeInstr::make_load_member(
    const BytecodeRegister& object, const BytecodeMemberId& name, const BytecodeRegister& target) {
    return {LoadMember{object, name, target}};
}

BytecodeInstr BytecodeInstr::make_store_member(
    const BytecodeRegister& source, const BytecodeRegister& object, const BytecodeMemberId& name) {
    return {StoreMember{source, object, name}};
}

BytecodeInstr BytecodeInstr::make_load_tuple_member(
    const BytecodeRegister& tuple, const u32& index, const BytecodeRegister& target) {
    return {LoadTupleMember{tuple, index, target}};
}

BytecodeInstr BytecodeInstr::make_store_tuple_member(
    const BytecodeRegister& source, const BytecodeRegister& tuple, const u32& index) {
    return {StoreTupleMember{source, tuple, index}};
}

BytecodeInstr BytecodeInstr::make_load_index(
    const BytecodeRegister& array, const BytecodeRegister& index, const BytecodeRegister& target) {
    return {LoadIndex{array, index, target}};
}

BytecodeInstr BytecodeInstr::make_store_index(
    const BytecodeRegister& source, const BytecodeRegister& array, const BytecodeRegister& index) {
    return {StoreIndex{source, array, index}};
}

BytecodeInstr BytecodeInstr::make_load_closure(const BytecodeRegister& target) {
    return {LoadClosure{target}};
}

BytecodeInstr BytecodeInstr::make_load_env(const BytecodeRegister& env, const u32& level,
    const u32& index, const BytecodeRegister& target) {
    return {LoadEnv{env, level, index, target}};
}

BytecodeInstr BytecodeInstr::make_store_env(const BytecodeRegister& source,
    const BytecodeRegister& env, const u32& level, const u32& index) {
    return {StoreEnv{source, env, level, index}};
}

BytecodeInstr BytecodeInstr::make_add(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {Add{lhs, rhs, target}};
}

BytecodeInstr BytecodeInstr::make_sub(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {Sub{lhs, rhs, target}};
}

BytecodeInstr BytecodeInstr::make_mul(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {Mul{lhs, rhs, target}};
}

BytecodeInstr BytecodeInstr::make_div(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {Div{lhs, rhs, target}};
}

BytecodeInstr BytecodeInstr::make_mod(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {Mod{lhs, rhs, target}};
}

BytecodeInstr BytecodeInstr::make_pow(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {Pow{lhs, rhs, target}};
}

BytecodeInstr
BytecodeInstr::make_uadd(const BytecodeRegister& value, const BytecodeRegister& target) {
    return {UAdd{value, target}};
}

BytecodeInstr
BytecodeInstr::make_uneg(const BytecodeRegister& value, const BytecodeRegister& target) {
    return {UNeg{value, target}};
}

BytecodeInstr BytecodeInstr::make_lsh(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {LSh{lhs, rhs, target}};
}

BytecodeInstr BytecodeInstr::make_rsh(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {RSh{lhs, rhs, target}};
}

BytecodeInstr BytecodeInstr::make_band(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {BAnd{lhs, rhs, target}};
}

BytecodeInstr BytecodeInstr::make_bor(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {BOr{lhs, rhs, target}};
}

BytecodeInstr BytecodeInstr::make_bxor(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {BXor{lhs, rhs, target}};
}

BytecodeInstr
BytecodeInstr::make_bnot(const BytecodeRegister& value, const BytecodeRegister& target) {
    return {BNot{value, target}};
}

BytecodeInstr BytecodeInstr::make_gt(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {Gt{lhs, rhs, target}};
}

BytecodeInstr BytecodeInstr::make_gte(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {Gte{lhs, rhs, target}};
}

BytecodeInstr BytecodeInstr::make_lt(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {Lt{lhs, rhs, target}};
}

BytecodeInstr BytecodeInstr::make_lte(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {Lte{lhs, rhs, target}};
}

BytecodeInstr BytecodeInstr::make_eq(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {Eq{lhs, rhs, target}};
}

BytecodeInstr BytecodeInstr::make_neq(
    const BytecodeRegister& lhs, const BytecodeRegister& rhs, const BytecodeRegister& target) {
    return {NEq{lhs, rhs, target}};
}

BytecodeInstr
BytecodeInstr::make_lnot(const BytecodeRegister& value, const BytecodeRegister& target) {
    return {LNot{value, target}};
}

BytecodeInstr BytecodeInstr::make_array(const u32& count, const BytecodeRegister& target) {
    return {Array{count, target}};
}

BytecodeInstr BytecodeInstr::make_tuple(const u32& count, const BytecodeRegister& target) {
    return {Tuple{count, target}};
}

BytecodeInstr BytecodeInstr::make_set(const u32& count, const BytecodeRegister& target) {
    return {Set{count, target}};
}

BytecodeInstr BytecodeInstr::make_map(const u32& count, const BytecodeRegister& target) {
    return {Map{count, target}};
}

BytecodeInstr BytecodeInstr::make_env(
    const BytecodeRegister& parent, const u32& size, const BytecodeRegister& target) {
    return {Env{parent, size, target}};
}

BytecodeInstr BytecodeInstr::make_closure(
    const BytecodeRegister& tmpl, const BytecodeRegister& env, const BytecodeRegister& target) {
    return {Closure{tmpl, env, target}};
}

BytecodeInstr
BytecodeInstr::make_iterator(const BytecodeRegister& container, const BytecodeRegister& target) {
    return {Iterator{container, target}};
}

BytecodeInstr BytecodeInstr::make_iterator_next(const BytecodeRegister& iterator,
    const BytecodeRegister& valid, const BytecodeRegister& value) {
    return {IteratorNext{iterator, valid, value}};
}

BytecodeInstr BytecodeInstr::make_formatter(const BytecodeRegister& target) {
    return {Formatter{target}};
}

BytecodeInstr BytecodeInstr::make_append_format(
    const BytecodeRegister& value, const BytecodeRegister& formatter) {
    return {AppendFormat{value, formatter}};
}

BytecodeInstr BytecodeInstr::make_format_result(
    const BytecodeRegister& formatter, const BytecodeRegister& target) {
    return {FormatResult{formatter, target}};
}

BytecodeInstr
BytecodeInstr::make_copy(const BytecodeRegister& source, const BytecodeRegister& target) {
    return {Copy{source, target}};
}

BytecodeInstr BytecodeInstr::make_swap(const BytecodeRegister& a, const BytecodeRegister& b) {
    return {Swap{a, b}};
}

BytecodeInstr BytecodeInstr::make_push(const BytecodeRegister& value) {
    return {Push{value}};
}

BytecodeInstr BytecodeInstr::make_pop() {
    return {Pop{}};
}

BytecodeInstr BytecodeInstr::make_pop_to(const BytecodeRegister& target) {
    return {PopTo{target}};
}

BytecodeInstr BytecodeInstr::make_jmp(const BytecodeOffset& offset) {
    return {Jmp{offset}};
}

BytecodeInstr
BytecodeInstr::make_jmp_true(const BytecodeRegister& condition, const BytecodeOffset& offset) {
    return {JmpTrue{condition, offset}};
}

BytecodeInstr
BytecodeInstr::make_jmp_false(const BytecodeRegister& condition, const BytecodeOffset& offset) {
    return {JmpFalse{condition, offset}};
}

BytecodeInstr
BytecodeInstr::make_jmp_null(const BytecodeRegister& condition, const BytecodeOffset& offset) {
    return {JmpNull{condition, offset}};
}

BytecodeInstr
BytecodeInstr::make_jmp_not_null(const BytecodeRegister& condition, const BytecodeOffset& offset) {
    return {JmpNotNull{condition, offset}};
}

BytecodeInstr BytecodeInstr::make_call(const BytecodeRegister& function, const u32& count) {
    return {Call{function, count}};
}

BytecodeInstr BytecodeInstr::make_load_method(const BytecodeRegister& object,
    const BytecodeMemberId& name, const BytecodeRegister& thiz, const BytecodeRegister& method) {
    return {LoadMethod{object, name, thiz, method}};
}

BytecodeInstr BytecodeInstr::make_call_method(const BytecodeRegister& method, const u32& count) {
    return {CallMethod{method, count}};
}

BytecodeInstr BytecodeInstr::make_return(const BytecodeRegister& value) {
    return {Return{value}};
}

BytecodeInstr
BytecodeInstr::make_assert_fail(const BytecodeRegister& expr, const BytecodeRegister& message) {
    return {AssertFail{expr, message}};
}

BytecodeInstr::BytecodeInstr(LoadNull load_null)
    : type_(BytecodeOp::LoadNull)
    , load_null_(std::move(load_null)) {}

BytecodeInstr::BytecodeInstr(LoadFalse load_false)
    : type_(BytecodeOp::LoadFalse)
    , load_false_(std::move(load_false)) {}

BytecodeInstr::BytecodeInstr(LoadTrue load_true)
    : type_(BytecodeOp::LoadTrue)
    , load_true_(std::move(load_true)) {}

BytecodeInstr::BytecodeInstr(LoadInt load_int)
    : type_(BytecodeOp::LoadInt)
    , load_int_(std::move(load_int)) {}

BytecodeInstr::BytecodeInstr(LoadFloat load_float)
    : type_(BytecodeOp::LoadFloat)
    , load_float_(std::move(load_float)) {}

BytecodeInstr::BytecodeInstr(LoadParam load_param)
    : type_(BytecodeOp::LoadParam)
    , load_param_(std::move(load_param)) {}

BytecodeInstr::BytecodeInstr(StoreParam store_param)
    : type_(BytecodeOp::StoreParam)
    , store_param_(std::move(store_param)) {}

BytecodeInstr::BytecodeInstr(LoadModule load_module)
    : type_(BytecodeOp::LoadModule)
    , load_module_(std::move(load_module)) {}

BytecodeInstr::BytecodeInstr(StoreModule store_module)
    : type_(BytecodeOp::StoreModule)
    , store_module_(std::move(store_module)) {}

BytecodeInstr::BytecodeInstr(LoadMember load_member)
    : type_(BytecodeOp::LoadMember)
    , load_member_(std::move(load_member)) {}

BytecodeInstr::BytecodeInstr(StoreMember store_member)
    : type_(BytecodeOp::StoreMember)
    , store_member_(std::move(store_member)) {}

BytecodeInstr::BytecodeInstr(LoadTupleMember load_tuple_member)
    : type_(BytecodeOp::LoadTupleMember)
    , load_tuple_member_(std::move(load_tuple_member)) {}

BytecodeInstr::BytecodeInstr(StoreTupleMember store_tuple_member)
    : type_(BytecodeOp::StoreTupleMember)
    , store_tuple_member_(std::move(store_tuple_member)) {}

BytecodeInstr::BytecodeInstr(LoadIndex load_index)
    : type_(BytecodeOp::LoadIndex)
    , load_index_(std::move(load_index)) {}

BytecodeInstr::BytecodeInstr(StoreIndex store_index)
    : type_(BytecodeOp::StoreIndex)
    , store_index_(std::move(store_index)) {}

BytecodeInstr::BytecodeInstr(LoadClosure load_closure)
    : type_(BytecodeOp::LoadClosure)
    , load_closure_(std::move(load_closure)) {}

BytecodeInstr::BytecodeInstr(LoadEnv load_env)
    : type_(BytecodeOp::LoadEnv)
    , load_env_(std::move(load_env)) {}

BytecodeInstr::BytecodeInstr(StoreEnv store_env)
    : type_(BytecodeOp::StoreEnv)
    , store_env_(std::move(store_env)) {}

BytecodeInstr::BytecodeInstr(Add add)
    : type_(BytecodeOp::Add)
    , add_(std::move(add)) {}

BytecodeInstr::BytecodeInstr(Sub sub)
    : type_(BytecodeOp::Sub)
    , sub_(std::move(sub)) {}

BytecodeInstr::BytecodeInstr(Mul mul)
    : type_(BytecodeOp::Mul)
    , mul_(std::move(mul)) {}

BytecodeInstr::BytecodeInstr(Div div)
    : type_(BytecodeOp::Div)
    , div_(std::move(div)) {}

BytecodeInstr::BytecodeInstr(Mod mod)
    : type_(BytecodeOp::Mod)
    , mod_(std::move(mod)) {}

BytecodeInstr::BytecodeInstr(Pow pow)
    : type_(BytecodeOp::Pow)
    , pow_(std::move(pow)) {}

BytecodeInstr::BytecodeInstr(UAdd uadd)
    : type_(BytecodeOp::UAdd)
    , uadd_(std::move(uadd)) {}

BytecodeInstr::BytecodeInstr(UNeg uneg)
    : type_(BytecodeOp::UNeg)
    , uneg_(std::move(uneg)) {}

BytecodeInstr::BytecodeInstr(LSh lsh)
    : type_(BytecodeOp::LSh)
    , lsh_(std::move(lsh)) {}

BytecodeInstr::BytecodeInstr(RSh rsh)
    : type_(BytecodeOp::RSh)
    , rsh_(std::move(rsh)) {}

BytecodeInstr::BytecodeInstr(BAnd band)
    : type_(BytecodeOp::BAnd)
    , band_(std::move(band)) {}

BytecodeInstr::BytecodeInstr(BOr bor)
    : type_(BytecodeOp::BOr)
    , bor_(std::move(bor)) {}

BytecodeInstr::BytecodeInstr(BXor bxor)
    : type_(BytecodeOp::BXor)
    , bxor_(std::move(bxor)) {}

BytecodeInstr::BytecodeInstr(BNot bnot)
    : type_(BytecodeOp::BNot)
    , bnot_(std::move(bnot)) {}

BytecodeInstr::BytecodeInstr(Gt gt)
    : type_(BytecodeOp::Gt)
    , gt_(std::move(gt)) {}

BytecodeInstr::BytecodeInstr(Gte gte)
    : type_(BytecodeOp::Gte)
    , gte_(std::move(gte)) {}

BytecodeInstr::BytecodeInstr(Lt lt)
    : type_(BytecodeOp::Lt)
    , lt_(std::move(lt)) {}

BytecodeInstr::BytecodeInstr(Lte lte)
    : type_(BytecodeOp::Lte)
    , lte_(std::move(lte)) {}

BytecodeInstr::BytecodeInstr(Eq eq)
    : type_(BytecodeOp::Eq)
    , eq_(std::move(eq)) {}

BytecodeInstr::BytecodeInstr(NEq neq)
    : type_(BytecodeOp::NEq)
    , neq_(std::move(neq)) {}

BytecodeInstr::BytecodeInstr(LNot lnot)
    : type_(BytecodeOp::LNot)
    , lnot_(std::move(lnot)) {}

BytecodeInstr::BytecodeInstr(Array array)
    : type_(BytecodeOp::Array)
    , array_(std::move(array)) {}

BytecodeInstr::BytecodeInstr(Tuple tuple)
    : type_(BytecodeOp::Tuple)
    , tuple_(std::move(tuple)) {}

BytecodeInstr::BytecodeInstr(Set set)
    : type_(BytecodeOp::Set)
    , set_(std::move(set)) {}

BytecodeInstr::BytecodeInstr(Map map)
    : type_(BytecodeOp::Map)
    , map_(std::move(map)) {}

BytecodeInstr::BytecodeInstr(Env env)
    : type_(BytecodeOp::Env)
    , env_(std::move(env)) {}

BytecodeInstr::BytecodeInstr(Closure closure)
    : type_(BytecodeOp::Closure)
    , closure_(std::move(closure)) {}

BytecodeInstr::BytecodeInstr(Iterator iterator)
    : type_(BytecodeOp::Iterator)
    , iterator_(std::move(iterator)) {}

BytecodeInstr::BytecodeInstr(IteratorNext iterator_next)
    : type_(BytecodeOp::IteratorNext)
    , iterator_next_(std::move(iterator_next)) {}

BytecodeInstr::BytecodeInstr(Formatter formatter)
    : type_(BytecodeOp::Formatter)
    , formatter_(std::move(formatter)) {}

BytecodeInstr::BytecodeInstr(AppendFormat append_format)
    : type_(BytecodeOp::AppendFormat)
    , append_format_(std::move(append_format)) {}

BytecodeInstr::BytecodeInstr(FormatResult format_result)
    : type_(BytecodeOp::FormatResult)
    , format_result_(std::move(format_result)) {}

BytecodeInstr::BytecodeInstr(Copy copy)
    : type_(BytecodeOp::Copy)
    , copy_(std::move(copy)) {}

BytecodeInstr::BytecodeInstr(Swap swap)
    : type_(BytecodeOp::Swap)
    , swap_(std::move(swap)) {}

BytecodeInstr::BytecodeInstr(Push push)
    : type_(BytecodeOp::Push)
    , push_(std::move(push)) {}

BytecodeInstr::BytecodeInstr(Pop pop)
    : type_(BytecodeOp::Pop)
    , pop_(std::move(pop)) {}

BytecodeInstr::BytecodeInstr(PopTo pop_to)
    : type_(BytecodeOp::PopTo)
    , pop_to_(std::move(pop_to)) {}

BytecodeInstr::BytecodeInstr(Jmp jmp)
    : type_(BytecodeOp::Jmp)
    , jmp_(std::move(jmp)) {}

BytecodeInstr::BytecodeInstr(JmpTrue jmp_true)
    : type_(BytecodeOp::JmpTrue)
    , jmp_true_(std::move(jmp_true)) {}

BytecodeInstr::BytecodeInstr(JmpFalse jmp_false)
    : type_(BytecodeOp::JmpFalse)
    , jmp_false_(std::move(jmp_false)) {}

BytecodeInstr::BytecodeInstr(JmpNull jmp_null)
    : type_(BytecodeOp::JmpNull)
    , jmp_null_(std::move(jmp_null)) {}

BytecodeInstr::BytecodeInstr(JmpNotNull jmp_not_null)
    : type_(BytecodeOp::JmpNotNull)
    , jmp_not_null_(std::move(jmp_not_null)) {}

BytecodeInstr::BytecodeInstr(Call call)
    : type_(BytecodeOp::Call)
    , call_(std::move(call)) {}

BytecodeInstr::BytecodeInstr(LoadMethod load_method)
    : type_(BytecodeOp::LoadMethod)
    , load_method_(std::move(load_method)) {}

BytecodeInstr::BytecodeInstr(CallMethod call_method)
    : type_(BytecodeOp::CallMethod)
    , call_method_(std::move(call_method)) {}

BytecodeInstr::BytecodeInstr(Return ret)
    : type_(BytecodeOp::Return)
    , return_(std::move(ret)) {}

BytecodeInstr::BytecodeInstr(AssertFail assert_fail)
    : type_(BytecodeOp::AssertFail)
    , assert_fail_(std::move(assert_fail)) {}

const BytecodeInstr::LoadNull& BytecodeInstr::as_load_null() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::LoadNull, "Bad member access on BytecodeInstr: not a LoadNull.");
    return load_null_;
}

const BytecodeInstr::LoadFalse& BytecodeInstr::as_load_false() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::LoadFalse, "Bad member access on BytecodeInstr: not a LoadFalse.");
    return load_false_;
}

const BytecodeInstr::LoadTrue& BytecodeInstr::as_load_true() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::LoadTrue, "Bad member access on BytecodeInstr: not a LoadTrue.");
    return load_true_;
}

const BytecodeInstr::LoadInt& BytecodeInstr::as_load_int() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::LoadInt, "Bad member access on BytecodeInstr: not a LoadInt.");
    return load_int_;
}

const BytecodeInstr::LoadFloat& BytecodeInstr::as_load_float() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::LoadFloat, "Bad member access on BytecodeInstr: not a LoadFloat.");
    return load_float_;
}

const BytecodeInstr::LoadParam& BytecodeInstr::as_load_param() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::LoadParam, "Bad member access on BytecodeInstr: not a LoadParam.");
    return load_param_;
}

const BytecodeInstr::StoreParam& BytecodeInstr::as_store_param() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::StoreParam, "Bad member access on BytecodeInstr: not a StoreParam.");
    return store_param_;
}

const BytecodeInstr::LoadModule& BytecodeInstr::as_load_module() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::LoadModule, "Bad member access on BytecodeInstr: not a LoadModule.");
    return load_module_;
}

const BytecodeInstr::StoreModule& BytecodeInstr::as_store_module() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::StoreModule, "Bad member access on BytecodeInstr: not a StoreModule.");
    return store_module_;
}

const BytecodeInstr::LoadMember& BytecodeInstr::as_load_member() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::LoadMember, "Bad member access on BytecodeInstr: not a LoadMember.");
    return load_member_;
}

const BytecodeInstr::StoreMember& BytecodeInstr::as_store_member() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::StoreMember, "Bad member access on BytecodeInstr: not a StoreMember.");
    return store_member_;
}

const BytecodeInstr::LoadTupleMember& BytecodeInstr::as_load_tuple_member() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::LoadTupleMember,
        "Bad member access on BytecodeInstr: not a LoadTupleMember.");
    return load_tuple_member_;
}

const BytecodeInstr::StoreTupleMember& BytecodeInstr::as_store_tuple_member() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::StoreTupleMember,
        "Bad member access on BytecodeInstr: not a StoreTupleMember.");
    return store_tuple_member_;
}

const BytecodeInstr::LoadIndex& BytecodeInstr::as_load_index() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::LoadIndex, "Bad member access on BytecodeInstr: not a LoadIndex.");
    return load_index_;
}

const BytecodeInstr::StoreIndex& BytecodeInstr::as_store_index() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::StoreIndex, "Bad member access on BytecodeInstr: not a StoreIndex.");
    return store_index_;
}

const BytecodeInstr::LoadClosure& BytecodeInstr::as_load_closure() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::LoadClosure, "Bad member access on BytecodeInstr: not a LoadClosure.");
    return load_closure_;
}

const BytecodeInstr::LoadEnv& BytecodeInstr::as_load_env() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::LoadEnv, "Bad member access on BytecodeInstr: not a LoadEnv.");
    return load_env_;
}

const BytecodeInstr::StoreEnv& BytecodeInstr::as_store_env() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::StoreEnv, "Bad member access on BytecodeInstr: not a StoreEnv.");
    return store_env_;
}

const BytecodeInstr::Add& BytecodeInstr::as_add() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Add, "Bad member access on BytecodeInstr: not a Add.");
    return add_;
}

const BytecodeInstr::Sub& BytecodeInstr::as_sub() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Sub, "Bad member access on BytecodeInstr: not a Sub.");
    return sub_;
}

const BytecodeInstr::Mul& BytecodeInstr::as_mul() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Mul, "Bad member access on BytecodeInstr: not a Mul.");
    return mul_;
}

const BytecodeInstr::Div& BytecodeInstr::as_div() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Div, "Bad member access on BytecodeInstr: not a Div.");
    return div_;
}

const BytecodeInstr::Mod& BytecodeInstr::as_mod() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Mod, "Bad member access on BytecodeInstr: not a Mod.");
    return mod_;
}

const BytecodeInstr::Pow& BytecodeInstr::as_pow() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Pow, "Bad member access on BytecodeInstr: not a Pow.");
    return pow_;
}

const BytecodeInstr::UAdd& BytecodeInstr::as_uadd() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::UAdd, "Bad member access on BytecodeInstr: not a UAdd.");
    return uadd_;
}

const BytecodeInstr::UNeg& BytecodeInstr::as_uneg() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::UNeg, "Bad member access on BytecodeInstr: not a UNeg.");
    return uneg_;
}

const BytecodeInstr::LSh& BytecodeInstr::as_lsh() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::LSh, "Bad member access on BytecodeInstr: not a LSh.");
    return lsh_;
}

const BytecodeInstr::RSh& BytecodeInstr::as_rsh() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::RSh, "Bad member access on BytecodeInstr: not a RSh.");
    return rsh_;
}

const BytecodeInstr::BAnd& BytecodeInstr::as_band() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::BAnd, "Bad member access on BytecodeInstr: not a BAnd.");
    return band_;
}

const BytecodeInstr::BOr& BytecodeInstr::as_bor() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::BOr, "Bad member access on BytecodeInstr: not a BOr.");
    return bor_;
}

const BytecodeInstr::BXor& BytecodeInstr::as_bxor() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::BXor, "Bad member access on BytecodeInstr: not a BXor.");
    return bxor_;
}

const BytecodeInstr::BNot& BytecodeInstr::as_bnot() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::BNot, "Bad member access on BytecodeInstr: not a BNot.");
    return bnot_;
}

const BytecodeInstr::Gt& BytecodeInstr::as_gt() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Gt, "Bad member access on BytecodeInstr: not a Gt.");
    return gt_;
}

const BytecodeInstr::Gte& BytecodeInstr::as_gte() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Gte, "Bad member access on BytecodeInstr: not a Gte.");
    return gte_;
}

const BytecodeInstr::Lt& BytecodeInstr::as_lt() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Lt, "Bad member access on BytecodeInstr: not a Lt.");
    return lt_;
}

const BytecodeInstr::Lte& BytecodeInstr::as_lte() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Lte, "Bad member access on BytecodeInstr: not a Lte.");
    return lte_;
}

const BytecodeInstr::Eq& BytecodeInstr::as_eq() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Eq, "Bad member access on BytecodeInstr: not a Eq.");
    return eq_;
}

const BytecodeInstr::NEq& BytecodeInstr::as_neq() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::NEq, "Bad member access on BytecodeInstr: not a NEq.");
    return neq_;
}

const BytecodeInstr::LNot& BytecodeInstr::as_lnot() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::LNot, "Bad member access on BytecodeInstr: not a LNot.");
    return lnot_;
}

const BytecodeInstr::Array& BytecodeInstr::as_array() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::Array, "Bad member access on BytecodeInstr: not a Array.");
    return array_;
}

const BytecodeInstr::Tuple& BytecodeInstr::as_tuple() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::Tuple, "Bad member access on BytecodeInstr: not a Tuple.");
    return tuple_;
}

const BytecodeInstr::Set& BytecodeInstr::as_set() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Set, "Bad member access on BytecodeInstr: not a Set.");
    return set_;
}

const BytecodeInstr::Map& BytecodeInstr::as_map() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Map, "Bad member access on BytecodeInstr: not a Map.");
    return map_;
}

const BytecodeInstr::Env& BytecodeInstr::as_env() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Env, "Bad member access on BytecodeInstr: not a Env.");
    return env_;
}

const BytecodeInstr::Closure& BytecodeInstr::as_closure() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::Closure, "Bad member access on BytecodeInstr: not a Closure.");
    return closure_;
}

const BytecodeInstr::Iterator& BytecodeInstr::as_iterator() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::Iterator, "Bad member access on BytecodeInstr: not a Iterator.");
    return iterator_;
}

const BytecodeInstr::IteratorNext& BytecodeInstr::as_iterator_next() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::IteratorNext,
        "Bad member access on BytecodeInstr: not a IteratorNext.");
    return iterator_next_;
}

const BytecodeInstr::Formatter& BytecodeInstr::as_formatter() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::Formatter, "Bad member access on BytecodeInstr: not a Formatter.");
    return formatter_;
}

const BytecodeInstr::AppendFormat& BytecodeInstr::as_append_format() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::AppendFormat,
        "Bad member access on BytecodeInstr: not a AppendFormat.");
    return append_format_;
}

const BytecodeInstr::FormatResult& BytecodeInstr::as_format_result() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::FormatResult,
        "Bad member access on BytecodeInstr: not a FormatResult.");
    return format_result_;
}

const BytecodeInstr::Copy& BytecodeInstr::as_copy() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Copy, "Bad member access on BytecodeInstr: not a Copy.");
    return copy_;
}

const BytecodeInstr::Swap& BytecodeInstr::as_swap() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Swap, "Bad member access on BytecodeInstr: not a Swap.");
    return swap_;
}

const BytecodeInstr::Push& BytecodeInstr::as_push() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Push, "Bad member access on BytecodeInstr: not a Push.");
    return push_;
}

const BytecodeInstr::Pop& BytecodeInstr::as_pop() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Pop, "Bad member access on BytecodeInstr: not a Pop.");
    return pop_;
}

const BytecodeInstr::PopTo& BytecodeInstr::as_pop_to() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::PopTo, "Bad member access on BytecodeInstr: not a PopTo.");
    return pop_to_;
}

const BytecodeInstr::Jmp& BytecodeInstr::as_jmp() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Jmp, "Bad member access on BytecodeInstr: not a Jmp.");
    return jmp_;
}

const BytecodeInstr::JmpTrue& BytecodeInstr::as_jmp_true() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::JmpTrue, "Bad member access on BytecodeInstr: not a JmpTrue.");
    return jmp_true_;
}

const BytecodeInstr::JmpFalse& BytecodeInstr::as_jmp_false() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::JmpFalse, "Bad member access on BytecodeInstr: not a JmpFalse.");
    return jmp_false_;
}

const BytecodeInstr::JmpNull& BytecodeInstr::as_jmp_null() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::JmpNull, "Bad member access on BytecodeInstr: not a JmpNull.");
    return jmp_null_;
}

const BytecodeInstr::JmpNotNull& BytecodeInstr::as_jmp_not_null() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::JmpNotNull, "Bad member access on BytecodeInstr: not a JmpNotNull.");
    return jmp_not_null_;
}

const BytecodeInstr::Call& BytecodeInstr::as_call() const {
    TIRO_DEBUG_ASSERT(type_ == BytecodeOp::Call, "Bad member access on BytecodeInstr: not a Call.");
    return call_;
}

const BytecodeInstr::LoadMethod& BytecodeInstr::as_load_method() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::LoadMethod, "Bad member access on BytecodeInstr: not a LoadMethod.");
    return load_method_;
}

const BytecodeInstr::CallMethod& BytecodeInstr::as_call_method() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::CallMethod, "Bad member access on BytecodeInstr: not a CallMethod.");
    return call_method_;
}

const BytecodeInstr::Return& BytecodeInstr::as_return() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::Return, "Bad member access on BytecodeInstr: not a Return.");
    return return_;
}

const BytecodeInstr::AssertFail& BytecodeInstr::as_assert_fail() const {
    TIRO_DEBUG_ASSERT(
        type_ == BytecodeOp::AssertFail, "Bad member access on BytecodeInstr: not a AssertFail.");
    return assert_fail_;
}

void BytecodeInstr::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_load_null([[maybe_unused]] const LoadNull& load_null) {
            stream.format("LoadNull(target: {})", load_null.target);
        }

        void visit_load_false([[maybe_unused]] const LoadFalse& load_false) {
            stream.format("LoadFalse(target: {})", load_false.target);
        }

        void visit_load_true([[maybe_unused]] const LoadTrue& load_true) {
            stream.format("LoadTrue(target: {})", load_true.target);
        }

        void visit_load_int([[maybe_unused]] const LoadInt& load_int) {
            stream.format("LoadInt(constant: {}, target: {})", load_int.constant, load_int.target);
        }

        void visit_load_float([[maybe_unused]] const LoadFloat& load_float) {
            stream.format(
                "LoadFloat(constant: {}, target: {})", load_float.constant, load_float.target);
        }

        void visit_load_param([[maybe_unused]] const LoadParam& load_param) {
            stream.format(
                "LoadParam(source: {}, target: {})", load_param.source, load_param.target);
        }

        void visit_store_param([[maybe_unused]] const StoreParam& store_param) {
            stream.format(
                "StoreParam(source: {}, target: {})", store_param.source, store_param.target);
        }

        void visit_load_module([[maybe_unused]] const LoadModule& load_module) {
            stream.format(
                "LoadModule(source: {}, target: {})", load_module.source, load_module.target);
        }

        void visit_store_module([[maybe_unused]] const StoreModule& store_module) {
            stream.format(
                "StoreModule(source: {}, target: {})", store_module.source, store_module.target);
        }

        void visit_load_member([[maybe_unused]] const LoadMember& load_member) {
            stream.format("LoadMember(object: {}, name: {}, target: {})", load_member.object,
                load_member.name, load_member.target);
        }

        void visit_store_member([[maybe_unused]] const StoreMember& store_member) {
            stream.format("StoreMember(source: {}, object: {}, name: {})", store_member.source,
                store_member.object, store_member.name);
        }

        void visit_load_tuple_member([[maybe_unused]] const LoadTupleMember& load_tuple_member) {
            stream.format("LoadTupleMember(tuple: {}, index: {}, target: {})",
                load_tuple_member.tuple, load_tuple_member.index, load_tuple_member.target);
        }

        void visit_store_tuple_member([[maybe_unused]] const StoreTupleMember& store_tuple_member) {
            stream.format("StoreTupleMember(source: {}, tuple: {}, index: {})",
                store_tuple_member.source, store_tuple_member.tuple, store_tuple_member.index);
        }

        void visit_load_index([[maybe_unused]] const LoadIndex& load_index) {
            stream.format("LoadIndex(array: {}, index: {}, target: {})", load_index.array,
                load_index.index, load_index.target);
        }

        void visit_store_index([[maybe_unused]] const StoreIndex& store_index) {
            stream.format("StoreIndex(source: {}, array: {}, index: {})", store_index.source,
                store_index.array, store_index.index);
        }

        void visit_load_closure([[maybe_unused]] const LoadClosure& load_closure) {
            stream.format("LoadClosure(target: {})", load_closure.target);
        }

        void visit_load_env([[maybe_unused]] const LoadEnv& load_env) {
            stream.format("LoadEnv(env: {}, level: {}, index: {}, target: {})", load_env.env,
                load_env.level, load_env.index, load_env.target);
        }

        void visit_store_env([[maybe_unused]] const StoreEnv& store_env) {
            stream.format("StoreEnv(source: {}, env: {}, level: {}, index: {})", store_env.source,
                store_env.env, store_env.level, store_env.index);
        }

        void visit_add([[maybe_unused]] const Add& add) {
            stream.format("Add(lhs: {}, rhs: {}, target: {})", add.lhs, add.rhs, add.target);
        }

        void visit_sub([[maybe_unused]] const Sub& sub) {
            stream.format("Sub(lhs: {}, rhs: {}, target: {})", sub.lhs, sub.rhs, sub.target);
        }

        void visit_mul([[maybe_unused]] const Mul& mul) {
            stream.format("Mul(lhs: {}, rhs: {}, target: {})", mul.lhs, mul.rhs, mul.target);
        }

        void visit_div([[maybe_unused]] const Div& div) {
            stream.format("Div(lhs: {}, rhs: {}, target: {})", div.lhs, div.rhs, div.target);
        }

        void visit_mod([[maybe_unused]] const Mod& mod) {
            stream.format("Mod(lhs: {}, rhs: {}, target: {})", mod.lhs, mod.rhs, mod.target);
        }

        void visit_pow([[maybe_unused]] const Pow& pow) {
            stream.format("Pow(lhs: {}, rhs: {}, target: {})", pow.lhs, pow.rhs, pow.target);
        }

        void visit_uadd([[maybe_unused]] const UAdd& uadd) {
            stream.format("UAdd(value: {}, target: {})", uadd.value, uadd.target);
        }

        void visit_uneg([[maybe_unused]] const UNeg& uneg) {
            stream.format("UNeg(value: {}, target: {})", uneg.value, uneg.target);
        }

        void visit_lsh([[maybe_unused]] const LSh& lsh) {
            stream.format("LSh(lhs: {}, rhs: {}, target: {})", lsh.lhs, lsh.rhs, lsh.target);
        }

        void visit_rsh([[maybe_unused]] const RSh& rsh) {
            stream.format("RSh(lhs: {}, rhs: {}, target: {})", rsh.lhs, rsh.rhs, rsh.target);
        }

        void visit_band([[maybe_unused]] const BAnd& band) {
            stream.format("BAnd(lhs: {}, rhs: {}, target: {})", band.lhs, band.rhs, band.target);
        }

        void visit_bor([[maybe_unused]] const BOr& bor) {
            stream.format("BOr(lhs: {}, rhs: {}, target: {})", bor.lhs, bor.rhs, bor.target);
        }

        void visit_bxor([[maybe_unused]] const BXor& bxor) {
            stream.format("BXor(lhs: {}, rhs: {}, target: {})", bxor.lhs, bxor.rhs, bxor.target);
        }

        void visit_bnot([[maybe_unused]] const BNot& bnot) {
            stream.format("BNot(value: {}, target: {})", bnot.value, bnot.target);
        }

        void visit_gt([[maybe_unused]] const Gt& gt) {
            stream.format("Gt(lhs: {}, rhs: {}, target: {})", gt.lhs, gt.rhs, gt.target);
        }

        void visit_gte([[maybe_unused]] const Gte& gte) {
            stream.format("Gte(lhs: {}, rhs: {}, target: {})", gte.lhs, gte.rhs, gte.target);
        }

        void visit_lt([[maybe_unused]] const Lt& lt) {
            stream.format("Lt(lhs: {}, rhs: {}, target: {})", lt.lhs, lt.rhs, lt.target);
        }

        void visit_lte([[maybe_unused]] const Lte& lte) {
            stream.format("Lte(lhs: {}, rhs: {}, target: {})", lte.lhs, lte.rhs, lte.target);
        }

        void visit_eq([[maybe_unused]] const Eq& eq) {
            stream.format("Eq(lhs: {}, rhs: {}, target: {})", eq.lhs, eq.rhs, eq.target);
        }

        void visit_neq([[maybe_unused]] const NEq& neq) {
            stream.format("NEq(lhs: {}, rhs: {}, target: {})", neq.lhs, neq.rhs, neq.target);
        }

        void visit_lnot([[maybe_unused]] const LNot& lnot) {
            stream.format("LNot(value: {}, target: {})", lnot.value, lnot.target);
        }

        void visit_array([[maybe_unused]] const Array& array) {
            stream.format("Array(count: {}, target: {})", array.count, array.target);
        }

        void visit_tuple([[maybe_unused]] const Tuple& tuple) {
            stream.format("Tuple(count: {}, target: {})", tuple.count, tuple.target);
        }

        void visit_set([[maybe_unused]] const Set& set) {
            stream.format("Set(count: {}, target: {})", set.count, set.target);
        }

        void visit_map([[maybe_unused]] const Map& map) {
            stream.format("Map(count: {}, target: {})", map.count, map.target);
        }

        void visit_env([[maybe_unused]] const Env& env) {
            stream.format(
                "Env(parent: {}, size: {}, target: {})", env.parent, env.size, env.target);
        }

        void visit_closure([[maybe_unused]] const Closure& closure) {
            stream.format("Closure(tmpl: {}, env: {}, target: {})", closure.tmpl, closure.env,
                closure.target);
        }

        void visit_iterator([[maybe_unused]] const Iterator& iterator) {
            stream.format(
                "Iterator(container: {}, target: {})", iterator.container, iterator.target);
        }

        void visit_iterator_next([[maybe_unused]] const IteratorNext& iterator_next) {
            stream.format("IteratorNext(iterator: {}, valid: {}, value: {})",
                iterator_next.iterator, iterator_next.valid, iterator_next.value);
        }

        void visit_formatter([[maybe_unused]] const Formatter& formatter) {
            stream.format("Formatter(target: {})", formatter.target);
        }

        void visit_append_format([[maybe_unused]] const AppendFormat& append_format) {
            stream.format("AppendFormat(value: {}, formatter: {})", append_format.value,
                append_format.formatter);
        }

        void visit_format_result([[maybe_unused]] const FormatResult& format_result) {
            stream.format("FormatResult(formatter: {}, target: {})", format_result.formatter,
                format_result.target);
        }

        void visit_copy([[maybe_unused]] const Copy& copy) {
            stream.format("Copy(source: {}, target: {})", copy.source, copy.target);
        }

        void visit_swap([[maybe_unused]] const Swap& swap) {
            stream.format("Swap(a: {}, b: {})", swap.a, swap.b);
        }

        void visit_push([[maybe_unused]] const Push& push) {
            stream.format("Push(value: {})", push.value);
        }

        void visit_pop([[maybe_unused]] const Pop& pop) { stream.format("Pop"); }

        void visit_pop_to([[maybe_unused]] const PopTo& pop_to) {
            stream.format("PopTo(target: {})", pop_to.target);
        }

        void visit_jmp([[maybe_unused]] const Jmp& jmp) {
            stream.format("Jmp(offset: {})", jmp.offset);
        }

        void visit_jmp_true([[maybe_unused]] const JmpTrue& jmp_true) {
            stream.format(
                "JmpTrue(condition: {}, offset: {})", jmp_true.condition, jmp_true.offset);
        }

        void visit_jmp_false([[maybe_unused]] const JmpFalse& jmp_false) {
            stream.format(
                "JmpFalse(condition: {}, offset: {})", jmp_false.condition, jmp_false.offset);
        }

        void visit_jmp_null([[maybe_unused]] const JmpNull& jmp_null) {
            stream.format(
                "JmpNull(condition: {}, offset: {})", jmp_null.condition, jmp_null.offset);
        }

        void visit_jmp_not_null([[maybe_unused]] const JmpNotNull& jmp_not_null) {
            stream.format("JmpNotNull(condition: {}, offset: {})", jmp_not_null.condition,
                jmp_not_null.offset);
        }

        void visit_call([[maybe_unused]] const Call& call) {
            stream.format("Call(function: {}, count: {})", call.function, call.count);
        }

        void visit_load_method([[maybe_unused]] const LoadMethod& load_method) {
            stream.format("LoadMethod(object: {}, name: {}, thiz: {}, method: {})",
                load_method.object, load_method.name, load_method.thiz, load_method.method);
        }

        void visit_call_method([[maybe_unused]] const CallMethod& call_method) {
            stream.format(
                "CallMethod(method: {}, count: {})", call_method.method, call_method.count);
        }

        void visit_return([[maybe_unused]] const Return& ret) {
            stream.format("Return(value: {})", ret.value);
        }

        void visit_assert_fail([[maybe_unused]] const AssertFail& assert_fail) {
            stream.format(
                "AssertFail(expr: {}, message: {})", assert_fail.expr, assert_fail.message);
        }
    };
    visit(FormatVisitor{stream});
}

// [[[end]]]

} // namespace tiro
