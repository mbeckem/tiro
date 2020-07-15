#include <catch2/catch.hpp>

#include "compiler/ir/liveness.hpp"

#include <unordered_set>

using namespace tiro;

namespace {

struct TestContext {
    explicit TestContext(std::string_view function_name = "func")
        : strings_()
        , func_(strings_.insert(function_name), FunctionType::Normal, strings_) {}

    TestContext(TestContext&&) = delete;
    TestContext& operator=(TestContext&&) = delete;

    StringTable& strings() { return strings_; }
    Function& func() { return func_; }

    std::string_view label(BlockId block) const { return strings_.dump(func_[block]->label()); }

    BlockId entry() const { return func_.entry(); }

    BlockId exit() const { return func_.exit(); }

    BlockId make_block(std::string_view label) { return func_.make(Block(strings_.insert(label))); }

    void set_jump(BlockId id, BlockId target) {
        func_[id]->terminator(Terminator::make_jump(target));
        func_[target]->append_predecessor(id);
    }

    void set_branch(BlockId id, LocalId local, BlockId target1, BlockId target2) {
        func_[id]->terminator(Terminator::make_branch(BranchType::IfTrue, local, target1, target2));
        func_[target1]->append_predecessor(id);
        func_[target2]->append_predecessor(id);
    }

    void set_return(BlockId id, LocalId local) {
        func_[id]->terminator(Terminator::make_return(local, exit()));
        func_[exit()]->append_predecessor(id);
    }

    bool has_predecessor(BlockId id, BlockId pred) const {
        auto block = func_[id];
        return contains(block->predecessors(), pred);
    }

    LocalId define(BlockId id, std::string_view name, const RValue& value) {
        Local local(value);
        local.name(strings_.insert(name));
        auto local_id = func_.make(local);
        func_[id]->append_stmt(Stmt::make_define(local_id));
        return local_id;
    }

    LocalId define_phi(BlockId id, std::string_view name, std::initializer_list<LocalId> operands) {
        auto phi_id = func_.make(Phi(operands));
        return define(id, name, RValue::make_phi(phi_id));
    }

    StringTable strings_;
    Function func_;
};

struct TestLiveness {
    explicit TestLiveness(const Function& func)
        : lv(func) {
        lv.compute();
    }

    void require_live_in(BlockId id, std::initializer_list<LocalId> expected);

    const LiveRange* require_range(
        LocalId value, LiveInterval expected_def, std::vector<LiveInterval> expected_live_in);

    Liveness lv;
};

} // namespace

template<typename R1, typename R2>
static bool range_equal(const R1& r1, const R2& r2) {
    using T1 = typename R1::value_type;
    using T2 = typename R2::value_type;
    using T = std::common_type_t<T1, T2>;

    std::unordered_set<T, UseHasher> m1(r1.begin(), r1.end());
    std::unordered_set<T, UseHasher> m2(r2.begin(), r2.end());
    return m1 == m2;
}

template<typename Range>
static std::string format_range(const Range& range) {
    return fmt::format("{{{}}}", fmt::join(range, ", "));
}

void TestLiveness::require_live_in(BlockId id, std::initializer_list<LocalId> expected) {
    CAPTURE(fmt::to_string(id));
    CAPTURE(format_range(expected));

    auto live_values = lv.live_in_values(id);
    REQUIRE(range_equal(live_values, expected));
}

const LiveRange* TestLiveness::require_range(
    LocalId value, LiveInterval expected_def, std::vector<LiveInterval> expected_live_in) {
    CAPTURE(fmt::to_string(value));
    CAPTURE(fmt::to_string(expected_def));
    CAPTURE(format_range(expected_live_in));

    auto range = lv.live_range(value);
    REQUIRE(range);
    REQUIRE(range->definition() == expected_def);
    REQUIRE(range->dead() == (expected_def.start == expected_def.end));

    {
        auto live_in = to_vector(range->live_in_intervals());
        CAPTURE(format_range(live_in));
        CAPTURE(format_range(expected_live_in));
        REQUIRE(range_equal(live_in, expected_live_in));
    }

    for (const auto& interval : expected_live_in) {
        REQUIRE(range->live_in(interval.block));
    }
    return range;
}

TEST_CASE("Liveness information should be correct for simple variables", "[liveness]") {
    TestContext test;
    auto block_entry = test.entry();
    auto block_a = test.make_block("a");
    auto block_b = test.make_block("b");
    auto block_exit = test.exit();

    auto x = test.define(block_entry, "x", Constant::make_integer(1)); // used in z and jump
    auto y = test.define(block_entry, "y", Constant::make_integer(2)); // dead
    auto z = test.define(block_entry, "z", RValue::make_use_local(x)); // returned
    auto w = test.define(block_b, "w", Constant::make_null());

    test.set_branch(block_entry, x, block_a, block_b);
    test.set_return(block_a, z);
    test.set_return(block_b, w);

    TestLiveness liveness(test.func());
    liveness.require_live_in(block_entry, {});
    liveness.require_live_in(block_a, {z});
    liveness.require_live_in(block_b, {});
    liveness.require_live_in(block_exit, {});

    auto rx = liveness.require_range(x, LiveInterval(block_entry, 0, 3), {});
    REQUIRE(!rx->last_use(block_entry, 2));
    REQUIRE(rx->last_use(block_entry, 3));

    auto ry = liveness.require_range(y, LiveInterval(block_entry, 1, 1), {});
    REQUIRE(ry->last_use(block_entry, 1));

    auto rz = liveness.require_range(
        z, LiveInterval(block_entry, 2, 4), {LiveInterval(block_a, 0, 0)});
    REQUIRE(!rz->last_use(block_entry, 3));
    REQUIRE(rz->last_use(block_a, 0));

    auto rw = liveness.require_range(w, LiveInterval(block_b, 0, 1), {});
    REQUIRE(rw->last_use(block_b, 1));
}

TEST_CASE("Liveness should be correct for arguments of phi functions", "[liveness]") {
    TestContext test;

    /*  
     *  entry
     *  |  \ 
     *  |   a
     *  \  /
     *  exit
     */
    auto block_entry = test.entry();
    auto block_a = test.make_block("a");
    auto block_exit = test.exit();

    // w is used only in the phi function y.
    // x is being used as a normal local in addition to being an operand of the phi function.
    auto w = test.define(block_entry, "w", Constant::make_integer(1));
    auto x = test.define(block_entry, "x", Constant::make_integer(2));
    auto y = test.define_phi(block_exit, "y", {w, x});
    auto z = test.define(block_exit, "z", RValue::make_use_local(x));
    test.set_branch(block_entry, w, block_exit, block_a);
    test.set_jump(block_a, block_exit);

    TestLiveness liveness(test.func());
    liveness.require_live_in(block_entry, {});
    liveness.require_live_in(block_a, {x});
    liveness.require_live_in(block_exit, {x});

    auto rw = liveness.require_range(w, LiveInterval(block_entry, 0, 3), {});
    REQUIRE(rw->last_use(block_entry, 3));

    auto rx = liveness.require_range(x, LiveInterval(block_entry, 1, 3),
        {LiveInterval(block_a, 0, 1), LiveInterval(block_exit, 0, 1)});
    REQUIRE_FALSE(rx->last_use(block_a, 0));
    REQUIRE(rx->last_use(block_exit, 1));

    liveness.require_range(y, LiveInterval(block_exit, 0, 0), {});
    liveness.require_range(z, LiveInterval(block_exit, 1, 1), {});
}
