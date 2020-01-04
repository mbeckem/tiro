#include "hammer/compiler/codegen/code_builder.hpp"

#include <limits>

namespace hammer::compiler {

LabelGroup::LabelGroup(CodeBuilder& builder)
    : builder_(&builder)
    , unique_(builder_->next_unique()) {}

LabelID LabelGroup::gen(std::string_view name) {
    std::string unique_name = fmt::format("{}-{}", name, unique_);
    return builder_->create_label(std::move(unique_name));
}

CodeBuilder::CodeBuilder(std::vector<byte>& out)
    : w_(out) {}

void CodeBuilder::define_label(LabelID label) {
    check_label(label);

    LabelDef& def = labels_[label.value_];
    HAMMER_CHECK(def.location == u32(-1), "Label was already defined.");
    HAMMER_CHECK(w_.pos() < u32(-1), "Code size too large.");
    def.location = w_.pos();
}

void CodeBuilder::finish() {
    for (const auto& use : label_uses_) {
        u32 offset = use.first;
        LabelID label = use.second;

        HAMMER_ASSERT(label.value_ < labels_.size(), "Invalid label id.");
        HAMMER_ASSERT(offset < w_.pos(), "Invalid offset.");

        const LabelDef& def = labels_[label.value_];
        HAMMER_CHECK(def.location != u32(-1),
            "The label {} did not have its location defined.", def.name);

        w_.overwrite_u32(offset, def.location);
    }
}

u32 CodeBuilder::next_unique() {
    HAMMER_CHECK(next_unique_ != 0, "Too many unique values.");
    return next_unique_++;
}

LabelID CodeBuilder::create_label(std::string name) {
    HAMMER_CHECK(labels_.size() < u32(-1), "Too many labels.");
    const u32 index = static_cast<u32>(labels_.size());

    LabelDef& def = labels_.emplace_back();
    def.name = std::move(name);
    return index;
}

void CodeBuilder::check_label(LabelID id) const {
    HAMMER_CHECK(id.value_ != u32(-1) && id.value_ < labels_.size(),
        "Invalid label id.");
}

void CodeBuilder::emit_offset(LabelID label) {
    check_label(label);
    HAMMER_CHECK(w_.pos() < u32(-1), "Code size too large.");

    label_uses_.emplace_back(u32(w_.pos()), label);
    w_.emit_u32(u32(-1)); // Will be overwritten in finish()
}

void CodeBuilder::emit_op(Opcode op) {
    w_.emit_u8(static_cast<u8>(op));
}

void CodeBuilder::load_null() {
    emit_op(Opcode::LoadNull);
}

void CodeBuilder::load_false() {
    emit_op(Opcode::LoadFalse);
}

void CodeBuilder::load_true() {
    emit_op(Opcode::LoadTrue);
}

void CodeBuilder::load_int(i64 i) {
    emit_op(Opcode::LoadInt);
    w_.emit_i64(i);
}

void CodeBuilder::load_float(f64 d) {
    emit_op(Opcode::LoadFloat);
    w_.emit_f64(d);
}

void CodeBuilder::load_param(u32 i) {
    emit_op(Opcode::LoadParam);
    w_.emit_u32(i);
}

void CodeBuilder::store_param(u32 i) {
    emit_op(Opcode::StoreParam);
    w_.emit_u32(i);
}

void CodeBuilder::load_local(u32 i) {
    emit_op(Opcode::LoadLocal);
    w_.emit_u32(i);
}

void CodeBuilder::store_local(u32 i) {
    emit_op(Opcode::StoreLocal);
    w_.emit_u32(i);
}

void CodeBuilder::load_closure() {
    emit_op(Opcode::LoadClosure);
}

void CodeBuilder::load_context(u32 n, u32 i) {
    emit_op(Opcode::LoadContext);
    w_.emit_u32(n);
    w_.emit_u32(i);
}

void CodeBuilder::store_context(u32 n, u32 i) {
    emit_op(Opcode::StoreContext);
    w_.emit_u32(n);
    w_.emit_u32(i);
}

void CodeBuilder::load_member(u32 i) {
    emit_op(Opcode::LoadMember);
    w_.emit_u32(i);
}

void CodeBuilder::store_member(u32 i) {
    emit_op(Opcode::StoreMember);
    w_.emit_u32(i);
}

void CodeBuilder::load_index() {
    emit_op(Opcode::LoadIndex);
}

void CodeBuilder::store_index() {
    emit_op(Opcode::StoreIndex);
}

void CodeBuilder::load_module(u32 i) {
    emit_op(Opcode::LoadModule);
    w_.emit_u32(i);
}

void CodeBuilder::store_module(u32 i) {
    emit_op(Opcode::StoreModule);
    w_.emit_u32(i);
}

void CodeBuilder::load_global(u32 i) {
    emit_op(Opcode::LoadGlobal);
    w_.emit_u32(i);
}

void CodeBuilder::dup() {
    emit_op(Opcode::Dup);
}

void CodeBuilder::pop() {
    emit_op(Opcode::Pop);
}

void CodeBuilder::rot_2() {
    emit_op(Opcode::Rot2);
}

void CodeBuilder::rot_3() {
    emit_op(Opcode::Rot3);
}

void CodeBuilder::rot_4() {
    emit_op(Opcode::Rot4);
}

void CodeBuilder::add() {
    emit_op(Opcode::Add);
}

void CodeBuilder::sub() {
    emit_op(Opcode::Sub);
}

void CodeBuilder::mul() {
    emit_op(Opcode::Mul);
}

void CodeBuilder::div() {
    emit_op(Opcode::Div);
}

void CodeBuilder::mod() {
    emit_op(Opcode::Mod);
}

void CodeBuilder::pow() {
    emit_op(Opcode::Pow);
}

void CodeBuilder::lnot() {
    emit_op(Opcode::LNot);
}

void CodeBuilder::bnot() {
    emit_op(Opcode::BNot);
}

void CodeBuilder::upos() {
    emit_op(Opcode::UPos);
}

void CodeBuilder::uneg() {
    emit_op(Opcode::UNeg);
}

void CodeBuilder::lsh() {
    emit_op(Opcode::LSh);
}

void CodeBuilder::rsh() {
    emit_op(Opcode::RSh);
}

void CodeBuilder::band() {
    emit_op(Opcode::BAnd);
}

void CodeBuilder::bor() {
    emit_op(Opcode::BOr);
}

void CodeBuilder::bxor() {
    emit_op(Opcode::BXor);
}

void CodeBuilder::gt() {
    emit_op(Opcode::Gt);
}

void CodeBuilder::gte() {
    emit_op(Opcode::Gte);
}

void CodeBuilder::lt() {
    emit_op(Opcode::Lt);
}

void CodeBuilder::lte() {
    emit_op(Opcode::Lte);
}

void CodeBuilder::eq() {
    emit_op(Opcode::Eq);
}

void CodeBuilder::neq() {
    emit_op(Opcode::NEq);
}

void CodeBuilder::mk_array(u32 n) {
    emit_op(Opcode::MkArray);
    w_.emit_u32(n);
}

void CodeBuilder::mk_tuple(u32 n) {
    emit_op(Opcode::MkTuple);
    w_.emit_u32(n);
}

void CodeBuilder::mk_set(u32 n) {
    emit_op(Opcode::MkSet);
    w_.emit_u32(n);
}

void CodeBuilder::mk_map(u32 n) {
    emit_op(Opcode::MkMap);
    w_.emit_u32(n);
}

void CodeBuilder::mk_context(u32 n) {
    emit_op(Opcode::MkContext);
    w_.emit_u32(n);
}

void CodeBuilder::mk_closure() {
    emit_op(Opcode::MkClosure);
}

void CodeBuilder::jmp(LabelID target) {
    emit_op(Opcode::Jmp);
    emit_offset(target);
}

void CodeBuilder::jmp_true(LabelID target) {
    emit_op(Opcode::JmpTrue);
    emit_offset(target);
}

void CodeBuilder::jmp_true_pop(LabelID target) {
    emit_op(Opcode::JmpTruePop);
    emit_offset(target);
}

void CodeBuilder::jmp_false(LabelID target) {
    emit_op(Opcode::JmpFalse);
    emit_offset(target);
}

void CodeBuilder::jmp_false_pop(LabelID target) {
    emit_op(Opcode::JmpFalsePop);
    emit_offset(target);
}

void CodeBuilder::call(u32 n) {
    emit_op(Opcode::Call);
    w_.emit_u32(n);
}

void CodeBuilder::load_method(u32 i) {
    emit_op(Opcode::LoadMethod);
    w_.emit_u32(i);
}

void CodeBuilder::call_method(u32 n) {
    emit_op(Opcode::CallMethod);
    w_.emit_u32(n);
}

void CodeBuilder::ret() {
    emit_op(Opcode::Ret);
}

void CodeBuilder::assert_fail() {
    emit_op(Opcode::AssertFail);
}

} // namespace hammer::compiler
