#include <catch.hpp>

#include "compiler/ir/function.hpp"
#include "compiler/ir/locals.hpp"

using namespace tiro;

namespace {

struct TestFunction {
    StringTable strings;
    Function func;

    TestFunction()
        : strings()
        , func(string("test-func"), FunctionType::Normal, strings) {}

    InternedString string(std::string_view value) { return strings.insert(value); }

    LocalId local() { return local(RValue::make_error()); }
    LocalId local(const RValue& value) { return func.make(Local(value)); }

    template<typename T>
    void require_locals(const T& item, const std::vector<LocalId>& expected) {
        std::vector<LocalId> actual;
        visit_locals(func, item, [&](LocalId id) { actual.push_back(id); });
        require_equal(actual, expected);
    }

    template<typename T>
    void require_definitions(const T& item, const std::vector<LocalId>& expected) {
        std::vector<LocalId> actual;
        visit_definitions(func, item, [&](LocalId id) { actual.push_back(id); });
        require_equal(actual, expected);
    }

    template<typename T>
    void require_uses(const T& item, const std::vector<LocalId>& expected) {
        std::vector<LocalId> actual;
        visit_uses(func, item, [&](LocalId id) { actual.push_back(id); });
        require_equal(actual, expected);
    }

    void require_equal(const std::vector<LocalId>& actual, const std::vector<LocalId>& expected) {
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

TEST_CASE("visit_locals() should visit all referenced locals in a block", "[locals]") {
    TestFunction test;

    SECTION("block") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto l2 = test.local();
        auto l3 = test.local(RValue::make_use_lvalue(LValue::make_field(l0, test.string("foo"))));
        auto l4 = test.local(Constant::make_integer(1));

        Block block(test.string("block"));
        block.append_stmt(Stmt::make_assign(LValue::make_index(l0, l1), l2));
        block.append_stmt(Stmt::make_define(l3));
        block.terminator(Terminator::make_branch(BranchType::IfTrue, l4, BlockId(1), BlockId(2)));

        test.require_locals(block, {l0, l1, l2, l3, l0, l4});
    }
}

TEST_CASE("visit_locals() should visit all locals in terminators", "[locals]") {
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

TEST_CASE("visit_locals() should visit all locals in a lvalue", "[locals]") {
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

TEST_CASE("visit_locals() should visit all locals in a rvalue", "[locals]") {
    TestFunction test;

    SECTION("use lvalue") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto rvalue = RValue::make_use_lvalue(LValue::make_index(l0, l1));
        test.require_locals(rvalue, {l0, l1});
    }

    SECTION("use local") {
        auto l0 = test.local();
        test.require_locals(RValue::make_use_local(l0), {l0});
    }

    SECTION("phi") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto phi_id = test.func.make(Phi{l0, l1});
        test.require_locals(RValue::make_phi(phi_id), {l0, l1});
    }

    SECTION("phi0") { test.require_locals(RValue::make_phi0(), {}); }

    SECTION("constant") {
        auto value = RValue::make_constant(Constant::make_integer(123));
        test.require_locals(value, {});
    }

    SECTION("outer environment") { test.require_locals(RValue::make_outer_environment(), {}); }

    SECTION("binary op") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto op = RValue::make_binary_op(BinaryOpType::Plus, l0, l1);
        test.require_locals(op, {l0, l1});
    }

    SECTION("unary op") {
        auto l0 = test.local();
        auto op = RValue::make_unary_op(UnaryOpType::Minus, l0);
        test.require_locals(op, {l0});
    }

    SECTION("call") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto l2 = test.local();
        auto list_id = test.func.make(LocalList{l1, l2});
        auto call = RValue::make_call(l0, list_id);
        test.require_locals(call, {l0, l1, l2});
    }

    SECTION("aggregate") {
        auto l0 = test.local();
        auto method = RValue::make_aggregate(Aggregate::make_method(l0, test.string("foo")));
        test.require_locals(method, {l0});
    }

    SECTION("get aggregate member") {
        auto l0 = test.local();
        auto instance = RValue::make_get_aggregate_member(l0, AggregateMember::MethodInstance);
        test.require_locals(instance, {l0});
    }

    SECTION("method call") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto l2 = test.local();
        auto list_id = test.func.make(LocalList{l1, l2});
        auto call = RValue::make_method_call(l0, list_id);
        test.require_locals(call, {l0, l1, l2});
    }

    SECTION("make environment") {
        auto l0 = test.local();
        auto env = RValue::make_make_environment(l0, 123);
        test.require_locals(env, {l0});
    }

    SECTION("make closure") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto closure = RValue::make_make_closure(l0, l1);
        test.require_locals(closure, {l0, l1});
    }

    SECTION("container") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto list_id = test.func.make(LocalList{l0, l1});
        auto container = RValue::make_container(ContainerType::Array, list_id);
        test.require_locals(container, {l0, l1});
    }

    SECTION("format") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto list_id = test.func.make(LocalList{l0, l1});
        auto format = RValue::make_format(list_id);
        test.require_locals(format, {l0, l1});
    }

    SECTION("error") { test.require_locals(RValue::make_error(), {}); }
}

TEST_CASE("visit_locals() should visit the local's rvalue", "[locals]") {
    TestFunction test;

    auto l0 = test.local();
    auto l1 = test.local();
    auto local = Local(RValue::make_binary_op(BinaryOpType::Plus, l0, l1));
    test.require_locals(local, {l0, l1});
}

TEST_CASE("visit_locals() should visit the phi operands", "[locals]") {
    TestFunction test;

    auto l0 = test.local();
    auto l1 = test.local();
    test.require_locals(Phi{l0, l1}, {l0, l1});
}

TEST_CASE("visit_locals() should visit the list elements", "[locals]") {
    TestFunction test;

    auto l0 = test.local();
    auto l1 = test.local();
    test.require_locals(LocalList{l0, l1}, {l0, l1});
}

TEST_CASE("visit_locals() should visit locals in a statement", "[locals]") {
    TestFunction test;

    SECTION("assignment") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto target = LValue::make_field(l0, test.string("foo"));
        auto stmt = Stmt::make_assign(target, l1);
        test.require_locals(stmt, {l0, l1});
    }

    SECTION("define") {
        auto l0 = test.local();
        auto l1 = test.local();
        auto l2 = test.local(RValue::make_make_closure(l0, l1));
        auto define = Stmt::make_define(l2);
        test.require_locals(define, {l2, l0, l1});
    }
}

TEST_CASE("visit_definitions() only visits the definitions", "[locals]") {
    TestFunction test;

    auto l0 = test.local();
    auto l1 = test.local();
    auto l2 = test.local(RValue::make_make_closure(l0, l1));
    auto define = Stmt::make_define(l2);
    test.require_definitions(define, {l2});
}

TEST_CASE("visit_uses() only visits the definitions", "[locals]") {
    TestFunction test;

    auto l0 = test.local();
    auto l1 = test.local();
    auto l2 = test.local(RValue::make_make_closure(l0, l1));
    auto define = Stmt::make_define(l2);
    test.require_uses(define, {l0, l1});
}
