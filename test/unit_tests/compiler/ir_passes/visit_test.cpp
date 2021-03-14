#include <catch2/catch.hpp>

#include "compiler/ir/function.hpp"
#include "compiler/ir_passes/visit.hpp"

namespace tiro::ir::test {

namespace {

struct TestFunction {
    StringTable strings;
    Function func;

    TestFunction()
        : strings()
        , func(string("test-func"), FunctionType::Normal, strings) {}

    InternedString string(std::string_view value) { return strings.insert(value); }

    InstId local() { return local(Value::make_error()); }
    InstId local(Value value) { return func.make(Inst(std::move(value))); }

    template<typename T>
    void require_locals(const T& item, const std::vector<InstId>& expected) {
        std::vector<InstId> actual;
        visit_insts(func, item, [&](InstId id) { actual.push_back(id); });
        require_equal(actual, expected);
    }

    template<typename T>
    void require_uses(const T& item, const std::vector<InstId>& expected) {
        std::vector<InstId> actual;
        visit_inst_operands(func, item, [&](InstId id) { actual.push_back(id); });
        require_equal(actual, expected);
    }

    void require_equal(const std::vector<InstId>& actual, const std::vector<InstId>& expected) {
        REQUIRE(actual.size() == expected.size());
        for (size_t i = 0; i < actual.size(); ++i) {
            auto actual_local = actual[i];
            auto expected_local = expected[i];
            CAPTURE(actual_local.value(), expected_local.value());
            if (actual_local != expected_local) {
                FAIL();
            }
        }
    }
};

} // namespace

TEST_CASE("visit_insts() should visit all referenced insts in a block", "[visit]") {
    TestFunction test;

    SECTION("block") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto l2 = test.local();
        auto l3 = test.local(Value::make_read(LValue::make_field(l0, test.string("foo"))));
        auto l4 = test.local(Value::make_write(LValue::make_index(l0, l1), l2));
        auto l5 = test.local(Constant::make_integer(1));

        Block block(test.string("block"));
        block.append_inst(l4);
        block.append_inst(l3);
        block.terminator(Terminator::make_branch(BranchType::IfTrue, l5, BlockId(1), BlockId(2)));

        test.require_locals(block, {l4, l0, l1, l2, l3, l0, l5});
    }
}

TEST_CASE("visit_insts() should visit all insts in terminators", "[visit]") {
    TestFunction test;

    SECTION("none") {
        test.require_locals(Terminator::make_none(), {});
        test.require_locals(Terminator::make_jump(BlockId(1)), {});
        test.require_locals(Terminator::make_exit(), {});
        test.require_locals(Terminator::make_never(BlockId(2)), {});
    }

    SECTION("branch") {
        auto l0 = test.local();
        auto term = Terminator::make_branch(BranchType::IfTrue, l0, BlockId(1), BlockId(2));
        test.require_locals(term, {l0});
    }

    SECTION("return") {
        auto l0 = test.local();
        auto term = Terminator::make_return(l0, BlockId(1));
        test.require_locals(term, {l0});
    }

    SECTION("assert fail") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto term = Terminator::make_assert_fail(l0, l1, BlockId(1));
        test.require_locals(term, {l0, l1});
    }
}

TEST_CASE("visit_insts() should visit all insts in a lvalue", "[visit]") {
    TestFunction test;

    SECTION("param") { test.require_locals(LValue::make_param(ParamId(1)), {}); }

    SECTION("closure") {
        auto l0 = test.local();
        test.require_locals(LValue::make_closure(l0, 1, 2), {l0});
    }

    SECTION("module") { test.require_locals(LValue::make_module(ModuleMemberId(123)), {}); }

    SECTION("field") {
        auto l0 = test.local();
        test.require_locals(LValue::make_field(l0, test.string("foo")), {l0});
    }

    SECTION("tuple field") {
        auto l0 = test.local();
        test.require_locals(LValue::make_tuple_field(l0, 1), {l0});
    }

    SECTION("index") {
        auto l0 = test.local();
        auto l1 = test.local();
        test.require_locals(LValue::make_index(l0, l1), {l0, l1});
    }
}

TEST_CASE("visit_insts() should visit all insts in a value", "[visit]") {
    TestFunction test;

    SECTION("read lvalue") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto value = Value::make_read(LValue::make_index(l0, l1));
        test.require_locals(value, {l0, l1});
    }

    SECTION("alias local") {
        auto l0 = test.local();
        test.require_locals(Value::make_alias(l0), {l0});
    }

    SECTION("publish assign") {
        auto l0 = test.local();
        auto value = Value::make_publish_assign(SymbolId(123), l0);
        test.require_locals(value, {l0});
    }

    SECTION("phi") {
        auto l0 = test.local();
        auto l1 = test.local();
        test.require_locals(Value(Phi(test.func, {l0, l1})), {l0, l1});
    }

    SECTION("observe assign") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto list_id = test.func.make(LocalList{l0, l1});
        auto value = Value::make_observe_assign(SymbolId(123), list_id);
        test.require_locals(value, {l0, l1});
    }

    SECTION("constant") {
        auto value = Value::make_constant(Constant::make_integer(123));
        test.require_locals(value, {});
    }

    SECTION("outer environment") { test.require_locals(Value::make_outer_environment(), {}); }

    SECTION("binary op") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto op = Value::make_binary_op(BinaryOpType::Plus, l0, l1);
        test.require_locals(op, {l0, l1});
    }

    SECTION("unary op") {
        auto l0 = test.local();
        auto op = Value::make_unary_op(UnaryOpType::Minus, l0);
        test.require_locals(op, {l0});
    }

    SECTION("call") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto l2 = test.local();
        auto list_id = test.func.make(LocalList{l1, l2});
        auto call = Value::make_call(l0, list_id);
        test.require_locals(call, {l0, l1, l2});
    }

    SECTION("aggregate") {
        auto l0 = test.local();
        auto method = Value::make_aggregate(Aggregate::make_method(l0, test.string("foo")));
        test.require_locals(method, {l0});
    }

    SECTION("get aggregate member") {
        auto l0 = test.local();
        auto instance = Value::make_get_aggregate_member(l0, AggregateMember::MethodInstance);
        test.require_locals(instance, {l0});
    }

    SECTION("method call") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto l2 = test.local();
        auto list_id = test.func.make(LocalList{l1, l2});
        auto call = Value::make_method_call(l0, list_id);
        test.require_locals(call, {l0, l1, l2});
    }

    SECTION("make environment") {
        auto l0 = test.local();
        auto env = Value::make_make_environment(l0, 123);
        test.require_locals(env, {l0});
    }

    SECTION("make closure") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto closure = Value::make_make_closure(l0, l1);
        test.require_locals(closure, {l0, l1});
    }

    SECTION("container") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto list_id = test.func.make(LocalList{l0, l1});
        auto container = Value::make_container(ContainerType::Array, list_id);
        test.require_locals(container, {l0, l1});
    }

    SECTION("format") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto list_id = test.func.make(LocalList{l0, l1});
        auto format = Value::make_format(list_id);
        test.require_locals(format, {l0, l1});
    }

    SECTION("error") { test.require_locals(Value::make_error(), {}); }
}

TEST_CASE("visit_insts() should visit the local's value", "[visit]") {
    TestFunction test;

    auto l0 = test.local();
    auto l1 = test.local();
    auto local = Inst(Value::make_binary_op(BinaryOpType::Plus, l0, l1));
    test.require_locals(local, {l0, l1});
}

TEST_CASE("visit_insts() should visit the phi operands", "[visit]") {
    TestFunction test;

    auto l0 = test.local();
    auto l1 = test.local();
    test.require_locals(Phi(test.func, {l0, l1}), {l0, l1});
}

TEST_CASE("visit_insts() should visit the list elements", "[visit]") {
    TestFunction test;

    auto l0 = test.local();
    auto l1 = test.local();
    test.require_locals(LocalList{l0, l1}, {l0, l1});
}

TEST_CASE("visit_inst_operands() only visits the used insts, not the definition", "[visit]") {
    TestFunction test;

    auto l0 = test.local();
    auto l1 = test.local();
    auto l2 = test.local(Value::make_make_closure(l0, l1));
    test.require_uses(l2, {l0, l1});
}

} // namespace tiro::ir::test
