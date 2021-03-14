#include <catch2/catch.hpp>

#include "compiler/ir/traversal.hpp"

#include "./test_function.hpp"

#include <memory>

namespace tiro::ir::test {

using order_vec = std::vector<std::string>;

template<typename Range>
auto reversed(const Range& range) {
    return Range(range.rbegin(), range.rend());
}

template<typename Order>
std::vector<std::string> get_order(const Function& func) {
    std::vector<std::string> result;
    for (auto block_id : Order(func)) {
        auto label = func[block_id]->label();
        result.push_back(std::string(func.strings().value(label)));
    }
    return result;
}

static std::vector<std::string> preorder(const Function& func) {
    return get_order<PreorderTraversal>(func);
}

static std::vector<std::string> postorder(const Function& func) {
    return get_order<PostorderTraversal>(func);
}

static std::vector<std::string> reverse_postorder(const Function& func) {
    return get_order<ReversePostorderTraversal>(func);
}

/* Test graph:

        entry
        /   \ 
        B     D
        |\    |\ 
        | ^   | F
        |/    |/
        C     E
        \   /
        exit

    The edge marked ^ is a back edge from C to B.
*/
static std::unique_ptr<TestFunction> test_cfg() {
    auto ctx = std::make_unique<TestFunction>();

    auto entry = ctx->entry();
    auto B = ctx->make_block("B");
    auto C = ctx->make_block("C");
    auto D = ctx->make_block("D");
    auto E = ctx->make_block("E");
    auto F = ctx->make_block("F");
    auto exit = ctx->exit();

    ctx->set_branch(entry, B, D);
    ctx->set_jump(B, C);
    ctx->set_branch(C, exit, B);
    ctx->set_branch(D, E, F);
    ctx->set_jump(F, E);
    ctx->set_jump(E, exit);
    return ctx;
}

/*
    Tree example from wikipedia (https://en.wikipedia.org/wiki/Tree_traversal)

            entry
           /    \ 
          B      G
         / \      \ 
        A   D      I
           / \    /
          C   E  H
*/
static std::unique_ptr<TestFunction> test_tree() {
    auto ctx = std::make_unique<TestFunction>();

    auto entry = ctx->entry();
    auto A = ctx->make_block("A");
    auto B = ctx->make_block("B");
    auto C = ctx->make_block("C");
    auto D = ctx->make_block("D");
    auto E = ctx->make_block("E");
    auto G = ctx->make_block("G");
    auto H = ctx->make_block("H");
    auto I = ctx->make_block("I");

    ctx->set_branch(entry, B, G);
    ctx->set_branch(B, A, D);
    ctx->set_branch(D, C, E);
    ctx->set_jump(G, I);
    ctx->set_jump(I, H);
    return ctx;
}

TEST_CASE("The order is correct for the cfg example", "[traversal]") {
    auto ctx = test_cfg();

    auto pre = preorder(ctx->func());
    REQUIRE(pre == order_vec{"entry", "B", "C", "exit", "D", "E", "F"});

    auto post = postorder(ctx->func());
    REQUIRE(post == order_vec{"exit", "C", "B", "E", "F", "D", "entry"});

    auto rpos = reverse_postorder(ctx->func());
    REQUIRE(rpos == reversed(post));
}

TEST_CASE("The order is correct for the tree example", "[traversal]") {
    auto ctx = test_tree();

    auto pre = preorder(ctx->func());
    REQUIRE(pre == order_vec{"entry", "B", "A", "D", "C", "E", "G", "I", "H"});

    auto post = postorder(ctx->func());
    auto expected_post = order_vec{
        "A",
        "C",
        "E",
        "D",
        "B",
        "H",
        "I",
        "G",
        "entry",
    };
    REQUIRE(post == expected_post);

    auto rpos = reverse_postorder(ctx->func());
    REQUIRE(rpos == reversed(post));
}

} // namespace tiro::ir::test
