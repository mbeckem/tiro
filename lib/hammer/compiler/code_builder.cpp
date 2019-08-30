#include "hammer/compiler/code_builder.hpp"

#include <limits>

namespace hammer {

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
        HAMMER_CHECK(def.location != u32(-1), "The label {} did not have its location defined.",
                     def.name);

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
    HAMMER_CHECK(id.value_ != u32(-1) && id.value_ < labels_.size(), "Invalid label id.");
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
    emit_op(Opcode::load_null);
}

void CodeBuilder::load_false() {
    emit_op(Opcode::load_false);
}

void CodeBuilder::load_true() {
    emit_op(Opcode::load_true);
}

void CodeBuilder::load_int(i64 i) {
    emit_op(Opcode::load_int);
    w_.emit_i64(i);
}

void CodeBuilder::load_float(double d) {
    emit_op(Opcode::load_float);
    w_.emit_f64(d);
}

void CodeBuilder::load_const(u32 i) {
    emit_op(Opcode::load_const);
    w_.emit_u32(i);
}

void CodeBuilder::load_param(u32 i) {
    emit_op(Opcode::load_param);
    w_.emit_u32(i);
}

void CodeBuilder::store_param(u32 i) {
    emit_op(Opcode::store_param);
    w_.emit_u32(i);
}

void CodeBuilder::load_local(u32 i) {
    emit_op(Opcode::load_local);
    w_.emit_u32(i);
}

void CodeBuilder::store_local(u32 i) {
    emit_op(Opcode::store_local);
    w_.emit_u32(i);
}

void CodeBuilder::load_env(u32 n, u32 i) {
    emit_op(Opcode::load_env);
    w_.emit_u32(n);
    w_.emit_u32(i);
}

void CodeBuilder::store_env(u32 n, u32 i) {
    emit_op(Opcode::store_env);
    w_.emit_u32(n);
    w_.emit_u32(i);
}

void CodeBuilder::load_member(u32 i) {
    emit_op(Opcode::load_member);
    w_.emit_u32(i);
}

void CodeBuilder::store_member(u32 i) {
    emit_op(Opcode::store_member);
    w_.emit_u32(i);
}

void CodeBuilder::load_index() {
    emit_op(Opcode::load_index);
}

void CodeBuilder::store_index() {
    emit_op(Opcode::store_index);
}

void CodeBuilder::load_module(u32 i) {
    emit_op(Opcode::load_module);
    w_.emit_u32(i);
}

void CodeBuilder::store_module(u32 i) {
    emit_op(Opcode::store_module);
    w_.emit_u32(i);
}

void CodeBuilder::load_global(u32 i) {
    emit_op(Opcode::load_global);
    w_.emit_u32(i);
}

void CodeBuilder::dup() {
    emit_op(Opcode::dup);
}

void CodeBuilder::pop() {
    emit_op(Opcode::pop);
}

void CodeBuilder::rot_2() {
    emit_op(Opcode::rot_2);
}

void CodeBuilder::rot_3() {
    emit_op(Opcode::rot_3);
}

void CodeBuilder::rot_4() {
    emit_op(Opcode::rot_4);
}

void CodeBuilder::add() {
    emit_op(Opcode::add);
}

void CodeBuilder::sub() {
    emit_op(Opcode::sub);
}

void CodeBuilder::mul() {
    emit_op(Opcode::mul);
}

void CodeBuilder::div() {
    emit_op(Opcode::div);
}

void CodeBuilder::mod() {
    emit_op(Opcode::mod);
}

void CodeBuilder::pow() {
    emit_op(Opcode::pow);
}

void CodeBuilder::lnot() {
    emit_op(Opcode::lnot);
}

void CodeBuilder::bnot() {
    emit_op(Opcode::bnot);
}

void CodeBuilder::upos() {
    emit_op(Opcode::upos);
}

void CodeBuilder::uneg() {
    emit_op(Opcode::uneg);
}

void CodeBuilder::gt() {
    emit_op(Opcode::gt);
}

void CodeBuilder::gte() {
    emit_op(Opcode::gte);
}

void CodeBuilder::lt() {
    emit_op(Opcode::lt);
}

void CodeBuilder::lte() {
    emit_op(Opcode::lte);
}

void CodeBuilder::eq() {
    emit_op(Opcode::eq);
}

void CodeBuilder::neq() {
    emit_op(Opcode::neq);
}

void CodeBuilder::jmp(LabelID target) {
    emit_op(Opcode::jmp);
    emit_offset(target);
}

void CodeBuilder::jmp_true(LabelID target) {
    emit_op(Opcode::jmp_true);
    emit_offset(target);
}

void CodeBuilder::jmp_true_pop(LabelID target) {
    emit_op(Opcode::jmp_true_pop);
    emit_offset(target);
}

void CodeBuilder::jmp_false(LabelID target) {
    emit_op(Opcode::jmp_false);
    emit_offset(target);
}

void CodeBuilder::jmp_false_pop(LabelID target) {
    emit_op(Opcode::jmp_false_pop);
    emit_offset(target);
}

void CodeBuilder::call(u32 n) {
    emit_op(Opcode::call);
    w_.emit_u32(n);
}

void CodeBuilder::ret() {
    emit_op(Opcode::ret);
}

} // namespace hammer
