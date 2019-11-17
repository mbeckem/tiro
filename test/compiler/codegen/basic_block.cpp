#include <catch.hpp>

#include "hammer/compiler/codegen/basic_block.hpp"

using namespace hammer;

TEST_CASE("Edge types", "[basic-block]") {
    BasicBlockStorage storage;
    BasicBlock* b1 = storage.make_block({});
    BasicBlock* b2 = storage.make_block({});

    BasicBlockEdge edge_none = BasicBlockEdge::make_none();
    REQUIRE(edge_none.which() == BasicBlockEdge::Which::None);
    REQUIRE(&edge_none.none() != nullptr);

    BasicBlockEdge edge_jump = BasicBlockEdge::make_jump(b1);
    REQUIRE(edge_jump.which() == BasicBlockEdge::Which::Jump);
    REQUIRE(edge_jump.jump().target == b1);

    BasicBlockEdge edge_cond_jump = BasicBlockEdge::make_cond_jump(
        Opcode::JmpTruePop, b1, b2);
    REQUIRE(edge_cond_jump.which() == BasicBlockEdge::Which::CondJump);
    REQUIRE(edge_cond_jump.cond_jump().target == b1);
    REQUIRE(edge_cond_jump.cond_jump().fallthrough == b2);

    BasicBlockEdge edge_ret = BasicBlockEdge::make_ret();
    REQUIRE(edge_ret.which() == BasicBlockEdge::Which::Ret);
    REQUIRE(&edge_ret.ret() != nullptr);
}