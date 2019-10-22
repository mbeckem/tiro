#include "hammer/compiler/codegen/basic_block.hpp"

namespace hammer {

BasicBlockEdge BasicBlockEdge::make_none() {
    BasicBlockEdge edge;
    edge.which_ = Which::None;
    edge.none_ = None();
    return edge;
}

BasicBlockEdge BasicBlockEdge::make_jump(BasicBlock* target) {
    HAMMER_ASSERT_NOT_NULL(target);
    BasicBlockEdge edge;
    edge.which_ = Which::Jump;
    edge.jump_ = Jump{target};
    return edge;
}

BasicBlockEdge BasicBlockEdge::make_cond_jump(
    Opcode code, BasicBlock* target, BasicBlock* fallthrough) {
    HAMMER_ASSERT_NOT_NULL(target);
    HAMMER_ASSERT_NOT_NULL(fallthrough);
    // TODO HAMMER_ASSERT(is_jump(code), "Must be a jump instruction.");
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
#define HAMMER_CASE(x)             \
    case BasicBlockEdge::Which::x: \
        return #x;

        HAMMER_CASE(None)
        HAMMER_CASE(Jump)
        HAMMER_CASE(CondJump)
        HAMMER_CASE(Ret)
    }

    HAMMER_UNREACHABLE("Invalid edge type.");
}

BasicBlock::BasicBlock(InternedString title)
    : title_(title)
    , builder_(code_) {}

BasicBlockStorage::BasicBlockStorage() {}

BasicBlockStorage::~BasicBlockStorage() {
    reset();
}

BasicBlock* BasicBlockStorage::make_block(InternedString title) {
    blocks_.emplace_back(std::make_unique<BasicBlock>(title));
    return blocks_.back().get();
}

void BasicBlockStorage::reset() {
    blocks_.clear();
}

} // namespace hammer
