#include <catch.hpp>

#include "tiro/core/format.hpp"
#include "tiro/mir/dominators.hpp"

#include "./test_function.hpp"

#include <array>
#include <set>

using namespace tiro;
using namespace tiro::mir;

namespace {

class Context : public TestFunction {
public:
    Context()
        : TestFunction()
        , tree_(TestFunction::func()) {}

    DominatorTree& tree() { return tree_; }

private:
    DominatorTree tree_;
};

} // namespace

static auto make_context() {
    return std::make_unique<Context>();
}

template<typename Range1, typename Range2>
void require_set_equal(const Range1& r1, const Range2& r2) {
    std::set s1(r1.begin(), r1.end());
    std::set s2(r2.begin(), r2.end());
    REQUIRE(s1 == s2);
}

TEST_CASE("Dominators for trivial cfgs should be correct", "[dominators]") {
    auto ctx = make_context();
    auto& func = ctx->func();
    auto& tree = ctx->tree();

    func[func.entry()]->terminator(Terminator::make_jump(func.exit()));
    func[func.exit()]->append_predecessor(func.entry());
    tree.compute();

    REQUIRE(tree.dominates(func.entry(), func.exit()));
    REQUIRE(tree.dominates_strict(func.entry(), func.exit()));
    REQUIRE_FALSE(tree.dominates(func.exit(), func.entry()));
    REQUIRE_FALSE(tree.dominates_strict(func.exit(), func.entry()));

    REQUIRE(tree.dominates(func.entry(), func.entry()));
    REQUIRE(tree.dominates(func.exit(), func.exit()));
    REQUIRE_FALSE(tree.dominates_strict(func.entry(), func.entry()));
    REQUIRE_FALSE(tree.dominates_strict(func.exit(), func.exit()));
}

TEST_CASE("Dominators for an example graph should be correct", "[dominators]") {
    /* 
        Test graph:

         entry
         /   \ 
        B     D
        |\    /\
        | ^  |  F
        |/   |  |
        C    E<-G
         \   | /
           exit

        The edges marked ^ point into the indicated direction. Other
        edges flow top -> down.
    */
    auto ctx = make_context();
    auto& tree = ctx->tree();

    auto entry = ctx->entry();
    auto B = ctx->make_block("B");
    auto C = ctx->make_block("C");
    auto D = ctx->make_block("D");
    auto E = ctx->make_block("E");
    auto F = ctx->make_block("F");
    auto G = ctx->make_block("G");
    auto exit = ctx->exit();

    ctx->set_branch(entry, B, D);
    ctx->set_jump(B, C);
    ctx->set_branch(C, exit, B);
    ctx->set_branch(D, E, F);
    ctx->set_jump(F, G);
    ctx->set_branch(G, E, exit);
    ctx->set_jump(E, exit);
    tree.compute();

    REQUIRE(tree.immediate_dominator(entry) == entry);

    auto verify_idom = [&](const auto parent, const auto children) {
        INFO("parent = " << fmt::format("{}", parent));
        for (auto child : children) {
            INFO("child = " << fmt::format("{}", child));
            REQUIRE(tree.immediate_dominator(child) == parent);
        }
        require_set_equal(tree.immediately_dominated(parent), children);
    };

    verify_idom(entry, std::array{D, B, exit});
    verify_idom(D, std::array{E, F});
    verify_idom(F, std::array{G});
    verify_idom(B, std::array{C});

    auto verify_dominated = [&](const auto parent, const auto dominated) {
        INFO("parent = " << fmt::format("{}", parent));
        REQUIRE(tree.dominates(parent, parent));

        for (auto block_id : dominated) {
            INFO("block_id = " << fmt::format("{}", block_id));
            REQUIRE(tree.dominates(parent, block_id));
            REQUIRE(tree.dominates_strict(parent, block_id));
        }
    };

    verify_dominated(entry, std::array{B, C, D, E, F, G, exit});
    verify_dominated(B, std::array{C});
    verify_dominated(D, std::array{E, F, G});
    verify_dominated(F, std::array{G});
}
