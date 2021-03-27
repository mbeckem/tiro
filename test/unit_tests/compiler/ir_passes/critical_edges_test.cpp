#include <catch2/catch.hpp>

#include "common/text/string_table.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/ir_passes/critical_edges.hpp"

#include "../ir/test_function.hpp"

namespace tiro::ir::test {

template<typename Range>
static bool distinct_blocks(const Range& r) {
    std::unordered_set<BlockId, UseHasher> set(r.begin(), r.end());
    return set.size() == r.size();
}

TEST_CASE("Critical edges should be split", "[critical-edges]") {
    auto ctx = std::make_unique<ir::test::TestFunction>();
    auto& func = ctx->func();

    /*
        Test graph:

           entry
           /  \ 
          A <- B
           \  /
           exit   

        The edges from entry to A and from B to A are both critical.
    */

    auto entry = ctx->entry();
    auto A = ctx->make_block("A");
    auto B = ctx->make_block("B");
    auto exit = ctx->exit();

    ctx->set_branch(entry, A, B);
    ctx->set_jump(A, exit);
    ctx->set_branch(B, exit, A); // note: A is fallthrough

    REQUIRE(ctx->has_predecessor(A, entry));
    REQUIRE(ctx->has_predecessor(A, B));

    bool changed = split_critical_edges(func);
    REQUIRE(changed);

    REQUIRE_FALSE(ctx->has_predecessor(A, entry));
    REQUIRE_FALSE(ctx->has_predecessor(A, B));

    auto verify_edge = [&](BlockId new_id, BlockId pred, BlockId succ) {
        REQUIRE(ctx->has_predecessor(new_id, pred));
        REQUIRE(ctx->has_predecessor(succ, new_id));

        const auto& term = func[new_id].terminator();
        REQUIRE(term.type() == TerminatorType::Jump);
        REQUIRE(term.as_jump().target == succ);
    };

    auto new_entry_a = func[entry].terminator().as_branch().target;
    verify_edge(new_entry_a, entry, A);

    auto new_b_a = func[B].terminator().as_branch().fallthrough;
    verify_edge(new_b_a, B, A);

    REQUIRE(distinct_blocks(std::array{entry, A, B, exit, new_entry_a, new_b_a}));

    changed = split_critical_edges(func);
    REQUIRE_FALSE(changed);
}

} // namespace tiro::ir::test
