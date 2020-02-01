#include "tiro/codegen/basic_block.hpp"

namespace tiro::compiler {

BasicBlockEdge BasicBlockEdge::make_none() {
    BasicBlockEdge edge;
    edge.which_ = Which::None;
    edge.none_ = None();
    return edge;
}

BasicBlockEdge BasicBlockEdge::make_jump(BasicBlock* target) {
    TIRO_ASSERT_NOT_NULL(target);
    BasicBlockEdge edge;
    edge.which_ = Which::Jump;
    edge.jump_ = Jump{target};
    return edge;
}

BasicBlockEdge BasicBlockEdge::make_cond_jump(
    Opcode code, BasicBlock* target, BasicBlock* fallthrough) {
    TIRO_ASSERT_NOT_NULL(target);
    TIRO_ASSERT_NOT_NULL(fallthrough);
    // TODO TIRO_ASSERT(is_jump(code), "Must be a jump instruction.");
    BasicBlockEdge edge;
    edge.which_ = Which::CondJump;
    edge.cond_jump_ = CondJump{code, target, fallthrough};
    return edge;
}

BasicBlockEdge BasicBlockEdge::make_ret() {
    BasicBlockEdge edge;
    edge.which_ = Which::Ret;
    edge.ret_ = Ret();
    return edge;
}

std::string_view to_string(BasicBlockEdge::Which which) {
    switch (which) {
#define TIRO_CASE(x)               \
    case BasicBlockEdge::Which::x: \
        return #x;

        TIRO_CASE(None)
        TIRO_CASE(Jump)
        TIRO_CASE(CondJump)
        TIRO_CASE(Ret)
    }

    TIRO_UNREACHABLE("Invalid edge type.");
}

BasicBlock::BasicBlock(InternedString title, u32 initial_balance)
    : title_(title)
    , initial_balance_(initial_balance)
    , builder_(code_) {}

BasicBlockStorage::BasicBlockStorage() {}

BasicBlockStorage::~BasicBlockStorage() {
    reset();
}

BasicBlock*
BasicBlockStorage::make_block(InternedString title, u32 initial_balance) {
    blocks_.emplace_back(std::make_unique<BasicBlock>(title, initial_balance));
    return blocks_.back().get();
}

void BasicBlockStorage::reset() {
    blocks_.clear();
}

} // namespace tiro::compiler
