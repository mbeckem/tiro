#include "tiro/codegen/code_builder.hpp"

#include "tiro/codegen/basic_block.hpp"

#include <limits>

namespace tiro {

CodeBuilder::CodeBuilder(std::vector<byte>& out)
    : w_(out) {}

void CodeBuilder::define_block(NotNull<BasicBlock*> block) {
    bool inserted = blocks_.emplace(block, w_.pos()).second;
    TIRO_CHECK(inserted, "Label was already defined.");
    TIRO_CHECK(w_.pos() < u32(-1), "Code size too large.");
}

void CodeBuilder::finish() {
    for (const auto& use : block_uses_) {
        u32 offset = use.first;
        BasicBlock* block = use.second;

        TIRO_ASSERT(block, "Invalid block.");
        TIRO_ASSERT(offset < w_.pos(), "Invalid offset.");

        u32 location;
        if (auto pos = blocks_.find(block); pos != blocks_.end()) {
            location = pos->second;
        } else {
            TIRO_ERROR("Block was never defined.");
        }

        w_.overwrite_u32(offset, location);
    }
}

void CodeBuilder::emit_offset(BasicBlock* block) {
    if (auto pos = blocks_.find(block); pos != blocks_.end()) {
        w_.emit_u32(pos->second);
    } else {
        block_uses_.emplace_back(u32(w_.pos()), block);
        w_.emit_u32(u32(-1)); // Will be overwritten in finish()
    }
}

void CodeBuilder::emit_op(OldOpcode op) {
    w_.emit_u8(static_cast<u8>(op));
}

void CodeBuilder::load_null() {
    emit_op(OldOpcode::LoadNull);
}

void CodeBuilder::load_false() {
    emit_op(OldOpcode::LoadFalse);
}

void CodeBuilder::load_true() {
    emit_op(OldOpcode::LoadTrue);
}

void CodeBuilder::load_int(i64 i) {
    emit_op(OldOpcode::LoadInt);
    w_.emit_i64(i);
}

void CodeBuilder::load_float(f64 d) {
    emit_op(OldOpcode::LoadFloat);
    w_.emit_f64(d);
}

void CodeBuilder::load_param(u32 i) {
    emit_op(OldOpcode::LoadParam);
    w_.emit_u32(i);
}

void CodeBuilder::store_param(u32 i) {
    emit_op(OldOpcode::StoreParam);
    w_.emit_u32(i);
}

void CodeBuilder::load_local(u32 i) {
    emit_op(OldOpcode::LoadLocal);
    w_.emit_u32(i);
}

void CodeBuilder::store_local(u32 i) {
    emit_op(OldOpcode::StoreLocal);
    w_.emit_u32(i);
}

void CodeBuilder::load_closure() {
    emit_op(OldOpcode::LoadClosure);
}

void CodeBuilder::load_context(u32 n, u32 i) {
    emit_op(OldOpcode::LoadContext);
    w_.emit_u32(n);
    w_.emit_u32(i);
}

void CodeBuilder::store_context(u32 n, u32 i) {
    emit_op(OldOpcode::StoreContext);
    w_.emit_u32(n);
    w_.emit_u32(i);
}

void CodeBuilder::load_member(u32 i) {
    emit_op(OldOpcode::LoadMember);
    w_.emit_u32(i);
}

void CodeBuilder::store_member(u32 i) {
    emit_op(OldOpcode::StoreMember);
    w_.emit_u32(i);
}

void CodeBuilder::load_tuple_member(u32 i) {
    emit_op(OldOpcode::LoadTupleMember);
    w_.emit_u32(i);
}

void CodeBuilder::store_tuple_member(u32 i) {
    emit_op(OldOpcode::StoreTupleMember);
    w_.emit_u32(i);
}

void CodeBuilder::load_index() {
    emit_op(OldOpcode::LoadIndex);
}

void CodeBuilder::store_index() {
    emit_op(OldOpcode::StoreIndex);
}

void CodeBuilder::load_module(u32 i) {
    emit_op(OldOpcode::LoadModule);
    w_.emit_u32(i);
}

void CodeBuilder::store_module(u32 i) {
    emit_op(OldOpcode::StoreModule);
    w_.emit_u32(i);
}

void CodeBuilder::load_global(u32 i) {
    emit_op(OldOpcode::LoadGlobal);
    w_.emit_u32(i);
}

void CodeBuilder::dup() {
    emit_op(OldOpcode::Dup);
}

void CodeBuilder::pop() {
    emit_op(OldOpcode::Pop);
}

void CodeBuilder::pop_n(u32 n) {
    emit_op(OldOpcode::PopN);
    w_.emit_u32(n);
}

void CodeBuilder::rot_2() {
    emit_op(OldOpcode::Rot2);
}

void CodeBuilder::rot_3() {
    emit_op(OldOpcode::Rot3);
}

void CodeBuilder::rot_4() {
    emit_op(OldOpcode::Rot4);
}

void CodeBuilder::add() {
    emit_op(OldOpcode::Add);
}

void CodeBuilder::sub() {
    emit_op(OldOpcode::Sub);
}

void CodeBuilder::mul() {
    emit_op(OldOpcode::Mul);
}

void CodeBuilder::div() {
    emit_op(OldOpcode::Div);
}

void CodeBuilder::mod() {
    emit_op(OldOpcode::Mod);
}

void CodeBuilder::pow() {
    emit_op(OldOpcode::Pow);
}

void CodeBuilder::lnot() {
    emit_op(OldOpcode::LNot);
}

void CodeBuilder::bnot() {
    emit_op(OldOpcode::BNot);
}

void CodeBuilder::upos() {
    emit_op(OldOpcode::UPos);
}

void CodeBuilder::uneg() {
    emit_op(OldOpcode::UNeg);
}

void CodeBuilder::lsh() {
    emit_op(OldOpcode::LSh);
}

void CodeBuilder::rsh() {
    emit_op(OldOpcode::RSh);
}

void CodeBuilder::band() {
    emit_op(OldOpcode::BAnd);
}

void CodeBuilder::bor() {
    emit_op(OldOpcode::BOr);
}

void CodeBuilder::bxor() {
    emit_op(OldOpcode::BXor);
}

void CodeBuilder::gt() {
    emit_op(OldOpcode::Gt);
}

void CodeBuilder::gte() {
    emit_op(OldOpcode::Gte);
}

void CodeBuilder::lt() {
    emit_op(OldOpcode::Lt);
}

void CodeBuilder::lte() {
    emit_op(OldOpcode::Lte);
}

void CodeBuilder::eq() {
    emit_op(OldOpcode::Eq);
}

void CodeBuilder::neq() {
    emit_op(OldOpcode::NEq);
}

void CodeBuilder::mk_array(u32 n) {
    emit_op(OldOpcode::MkArray);
    w_.emit_u32(n);
}

void CodeBuilder::mk_tuple(u32 n) {
    emit_op(OldOpcode::MkTuple);
    w_.emit_u32(n);
}

void CodeBuilder::mk_set(u32 n) {
    emit_op(OldOpcode::MkSet);
    w_.emit_u32(n);
}

void CodeBuilder::mk_map(u32 n) {
    emit_op(OldOpcode::MkMap);
    w_.emit_u32(n);
}

void CodeBuilder::mk_context(u32 n) {
    emit_op(OldOpcode::MkContext);
    w_.emit_u32(n);
}

void CodeBuilder::mk_closure() {
    emit_op(OldOpcode::MkClosure);
}

void CodeBuilder::mk_builder() {
    emit_op(OldOpcode::MkBuilder);
}

void CodeBuilder::builder_append() {
    emit_op(OldOpcode::BuilderAppend);
}

void CodeBuilder::builder_string() {
    emit_op(OldOpcode::BuilderString);
}

void CodeBuilder::jmp(BasicBlock* target) {
    emit_op(OldOpcode::Jmp);
    emit_offset(target);
}

void CodeBuilder::jmp_true(BasicBlock* target) {
    emit_op(OldOpcode::JmpTrue);
    emit_offset(target);
}

void CodeBuilder::jmp_true_pop(BasicBlock* target) {
    emit_op(OldOpcode::JmpTruePop);
    emit_offset(target);
}

void CodeBuilder::jmp_false(BasicBlock* target) {
    emit_op(OldOpcode::JmpFalse);
    emit_offset(target);
}

void CodeBuilder::jmp_false_pop(BasicBlock* target) {
    emit_op(OldOpcode::JmpFalsePop);
    emit_offset(target);
}

void CodeBuilder::call(u32 n) {
    emit_op(OldOpcode::Call);
    w_.emit_u32(n);
}

void CodeBuilder::load_method(u32 i) {
    emit_op(OldOpcode::LoadMethod);
    w_.emit_u32(i);
}

void CodeBuilder::call_method(u32 n) {
    emit_op(OldOpcode::CallMethod);
    w_.emit_u32(n);
}

void CodeBuilder::ret() {
    emit_op(OldOpcode::Ret);
}

void CodeBuilder::assert_fail() {
    emit_op(OldOpcode::AssertFail);
}

} // namespace tiro
