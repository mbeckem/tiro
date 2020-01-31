#include "tiro/codegen/code_builder.hpp"

#include <limits>

namespace tiro::compiler {

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
    TIRO_CHECK(def.location == u32(-1), "Label was already defined.");
    TIRO_CHECK(w_.pos() < u32(-1), "Code size too large.");
    def.location = w_.pos();
}

void CodeBuilder::finish() {
    for (const auto& use : label_uses_) {
        u32 offset = use.first;
        LabelID label = use.second;

        TIRO_ASSERT(label.value_ < labels_.size(), "Invalid label id.");
        TIRO_ASSERT(offset < w_.pos(), "Invalid offset.");

        const LabelDef& def = labels_[label.value_];
        TIRO_CHECK(def.location != u32(-1),
            "The label {} did not have its location defined.", def.name);

        w_.overwrite_u32(offset, def.location);
    }
}

u32 CodeBuilder::next_unique() {
    TIRO_CHECK(next_unique_ != 0, "Too many unique values.");
    return (next_unique_++).value();
}

LabelID CodeBuilder::create_label(std::string name) {
    TIRO_CHECK(labels_.size() < u32(-1), "Too many labels.");
    const u32 index = static_cast<u32>(labels_.size());

    LabelDef& def = labels_.emplace_back();
    def.name = std::move(name);
    return index;
}

void CodeBuilder::check_label(LabelID id) const {
    TIRO_CHECK(id.value_ != u32(-1) && id.value_ < labels_.size(),
        "Invalid label id.");
}

void CodeBuilder::emit_offset(LabelID label) {
    check_label(label);
    TIRO_CHECK(w_.pos() < u32(-1), "Code size too large.");

    label_uses_.emplace_back(u32(w_.pos()), label);
    w_.emit_u32(u32(-1)); // Will be overwritten in finish()
}

void CodeBuilder::emit_op(Opcode op) {
    w_.emit_u8(static_cast<u8>(op));
}

void CodeBuilder::load_null() {
    emit_op(Opcode::LoadNull);
    balance_ += 1;
}

void CodeBuilder::load_false() {
    emit_op(Opcode::LoadFalse);
    balance_ += 1;
}

void CodeBuilder::load_true() {
    emit_op(Opcode::LoadTrue);
    balance_ += 1;
}

void CodeBuilder::load_int(i64 i) {
    emit_op(Opcode::LoadInt);
    w_.emit_i64(i);
    balance_ += 1;
}

void CodeBuilder::load_float(f64 d) {
    emit_op(Opcode::LoadFloat);
    w_.emit_f64(d);
    balance_ += 1;
}

void CodeBuilder::load_param(u32 i) {
    emit_op(Opcode::LoadParam);
    w_.emit_u32(i);
    balance_ += 1;
}

void CodeBuilder::store_param(u32 i) {
    emit_op(Opcode::StoreParam);
    w_.emit_u32(i);
    balance_ -= 1;
}

void CodeBuilder::load_local(u32 i) {
    emit_op(Opcode::LoadLocal);
    w_.emit_u32(i);
    balance_ += 1;
}

void CodeBuilder::store_local(u32 i) {
    emit_op(Opcode::StoreLocal);
    w_.emit_u32(i);
    balance_ -= 1;
}

void CodeBuilder::load_closure() {
    emit_op(Opcode::LoadClosure);
    balance_ += 1;
}

void CodeBuilder::load_context(u32 n, u32 i) {
    emit_op(Opcode::LoadContext);
    w_.emit_u32(n);
    w_.emit_u32(i);
    // balance_ += 0;
}

void CodeBuilder::store_context(u32 n, u32 i) {
    emit_op(Opcode::StoreContext);
    w_.emit_u32(n);
    w_.emit_u32(i);
    balance_ -= 2;
}

void CodeBuilder::load_member(u32 i) {
    emit_op(Opcode::LoadMember);
    w_.emit_u32(i);
    // balance_ += 0;
}

void CodeBuilder::store_member(u32 i) {
    emit_op(Opcode::StoreMember);
    w_.emit_u32(i);
    balance_ -= 2;
}

void CodeBuilder::load_tuple_member(u32 i) {
    emit_op(Opcode::LoadTupleMember);
    w_.emit_u32(i);
    // balance_ += 0;
}

void CodeBuilder::store_tuple_member(u32 i) {
    emit_op(Opcode::StoreTupleMember);
    w_.emit_u32(i);
    balance_ -= 2;
}

void CodeBuilder::load_index() {
    emit_op(Opcode::LoadIndex);
    balance_ -= 1;
}

void CodeBuilder::store_index() {
    emit_op(Opcode::StoreIndex);
    balance_ -= 3;
}

void CodeBuilder::load_module(u32 i) {
    emit_op(Opcode::LoadModule);
    w_.emit_u32(i);
    balance_ += 1;
}

void CodeBuilder::store_module(u32 i) {
    emit_op(Opcode::StoreModule);
    w_.emit_u32(i);
    balance_ -= 1;
}

void CodeBuilder::load_global(u32 i) {
    emit_op(Opcode::LoadGlobal);
    w_.emit_u32(i);
    balance_ += 1;
}

void CodeBuilder::dup() {
    emit_op(Opcode::Dup);
    balance_ += 1;
}

void CodeBuilder::pop() {
    emit_op(Opcode::Pop);
    balance_ -= 1;
}

void CodeBuilder::pop_n(u32 n) {
    emit_op(Opcode::PopN);
    w_.emit_u32(n);
    balance_ -= n;
}

void CodeBuilder::rot_2() {
    emit_op(Opcode::Rot2);
    // balance_ += 0;
}

void CodeBuilder::rot_3() {
    emit_op(Opcode::Rot3);
    // balance_ += 0;
}

void CodeBuilder::rot_4() {
    emit_op(Opcode::Rot4);
    // balance_ += 0;
}

void CodeBuilder::add() {
    emit_op(Opcode::Add);
    balance_ -= 1;
}

void CodeBuilder::sub() {
    emit_op(Opcode::Sub);
    balance_ -= 1;
}

void CodeBuilder::mul() {
    emit_op(Opcode::Mul);
    balance_ -= 1;
}

void CodeBuilder::div() {
    emit_op(Opcode::Div);
    balance_ -= 1;
}

void CodeBuilder::mod() {
    emit_op(Opcode::Mod);
    balance_ -= 1;
}

void CodeBuilder::pow() {
    emit_op(Opcode::Pow);
    balance_ -= 1;
}

void CodeBuilder::lnot() {
    emit_op(Opcode::LNot);
    // balance_ += 0;
}

void CodeBuilder::bnot() {
    emit_op(Opcode::BNot);
    // balance_ += 0;
}

void CodeBuilder::upos() {
    emit_op(Opcode::UPos);
    // balance_ += 0;
}

void CodeBuilder::uneg() {
    emit_op(Opcode::UNeg);
    // balance_ += 0;
}

void CodeBuilder::lsh() {
    emit_op(Opcode::LSh);
    balance_ -= 1;
}

void CodeBuilder::rsh() {
    emit_op(Opcode::RSh);
    balance_ -= 1;
}

void CodeBuilder::band() {
    emit_op(Opcode::BAnd);
    balance_ -= 1;
}

void CodeBuilder::bor() {
    emit_op(Opcode::BOr);
    balance_ -= 1;
}

void CodeBuilder::bxor() {
    emit_op(Opcode::BXor);
    balance_ -= 1;
}

void CodeBuilder::gt() {
    emit_op(Opcode::Gt);
    balance_ -= 1;
}

void CodeBuilder::gte() {
    emit_op(Opcode::Gte);
    balance_ -= 1;
}

void CodeBuilder::lt() {
    emit_op(Opcode::Lt);
    balance_ -= 1;
}

void CodeBuilder::lte() {
    emit_op(Opcode::Lte);
    balance_ -= 1;
}

void CodeBuilder::eq() {
    emit_op(Opcode::Eq);
    balance_ -= 1;
}

void CodeBuilder::neq() {
    emit_op(Opcode::NEq);
    balance_ -= 1;
}

void CodeBuilder::mk_array(u32 n) {
    emit_op(Opcode::MkArray);
    w_.emit_u32(n);
    balance_ -= n;
    balance_ += 1;
}

void CodeBuilder::mk_tuple(u32 n) {
    emit_op(Opcode::MkTuple);
    w_.emit_u32(n);
    balance_ -= n;
    balance_ += 1;
}

void CodeBuilder::mk_set(u32 n) {
    emit_op(Opcode::MkSet);
    w_.emit_u32(n);
    balance_ -= n;
    balance_ += 1;
}

void CodeBuilder::mk_map(u32 n) {
    emit_op(Opcode::MkMap);
    w_.emit_u32(n);
    balance_ -= SafeInt(n) * 2;
    balance_ += 1;
}

void CodeBuilder::mk_context(u32 n) {
    emit_op(Opcode::MkContext);
    w_.emit_u32(n);
    // balance_ += 0;
}

void CodeBuilder::mk_closure() {
    emit_op(Opcode::MkClosure);
    balance_ -= 1;
}

void CodeBuilder::mk_builder() {
    emit_op(Opcode::MkBuilder);
    balance_ += 1;
}

void CodeBuilder::builder_append() {
    emit_op(Opcode::BuilderAppend);
    balance_ -= 1;
}

void CodeBuilder::builder_string() {
    emit_op(Opcode::BuilderString);
    // balance_ += 0;
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
    balance_ -= 1;
}

void CodeBuilder::jmp_false(LabelID target) {
    emit_op(Opcode::JmpFalse);
    emit_offset(target);
}

void CodeBuilder::jmp_false_pop(LabelID target) {
    emit_op(Opcode::JmpFalsePop);
    emit_offset(target);
    balance_ -= 1;
}

void CodeBuilder::call(u32 n) {
    emit_op(Opcode::Call);
    w_.emit_u32(n);
    balance_ -= n;
}

void CodeBuilder::load_method(u32 i) {
    emit_op(Opcode::LoadMethod);
    w_.emit_u32(i);
    balance_ += 1;
}

void CodeBuilder::call_method(u32 n) {
    emit_op(Opcode::CallMethod);
    w_.emit_u32(n);
    balance_ -= SafeInt(n) + 1;
}

void CodeBuilder::ret() {
    emit_op(Opcode::Ret);
    balance_ -= 1;
}

void CodeBuilder::assert_fail() {
    emit_op(Opcode::AssertFail);
    balance_ -= 2;
}

} // namespace tiro::compiler
