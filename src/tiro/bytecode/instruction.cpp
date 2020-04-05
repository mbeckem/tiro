#include "tiro/bytecode/instruction.hpp"

namespace tiro {

/* [[[cog
    import unions
    import bytecode
    unions.implement_type(bytecode.Instruction)
]]] */
Instruction Instruction::make_load_null(const CompiledLocalID& target) {
    return LoadNull{target};
}

Instruction Instruction::make_load_false(const CompiledLocalID& target) {
    return LoadFalse{target};
}

Instruction Instruction::make_load_true(const CompiledLocalID& target) {
    return LoadTrue{target};
}

Instruction
Instruction::make_load_int(const i64& value, const CompiledLocalID& target) {
    return LoadInt{value, target};
}

Instruction
Instruction::make_load_float(const f64& value, const CompiledLocalID& target) {
    return LoadFloat{value, target};
}

Instruction Instruction::make_load_param(
    const CompiledParamID& source, const CompiledLocalID& target) {
    return LoadParam{source, target};
}

Instruction Instruction::make_store_param(
    const CompiledLocalID& source, const CompiledParamID& target) {
    return StoreParam{source, target};
}

Instruction Instruction::make_load_module(
    const CompiledModuleMemberID& source, const CompiledLocalID& target) {
    return LoadModule{source, target};
}

Instruction Instruction::make_store_module(
    const CompiledLocalID& source, const CompiledModuleMemberID& target) {
    return StoreModule{source, target};
}

Instruction Instruction::make_load_member(const CompiledLocalID& object,
    const CompiledModuleMemberID& name, const CompiledLocalID& target) {
    return LoadMember{object, name, target};
}

Instruction Instruction::make_store_member(const CompiledLocalID& source,
    const CompiledLocalID& object, const CompiledModuleMemberID& name) {
    return StoreMember{source, object, name};
}

Instruction Instruction::make_load_tuple_member(const CompiledLocalID& tuple,
    const u32& index, const CompiledLocalID& target) {
    return LoadTupleMember{tuple, index, target};
}

Instruction Instruction::make_store_tuple_member(const CompiledLocalID& source,
    const CompiledLocalID& tuple, const u32& index) {
    return StoreTupleMember{source, tuple, index};
}

Instruction Instruction::make_load_index(const CompiledLocalID& array,
    const CompiledLocalID& index, const CompiledLocalID& target) {
    return LoadIndex{array, index, target};
}

Instruction Instruction::make_store_index(const CompiledLocalID& source,
    const CompiledLocalID& array, const CompiledLocalID& index) {
    return StoreIndex{source, array, index};
}

Instruction Instruction::make_load_closure(const CompiledLocalID& target) {
    return LoadClosure{target};
}

Instruction Instruction::make_load_env(const CompiledLocalID& env,
    const u32& level, const u32& index, const CompiledLocalID& target) {
    return LoadEnv{env, level, index, target};
}

Instruction Instruction::make_store_env(const CompiledLocalID& source,
    const CompiledLocalID& env, const u32& level, const u32& index) {
    return StoreEnv{source, env, level, index};
}

Instruction Instruction::make_add(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return Add{lhs, rhs, target};
}

Instruction Instruction::make_sub(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return Sub{lhs, rhs, target};
}

Instruction Instruction::make_mul(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return Mul{lhs, rhs, target};
}

Instruction Instruction::make_div(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return Div{lhs, rhs, target};
}

Instruction Instruction::make_mod(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return Mod{lhs, rhs, target};
}

Instruction Instruction::make_pow(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return Pow{lhs, rhs, target};
}

Instruction Instruction::make_uadd(
    const CompiledLocalID& value, const CompiledLocalID& target) {
    return UAdd{value, target};
}

Instruction Instruction::make_uneg(
    const CompiledLocalID& value, const CompiledLocalID& target) {
    return UNeg{value, target};
}

Instruction Instruction::make_lsh(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return LSh{lhs, rhs, target};
}

Instruction Instruction::make_rsh(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return RSh{lhs, rhs, target};
}

Instruction Instruction::make_band(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return BAnd{lhs, rhs, target};
}

Instruction Instruction::make_bor(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return BOr{lhs, rhs, target};
}

Instruction Instruction::make_bxor(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return BXor{lhs, rhs, target};
}

Instruction Instruction::make_bnot(
    const CompiledLocalID& value, const CompiledLocalID& target) {
    return BNot{value, target};
}

Instruction Instruction::make_gt(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return Gt{lhs, rhs, target};
}

Instruction Instruction::make_gte(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return Gte{lhs, rhs, target};
}

Instruction Instruction::make_lt(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return Lt{lhs, rhs, target};
}

Instruction Instruction::make_lte(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return Lte{lhs, rhs, target};
}

Instruction Instruction::make_eq(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return Eq{lhs, rhs, target};
}

Instruction Instruction::make_neq(const CompiledLocalID& lhs,
    const CompiledLocalID& rhs, const CompiledLocalID& target) {
    return NEq{lhs, rhs, target};
}

Instruction Instruction::make_lnot(
    const CompiledLocalID& value, const CompiledLocalID& target) {
    return LNot{value, target};
}

Instruction
Instruction::make_array(const u32& count, const CompiledLocalID& target) {
    return Array{count, target};
}

Instruction
Instruction::make_tuple(const u32& count, const CompiledLocalID& target) {
    return Tuple{count, target};
}

Instruction
Instruction::make_set(const u32& count, const CompiledLocalID& target) {
    return Set{count, target};
}

Instruction
Instruction::make_map(const u32& count, const CompiledLocalID& target) {
    return Map{count, target};
}

Instruction Instruction::make_env(const CompiledLocalID& parent,
    const u32& size, const CompiledLocalID& target) {
    return Env{parent, size, target};
}

Instruction Instruction::make_closure(const CompiledLocalID& tmpl,
    const CompiledLocalID& env, const CompiledLocalID& target) {
    return Closure{tmpl, env, target};
}

Instruction Instruction::make_formatter(const CompiledLocalID& target) {
    return Formatter{target};
}

Instruction Instruction::make_append_format(
    const CompiledLocalID& value, const CompiledLocalID& formatter) {
    return AppendFormat{value, formatter};
}

Instruction Instruction::make_format_result(
    const CompiledLocalID& formatter, const CompiledLocalID& target) {
    return FormatResult{formatter, target};
}

Instruction Instruction::make_copy(
    const CompiledLocalID& source, const CompiledLocalID& target) {
    return Copy{source, target};
}

Instruction
Instruction::make_swap(const CompiledLocalID& a, const CompiledLocalID& b) {
    return Swap{a, b};
}

Instruction Instruction::make_push(const CompiledLocalID& value) {
    return Push{value};
}

Instruction Instruction::make_pop() {
    return Pop{};
}

Instruction Instruction::make_pop_to(const CompiledLocalID& target) {
    return PopTo{target};
}

Instruction Instruction::make_jmp(const CompiledOffset& target) {
    return Jmp{target};
}

Instruction Instruction::make_jmp_true(
    const CompiledLocalID& value, const CompiledOffset& target) {
    return JmpTrue{value, target};
}

Instruction Instruction::make_jmp_false(
    const CompiledLocalID& value, const CompiledOffset& target) {
    return JmpFalse{value, target};
}

Instruction
Instruction::make_call(const CompiledLocalID& function, const u32& count) {
    return Call{function, count};
}

Instruction Instruction::make_load_method(const CompiledLocalID& object,
    const CompiledModuleMemberID& name, const CompiledLocalID& thiz,
    const CompiledLocalID& method) {
    return LoadMethod{object, name, thiz, method};
}

Instruction
Instruction::make_call_method(const CompiledLocalID& method, const u32& count) {
    return CallMethod{method, count};
}

Instruction Instruction::make_return(const CompiledLocalID& value) {
    return Return{value};
}

Instruction Instruction::make_assert_fail(
    const CompiledLocalID& expr, const CompiledLocalID& message) {
    return AssertFail{expr, message};
}

Instruction::Instruction(const LoadNull& load_null)
    : type_(Opcode::LoadNull)
    , load_null_(load_null) {}

Instruction::Instruction(const LoadFalse& load_false)
    : type_(Opcode::LoadFalse)
    , load_false_(load_false) {}

Instruction::Instruction(const LoadTrue& load_true)
    : type_(Opcode::LoadTrue)
    , load_true_(load_true) {}

Instruction::Instruction(const LoadInt& load_int)
    : type_(Opcode::LoadInt)
    , load_int_(load_int) {}

Instruction::Instruction(const LoadFloat& load_float)
    : type_(Opcode::LoadFloat)
    , load_float_(load_float) {}

Instruction::Instruction(const LoadParam& load_param)
    : type_(Opcode::LoadParam)
    , load_param_(load_param) {}

Instruction::Instruction(const StoreParam& store_param)
    : type_(Opcode::StoreParam)
    , store_param_(store_param) {}

Instruction::Instruction(const LoadModule& load_module)
    : type_(Opcode::LoadModule)
    , load_module_(load_module) {}

Instruction::Instruction(const StoreModule& store_module)
    : type_(Opcode::StoreModule)
    , store_module_(store_module) {}

Instruction::Instruction(const LoadMember& load_member)
    : type_(Opcode::LoadMember)
    , load_member_(load_member) {}

Instruction::Instruction(const StoreMember& store_member)
    : type_(Opcode::StoreMember)
    , store_member_(store_member) {}

Instruction::Instruction(const LoadTupleMember& load_tuple_member)
    : type_(Opcode::LoadTupleMember)
    , load_tuple_member_(load_tuple_member) {}

Instruction::Instruction(const StoreTupleMember& store_tuple_member)
    : type_(Opcode::StoreTupleMember)
    , store_tuple_member_(store_tuple_member) {}

Instruction::Instruction(const LoadIndex& load_index)
    : type_(Opcode::LoadIndex)
    , load_index_(load_index) {}

Instruction::Instruction(const StoreIndex& store_index)
    : type_(Opcode::StoreIndex)
    , store_index_(store_index) {}

Instruction::Instruction(const LoadClosure& load_closure)
    : type_(Opcode::LoadClosure)
    , load_closure_(load_closure) {}

Instruction::Instruction(const LoadEnv& load_env)
    : type_(Opcode::LoadEnv)
    , load_env_(load_env) {}

Instruction::Instruction(const StoreEnv& store_env)
    : type_(Opcode::StoreEnv)
    , store_env_(store_env) {}

Instruction::Instruction(const Add& add)
    : type_(Opcode::Add)
    , add_(add) {}

Instruction::Instruction(const Sub& sub)
    : type_(Opcode::Sub)
    , sub_(sub) {}

Instruction::Instruction(const Mul& mul)
    : type_(Opcode::Mul)
    , mul_(mul) {}

Instruction::Instruction(const Div& div)
    : type_(Opcode::Div)
    , div_(div) {}

Instruction::Instruction(const Mod& mod)
    : type_(Opcode::Mod)
    , mod_(mod) {}

Instruction::Instruction(const Pow& pow)
    : type_(Opcode::Pow)
    , pow_(pow) {}

Instruction::Instruction(const UAdd& uadd)
    : type_(Opcode::UAdd)
    , uadd_(uadd) {}

Instruction::Instruction(const UNeg& uneg)
    : type_(Opcode::UNeg)
    , uneg_(uneg) {}

Instruction::Instruction(const LSh& lsh)
    : type_(Opcode::LSh)
    , lsh_(lsh) {}

Instruction::Instruction(const RSh& rsh)
    : type_(Opcode::RSh)
    , rsh_(rsh) {}

Instruction::Instruction(const BAnd& band)
    : type_(Opcode::BAnd)
    , band_(band) {}

Instruction::Instruction(const BOr& bor)
    : type_(Opcode::BOr)
    , bor_(bor) {}

Instruction::Instruction(const BXor& bxor)
    : type_(Opcode::BXor)
    , bxor_(bxor) {}

Instruction::Instruction(const BNot& bnot)
    : type_(Opcode::BNot)
    , bnot_(bnot) {}

Instruction::Instruction(const Gt& gt)
    : type_(Opcode::Gt)
    , gt_(gt) {}

Instruction::Instruction(const Gte& gte)
    : type_(Opcode::Gte)
    , gte_(gte) {}

Instruction::Instruction(const Lt& lt)
    : type_(Opcode::Lt)
    , lt_(lt) {}

Instruction::Instruction(const Lte& lte)
    : type_(Opcode::Lte)
    , lte_(lte) {}

Instruction::Instruction(const Eq& eq)
    : type_(Opcode::Eq)
    , eq_(eq) {}

Instruction::Instruction(const NEq& neq)
    : type_(Opcode::NEq)
    , neq_(neq) {}

Instruction::Instruction(const LNot& lnot)
    : type_(Opcode::LNot)
    , lnot_(lnot) {}

Instruction::Instruction(const Array& array)
    : type_(Opcode::Array)
    , array_(array) {}

Instruction::Instruction(const Tuple& tuple)
    : type_(Opcode::Tuple)
    , tuple_(tuple) {}

Instruction::Instruction(const Set& set)
    : type_(Opcode::Set)
    , set_(set) {}

Instruction::Instruction(const Map& map)
    : type_(Opcode::Map)
    , map_(map) {}

Instruction::Instruction(const Env& env)
    : type_(Opcode::Env)
    , env_(env) {}

Instruction::Instruction(const Closure& closure)
    : type_(Opcode::Closure)
    , closure_(closure) {}

Instruction::Instruction(const Formatter& formatter)
    : type_(Opcode::Formatter)
    , formatter_(formatter) {}

Instruction::Instruction(const AppendFormat& append_format)
    : type_(Opcode::AppendFormat)
    , append_format_(append_format) {}

Instruction::Instruction(const FormatResult& format_result)
    : type_(Opcode::FormatResult)
    , format_result_(format_result) {}

Instruction::Instruction(const Copy& copy)
    : type_(Opcode::Copy)
    , copy_(copy) {}

Instruction::Instruction(const Swap& swap)
    : type_(Opcode::Swap)
    , swap_(swap) {}

Instruction::Instruction(const Push& push)
    : type_(Opcode::Push)
    , push_(push) {}

Instruction::Instruction(const Pop& pop)
    : type_(Opcode::Pop)
    , pop_(pop) {}

Instruction::Instruction(const PopTo& pop_to)
    : type_(Opcode::PopTo)
    , pop_to_(pop_to) {}

Instruction::Instruction(const Jmp& jmp)
    : type_(Opcode::Jmp)
    , jmp_(jmp) {}

Instruction::Instruction(const JmpTrue& jmp_true)
    : type_(Opcode::JmpTrue)
    , jmp_true_(jmp_true) {}

Instruction::Instruction(const JmpFalse& jmp_false)
    : type_(Opcode::JmpFalse)
    , jmp_false_(jmp_false) {}

Instruction::Instruction(const Call& call)
    : type_(Opcode::Call)
    , call_(call) {}

Instruction::Instruction(const LoadMethod& load_method)
    : type_(Opcode::LoadMethod)
    , load_method_(load_method) {}

Instruction::Instruction(const CallMethod& call_method)
    : type_(Opcode::CallMethod)
    , call_method_(call_method) {}

Instruction::Instruction(const Return& ret)
    : type_(Opcode::Return)
    , return_(ret) {}

Instruction::Instruction(const AssertFail& assert_fail)
    : type_(Opcode::AssertFail)
    , assert_fail_(assert_fail) {}

const Instruction::LoadNull& Instruction::as_load_null() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::LoadNull,
        "Bad member access on Instruction: not a LoadNull.");
    return load_null_;
}

const Instruction::LoadFalse& Instruction::as_load_false() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::LoadFalse,
        "Bad member access on Instruction: not a LoadFalse.");
    return load_false_;
}

const Instruction::LoadTrue& Instruction::as_load_true() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::LoadTrue,
        "Bad member access on Instruction: not a LoadTrue.");
    return load_true_;
}

const Instruction::LoadInt& Instruction::as_load_int() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::LoadInt,
        "Bad member access on Instruction: not a LoadInt.");
    return load_int_;
}

const Instruction::LoadFloat& Instruction::as_load_float() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::LoadFloat,
        "Bad member access on Instruction: not a LoadFloat.");
    return load_float_;
}

const Instruction::LoadParam& Instruction::as_load_param() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::LoadParam,
        "Bad member access on Instruction: not a LoadParam.");
    return load_param_;
}

const Instruction::StoreParam& Instruction::as_store_param() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::StoreParam,
        "Bad member access on Instruction: not a StoreParam.");
    return store_param_;
}

const Instruction::LoadModule& Instruction::as_load_module() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::LoadModule,
        "Bad member access on Instruction: not a LoadModule.");
    return load_module_;
}

const Instruction::StoreModule& Instruction::as_store_module() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::StoreModule,
        "Bad member access on Instruction: not a StoreModule.");
    return store_module_;
}

const Instruction::LoadMember& Instruction::as_load_member() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::LoadMember,
        "Bad member access on Instruction: not a LoadMember.");
    return load_member_;
}

const Instruction::StoreMember& Instruction::as_store_member() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::StoreMember,
        "Bad member access on Instruction: not a StoreMember.");
    return store_member_;
}

const Instruction::LoadTupleMember& Instruction::as_load_tuple_member() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::LoadTupleMember,
        "Bad member access on Instruction: not a LoadTupleMember.");
    return load_tuple_member_;
}

const Instruction::StoreTupleMember&
Instruction::as_store_tuple_member() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::StoreTupleMember,
        "Bad member access on Instruction: not a StoreTupleMember.");
    return store_tuple_member_;
}

const Instruction::LoadIndex& Instruction::as_load_index() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::LoadIndex,
        "Bad member access on Instruction: not a LoadIndex.");
    return load_index_;
}

const Instruction::StoreIndex& Instruction::as_store_index() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::StoreIndex,
        "Bad member access on Instruction: not a StoreIndex.");
    return store_index_;
}

const Instruction::LoadClosure& Instruction::as_load_closure() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::LoadClosure,
        "Bad member access on Instruction: not a LoadClosure.");
    return load_closure_;
}

const Instruction::LoadEnv& Instruction::as_load_env() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::LoadEnv,
        "Bad member access on Instruction: not a LoadEnv.");
    return load_env_;
}

const Instruction::StoreEnv& Instruction::as_store_env() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::StoreEnv,
        "Bad member access on Instruction: not a StoreEnv.");
    return store_env_;
}

const Instruction::Add& Instruction::as_add() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Add, "Bad member access on Instruction: not a Add.");
    return add_;
}

const Instruction::Sub& Instruction::as_sub() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Sub, "Bad member access on Instruction: not a Sub.");
    return sub_;
}

const Instruction::Mul& Instruction::as_mul() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Mul, "Bad member access on Instruction: not a Mul.");
    return mul_;
}

const Instruction::Div& Instruction::as_div() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Div, "Bad member access on Instruction: not a Div.");
    return div_;
}

const Instruction::Mod& Instruction::as_mod() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Mod, "Bad member access on Instruction: not a Mod.");
    return mod_;
}

const Instruction::Pow& Instruction::as_pow() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Pow, "Bad member access on Instruction: not a Pow.");
    return pow_;
}

const Instruction::UAdd& Instruction::as_uadd() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::UAdd, "Bad member access on Instruction: not a UAdd.");
    return uadd_;
}

const Instruction::UNeg& Instruction::as_uneg() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::UNeg, "Bad member access on Instruction: not a UNeg.");
    return uneg_;
}

const Instruction::LSh& Instruction::as_lsh() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::LSh, "Bad member access on Instruction: not a LSh.");
    return lsh_;
}

const Instruction::RSh& Instruction::as_rsh() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::RSh, "Bad member access on Instruction: not a RSh.");
    return rsh_;
}

const Instruction::BAnd& Instruction::as_band() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::BAnd, "Bad member access on Instruction: not a BAnd.");
    return band_;
}

const Instruction::BOr& Instruction::as_bor() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::BOr, "Bad member access on Instruction: not a BOr.");
    return bor_;
}

const Instruction::BXor& Instruction::as_bxor() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::BXor, "Bad member access on Instruction: not a BXor.");
    return bxor_;
}

const Instruction::BNot& Instruction::as_bnot() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::BNot, "Bad member access on Instruction: not a BNot.");
    return bnot_;
}

const Instruction::Gt& Instruction::as_gt() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Gt, "Bad member access on Instruction: not a Gt.");
    return gt_;
}

const Instruction::Gte& Instruction::as_gte() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Gte, "Bad member access on Instruction: not a Gte.");
    return gte_;
}

const Instruction::Lt& Instruction::as_lt() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Lt, "Bad member access on Instruction: not a Lt.");
    return lt_;
}

const Instruction::Lte& Instruction::as_lte() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Lte, "Bad member access on Instruction: not a Lte.");
    return lte_;
}

const Instruction::Eq& Instruction::as_eq() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Eq, "Bad member access on Instruction: not a Eq.");
    return eq_;
}

const Instruction::NEq& Instruction::as_neq() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::NEq, "Bad member access on Instruction: not a NEq.");
    return neq_;
}

const Instruction::LNot& Instruction::as_lnot() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::LNot, "Bad member access on Instruction: not a LNot.");
    return lnot_;
}

const Instruction::Array& Instruction::as_array() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::Array,
        "Bad member access on Instruction: not a Array.");
    return array_;
}

const Instruction::Tuple& Instruction::as_tuple() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::Tuple,
        "Bad member access on Instruction: not a Tuple.");
    return tuple_;
}

const Instruction::Set& Instruction::as_set() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Set, "Bad member access on Instruction: not a Set.");
    return set_;
}

const Instruction::Map& Instruction::as_map() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Map, "Bad member access on Instruction: not a Map.");
    return map_;
}

const Instruction::Env& Instruction::as_env() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Env, "Bad member access on Instruction: not a Env.");
    return env_;
}

const Instruction::Closure& Instruction::as_closure() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::Closure,
        "Bad member access on Instruction: not a Closure.");
    return closure_;
}

const Instruction::Formatter& Instruction::as_formatter() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::Formatter,
        "Bad member access on Instruction: not a Formatter.");
    return formatter_;
}

const Instruction::AppendFormat& Instruction::as_append_format() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::AppendFormat,
        "Bad member access on Instruction: not a AppendFormat.");
    return append_format_;
}

const Instruction::FormatResult& Instruction::as_format_result() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::FormatResult,
        "Bad member access on Instruction: not a FormatResult.");
    return format_result_;
}

const Instruction::Copy& Instruction::as_copy() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Copy, "Bad member access on Instruction: not a Copy.");
    return copy_;
}

const Instruction::Swap& Instruction::as_swap() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Swap, "Bad member access on Instruction: not a Swap.");
    return swap_;
}

const Instruction::Push& Instruction::as_push() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Push, "Bad member access on Instruction: not a Push.");
    return push_;
}

const Instruction::Pop& Instruction::as_pop() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Pop, "Bad member access on Instruction: not a Pop.");
    return pop_;
}

const Instruction::PopTo& Instruction::as_pop_to() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::PopTo,
        "Bad member access on Instruction: not a PopTo.");
    return pop_to_;
}

const Instruction::Jmp& Instruction::as_jmp() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Jmp, "Bad member access on Instruction: not a Jmp.");
    return jmp_;
}

const Instruction::JmpTrue& Instruction::as_jmp_true() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::JmpTrue,
        "Bad member access on Instruction: not a JmpTrue.");
    return jmp_true_;
}

const Instruction::JmpFalse& Instruction::as_jmp_false() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::JmpFalse,
        "Bad member access on Instruction: not a JmpFalse.");
    return jmp_false_;
}

const Instruction::Call& Instruction::as_call() const {
    TIRO_DEBUG_ASSERT(
        type_ == Opcode::Call, "Bad member access on Instruction: not a Call.");
    return call_;
}

const Instruction::LoadMethod& Instruction::as_load_method() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::LoadMethod,
        "Bad member access on Instruction: not a LoadMethod.");
    return load_method_;
}

const Instruction::CallMethod& Instruction::as_call_method() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::CallMethod,
        "Bad member access on Instruction: not a CallMethod.");
    return call_method_;
}

const Instruction::Return& Instruction::as_return() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::Return,
        "Bad member access on Instruction: not a Return.");
    return return_;
}

const Instruction::AssertFail& Instruction::as_assert_fail() const {
    TIRO_DEBUG_ASSERT(type_ == Opcode::AssertFail,
        "Bad member access on Instruction: not a AssertFail.");
    return assert_fail_;
}

void Instruction::format(FormatStream& stream) const {
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
            stream.format("LoadInt(value: {}, target: {})", load_int.value,
                load_int.target);
        }

        void visit_load_float([[maybe_unused]] const LoadFloat& load_float) {
            stream.format("LoadFloat(value: {}, target: {})", load_float.value,
                load_float.target);
        }

        void visit_load_param([[maybe_unused]] const LoadParam& load_param) {
            stream.format("LoadParam(source: {}, target: {})",
                load_param.source, load_param.target);
        }

        void visit_store_param([[maybe_unused]] const StoreParam& store_param) {
            stream.format("StoreParam(source: {}, target: {})",
                store_param.source, store_param.target);
        }

        void visit_load_module([[maybe_unused]] const LoadModule& load_module) {
            stream.format("LoadModule(source: {}, target: {})",
                load_module.source, load_module.target);
        }

        void
        visit_store_module([[maybe_unused]] const StoreModule& store_module) {
            stream.format("StoreModule(source: {}, target: {})",
                store_module.source, store_module.target);
        }

        void visit_load_member([[maybe_unused]] const LoadMember& load_member) {
            stream.format("LoadMember(object: {}, name: {}, target: {})",
                load_member.object, load_member.name, load_member.target);
        }

        void
        visit_store_member([[maybe_unused]] const StoreMember& store_member) {
            stream.format("StoreMember(source: {}, object: {}, name: {})",
                store_member.source, store_member.object, store_member.name);
        }

        void visit_load_tuple_member(
            [[maybe_unused]] const LoadTupleMember& load_tuple_member) {
            stream.format("LoadTupleMember(tuple: {}, index: {}, target: {})",
                load_tuple_member.tuple, load_tuple_member.index,
                load_tuple_member.target);
        }

        void visit_store_tuple_member(
            [[maybe_unused]] const StoreTupleMember& store_tuple_member) {
            stream.format("StoreTupleMember(source: {}, tuple: {}, index: {})",
                store_tuple_member.source, store_tuple_member.tuple,
                store_tuple_member.index);
        }

        void visit_load_index([[maybe_unused]] const LoadIndex& load_index) {
            stream.format("LoadIndex(array: {}, index: {}, target: {})",
                load_index.array, load_index.index, load_index.target);
        }

        void visit_store_index([[maybe_unused]] const StoreIndex& store_index) {
            stream.format("StoreIndex(source: {}, array: {}, index: {})",
                store_index.source, store_index.array, store_index.index);
        }

        void
        visit_load_closure([[maybe_unused]] const LoadClosure& load_closure) {
            stream.format("LoadClosure(target: {})", load_closure.target);
        }

        void visit_load_env([[maybe_unused]] const LoadEnv& load_env) {
            stream.format("LoadEnv(env: {}, level: {}, index: {}, target: {})",
                load_env.env, load_env.level, load_env.index, load_env.target);
        }

        void visit_store_env([[maybe_unused]] const StoreEnv& store_env) {
            stream.format("StoreEnv(source: {}, env: {}, level: {}, index: {})",
                store_env.source, store_env.env, store_env.level,
                store_env.index);
        }

        void visit_add([[maybe_unused]] const Add& add) {
            stream.format("Add(lhs: {}, rhs: {}, target: {})", add.lhs, add.rhs,
                add.target);
        }

        void visit_sub([[maybe_unused]] const Sub& sub) {
            stream.format("Sub(lhs: {}, rhs: {}, target: {})", sub.lhs, sub.rhs,
                sub.target);
        }

        void visit_mul([[maybe_unused]] const Mul& mul) {
            stream.format("Mul(lhs: {}, rhs: {}, target: {})", mul.lhs, mul.rhs,
                mul.target);
        }

        void visit_div([[maybe_unused]] const Div& div) {
            stream.format("Div(lhs: {}, rhs: {}, target: {})", div.lhs, div.rhs,
                div.target);
        }

        void visit_mod([[maybe_unused]] const Mod& mod) {
            stream.format("Mod(lhs: {}, rhs: {}, target: {})", mod.lhs, mod.rhs,
                mod.target);
        }

        void visit_pow([[maybe_unused]] const Pow& pow) {
            stream.format("Pow(lhs: {}, rhs: {}, target: {})", pow.lhs, pow.rhs,
                pow.target);
        }

        void visit_uadd([[maybe_unused]] const UAdd& uadd) {
            stream.format(
                "UAdd(value: {}, target: {})", uadd.value, uadd.target);
        }

        void visit_uneg([[maybe_unused]] const UNeg& uneg) {
            stream.format(
                "UNeg(value: {}, target: {})", uneg.value, uneg.target);
        }

        void visit_lsh([[maybe_unused]] const LSh& lsh) {
            stream.format("LSh(lhs: {}, rhs: {}, target: {})", lsh.lhs, lsh.rhs,
                lsh.target);
        }

        void visit_rsh([[maybe_unused]] const RSh& rsh) {
            stream.format("RSh(lhs: {}, rhs: {}, target: {})", rsh.lhs, rsh.rhs,
                rsh.target);
        }

        void visit_band([[maybe_unused]] const BAnd& band) {
            stream.format("BAnd(lhs: {}, rhs: {}, target: {})", band.lhs,
                band.rhs, band.target);
        }

        void visit_bor([[maybe_unused]] const BOr& bor) {
            stream.format("BOr(lhs: {}, rhs: {}, target: {})", bor.lhs, bor.rhs,
                bor.target);
        }

        void visit_bxor([[maybe_unused]] const BXor& bxor) {
            stream.format("BXor(lhs: {}, rhs: {}, target: {})", bxor.lhs,
                bxor.rhs, bxor.target);
        }

        void visit_bnot([[maybe_unused]] const BNot& bnot) {
            stream.format(
                "BNot(value: {}, target: {})", bnot.value, bnot.target);
        }

        void visit_gt([[maybe_unused]] const Gt& gt) {
            stream.format(
                "Gt(lhs: {}, rhs: {}, target: {})", gt.lhs, gt.rhs, gt.target);
        }

        void visit_gte([[maybe_unused]] const Gte& gte) {
            stream.format("Gte(lhs: {}, rhs: {}, target: {})", gte.lhs, gte.rhs,
                gte.target);
        }

        void visit_lt([[maybe_unused]] const Lt& lt) {
            stream.format(
                "Lt(lhs: {}, rhs: {}, target: {})", lt.lhs, lt.rhs, lt.target);
        }

        void visit_lte([[maybe_unused]] const Lte& lte) {
            stream.format("Lte(lhs: {}, rhs: {}, target: {})", lte.lhs, lte.rhs,
                lte.target);
        }

        void visit_eq([[maybe_unused]] const Eq& eq) {
            stream.format(
                "Eq(lhs: {}, rhs: {}, target: {})", eq.lhs, eq.rhs, eq.target);
        }

        void visit_neq([[maybe_unused]] const NEq& neq) {
            stream.format("NEq(lhs: {}, rhs: {}, target: {})", neq.lhs, neq.rhs,
                neq.target);
        }

        void visit_lnot([[maybe_unused]] const LNot& lnot) {
            stream.format(
                "LNot(value: {}, target: {})", lnot.value, lnot.target);
        }

        void visit_array([[maybe_unused]] const Array& array) {
            stream.format(
                "Array(count: {}, target: {})", array.count, array.target);
        }

        void visit_tuple([[maybe_unused]] const Tuple& tuple) {
            stream.format(
                "Tuple(count: {}, target: {})", tuple.count, tuple.target);
        }

        void visit_set([[maybe_unused]] const Set& set) {
            stream.format("Set(count: {}, target: {})", set.count, set.target);
        }

        void visit_map([[maybe_unused]] const Map& map) {
            stream.format("Map(count: {}, target: {})", map.count, map.target);
        }

        void visit_env([[maybe_unused]] const Env& env) {
            stream.format("Env(parent: {}, size: {}, target: {})", env.parent,
                env.size, env.target);
        }

        void visit_closure([[maybe_unused]] const Closure& closure) {
            stream.format("Closure(tmpl: {}, env: {}, target: {})",
                closure.tmpl, closure.env, closure.target);
        }

        void visit_formatter([[maybe_unused]] const Formatter& formatter) {
            stream.format("Formatter(target: {})", formatter.target);
        }

        void visit_append_format(
            [[maybe_unused]] const AppendFormat& append_format) {
            stream.format("AppendFormat(value: {}, formatter: {})",
                append_format.value, append_format.formatter);
        }

        void visit_format_result(
            [[maybe_unused]] const FormatResult& format_result) {
            stream.format("FormatResult(formatter: {}, target: {})",
                format_result.formatter, format_result.target);
        }

        void visit_copy([[maybe_unused]] const Copy& copy) {
            stream.format(
                "Copy(source: {}, target: {})", copy.source, copy.target);
        }

        void visit_swap([[maybe_unused]] const Swap& swap) {
            stream.format("Swap(a: {}, b: {})", swap.a, swap.b);
        }

        void visit_push([[maybe_unused]] const Push& push) {
            stream.format("Push(value: {})", push.value);
        }

        void visit_pop([[maybe_unused]] const Pop& pop) {
            stream.format("Pop");
        }

        void visit_pop_to([[maybe_unused]] const PopTo& pop_to) {
            stream.format("PopTo(target: {})", pop_to.target);
        }

        void visit_jmp([[maybe_unused]] const Jmp& jmp) {
            stream.format("Jmp(target: {})", jmp.target);
        }

        void visit_jmp_true([[maybe_unused]] const JmpTrue& jmp_true) {
            stream.format("JmpTrue(value: {}, target: {})", jmp_true.value,
                jmp_true.target);
        }

        void visit_jmp_false([[maybe_unused]] const JmpFalse& jmp_false) {
            stream.format("JmpFalse(value: {}, target: {})", jmp_false.value,
                jmp_false.target);
        }

        void visit_call([[maybe_unused]] const Call& call) {
            stream.format(
                "Call(function: {}, count: {})", call.function, call.count);
        }

        void visit_load_method([[maybe_unused]] const LoadMethod& load_method) {
            stream.format(
                "LoadMethod(object: {}, name: {}, thiz: {}, method: {})",
                load_method.object, load_method.name, load_method.thiz,
                load_method.method);
        }

        void visit_call_method([[maybe_unused]] const CallMethod& call_method) {
            stream.format("CallMethod(method: {}, count: {})",
                call_method.method, call_method.count);
        }

        void visit_return([[maybe_unused]] const Return& ret) {
            stream.format("Return(value: {})", ret.value);
        }

        void visit_assert_fail([[maybe_unused]] const AssertFail& assert_fail) {
            stream.format("AssertFail(expr: {}, message: {})", assert_fail.expr,
                assert_fail.message);
        }
    };
    visit(FormatVisitor{stream});
}
// [[[end]]]

} // namespace tiro
