#include <catch.hpp>

#include "compiler/ir_gen/const_eval.hpp"

using namespace tiro;

namespace {

struct ConstantPool {
public:
    Constant str(std::string_view s) { return Constant::make_string(strings_.insert(s)); }

    Constant sym(std::string_view s) { return Constant::make_symbol(strings_.insert(s)); }

    Constant i(i64 v) { return Constant::make_integer(v); }

    Constant f(f64 v) { return Constant::make_float(v); }

    Constant b(bool v) { return v ? Constant::make_true() : Constant::make_false(); }

    Constant n() { return Constant::make_null(); }

    StringTable& strings() { return strings_; }

private:
    StringTable strings_;
};

} // namespace

static constexpr auto min_i64 = std::numeric_limits<i64>::min();
static constexpr auto max_i64 = std::numeric_limits<i64>::max();

static Constant require_constant(const EvalResult& result, const Constant& expected) {
    INFO(fmt::format("result = {}", result));
    INFO(fmt::format("expected = {}", expected));

    REQUIRE(result);
    REQUIRE(*result == expected);
    return *result;
}

static void require_error(const EvalResult& result, EvalResultType expected) {
    TIRO_DEBUG_ASSERT(expected != EvalResultType::Value, "Type must represent an error.");

    INFO(fmt::format("result = {}", result));
    INFO(fmt::format("expected = {}", expected));

    REQUIRE(!result);
    REQUIRE(result.type() == expected);
}

static void
require_error(BinaryOpType op, const Constant& lhs, const Constant& rhs, EvalResultType expected) {

    INFO(fmt::format("op = {}, lhs = {}, rhs = {}", op, lhs, rhs));
    auto result = eval_binary_operation(op, lhs, rhs);
    require_error(result, expected);
}

static void require_error(UnaryOpType op, const Constant& operand, EvalResultType expected) {
    INFO(fmt::format("op = {}, operand = {}", op, operand));
    auto result = eval_unary_operation(op, operand);
    require_error(result, expected);
}

static void require_constant(
    BinaryOpType op, const Constant& lhs, const Constant& rhs, const Constant& expected) {
    INFO(fmt::format("op = {}, lhs = {}, rhs = {}", op, lhs, rhs));
    auto result = eval_binary_operation(op, lhs, rhs);
    require_constant(result, expected);
}

static void require_constant(UnaryOpType op, const Constant& operand, const Constant& expected) {
    INFO(fmt::format("op = {}, operand = {}", op, operand));
    auto result = eval_unary_operation(op, operand);
    require_constant(result, expected);
}

static std::vector<Constant> non_numeric(ConstantPool& c) {
    return {c.b(true), c.b(false), c.n(), c.str("some string"), c.sym("some symbol")};
}

static std::vector<Constant> non_integral(ConstantPool& c) {
    auto nn = non_numeric(c);
    nn.push_back(c.f(123.123));
    return nn;
}

TEST_CASE("Constant evaluation should support addition", "[eval-ir]") {
    const auto plus = BinaryOpType::Plus;
    ConstantPool c;

    require_constant(plus, c.i(123), c.i(1), c.i(124));
    require_error(plus, c.i(min_i64), c.i(-1), EvalResultType::IntegerOverflow);
    require_error(plus, c.i(max_i64), c.i(1), EvalResultType::IntegerOverflow);

    require_constant(plus, c.f(555), c.f(333), c.f(888));
}

TEST_CASE("Constant evaluation should support subtraction", "[eval-ir]") {
    const auto sub = BinaryOpType::Minus;
    ConstantPool c;

    require_constant(sub, c.i(123), c.i(1), c.i(122));
    require_error(sub, c.i(min_i64), c.i(1), EvalResultType::IntegerOverflow);
    require_error(sub, c.i(max_i64), c.i(-1), EvalResultType::IntegerOverflow);
}

TEST_CASE("Constant evaluation should support multiplicaton", "[eval-ir]") {
    const auto mul = BinaryOpType::Multiply;
    ConstantPool c;

    require_constant(mul, c.i(123), c.i(2), c.i(246));
    require_error(mul, c.i(1 + max_i64 / 2), c.i(2), EvalResultType::IntegerOverflow);
    require_error(mul, c.i(-1 + min_i64 / 2), c.i(2), EvalResultType::IntegerOverflow);

    require_constant(mul, c.f(999), c.f(-10), c.f(-9990));
    require_constant(mul, c.f(999), c.f(.1), c.f(99.9));
}

TEST_CASE("Constant evaluation should support division", "[eval-ir]") {
    const auto div = BinaryOpType::Divide;
    ConstantPool c;

    require_constant(div, c.i(999), c.i(9), c.i(111));
    require_error(div, c.i(132), c.i(0), EvalResultType::DivideByZero);
    require_error(div, c.i(min_i64), c.i(-1), EvalResultType::IntegerOverflow);

    require_constant(div, c.f(99.0), c.f(10), c.f(9.9));
}

TEST_CASE("Constant evaluation should support remainder", "[eval-ir]") {
    const auto rem = BinaryOpType::Modulus;
    ConstantPool c;

    require_constant(rem, c.i(55), c.i(21), c.i(13));
    require_constant(rem, c.i(-55), c.i(21), c.i(-13));
    require_error(rem, c.i(55), c.i(0), EvalResultType::DivideByZero);

    require_constant(rem, c.f(10), c.f(6), c.f(4));
    require_constant(rem, c.f(9.5), c.f(1.5), c.f(0.5)); // fmod
}

TEST_CASE("Constant evaluation should support powers", "[eval-ir]") {
    const auto pow = BinaryOpType::Power;
    ConstantPool c;

    require_constant(pow, c.i(4), c.i(3), c.i(64));
    require_constant(pow, c.i(-4), c.i(3), c.i(-64));
    require_constant(pow, c.i(123), c.i(0), c.i(1));
    require_constant(pow, c.i(123), c.i(-1), c.i(0));
    require_constant(pow, c.i(0), c.i(0), c.i(1));
    require_error(pow, c.i(0), c.i(-1), EvalResultType::DivideByZero);
    require_error(pow, c.i(max_i64), c.i(2), EvalResultType::IntegerOverflow);

    require_constant(pow, c.f(1.5), c.f(2), c.f(2.25));
}

TEST_CASE(
    "Constant evaluation of arithmetic binary operators should error on "
    "non-numeric input",
    "[eval-ir]") {
    ConstantPool c;

    const auto invalid = non_numeric(c);

    const BinaryOpType operators[] = {
        BinaryOpType::Plus,
        BinaryOpType::Minus,
        BinaryOpType::Multiply,
        BinaryOpType::Divide,
        BinaryOpType::Modulus,
        BinaryOpType::Power,
    };

    auto test = [&](const auto& lhs, const auto& rhs) {
        for (auto op : operators) {
            require_error(op, lhs, rhs, EvalResultType::TypeError);
            require_error(op, rhs, lhs, EvalResultType::TypeError);
        }
    };

    // Invalid op invalid
    for (size_t i = 0; i < std::size(invalid); ++i) {
        const auto& lhs = invalid[i];
        for (size_t j = i + 1; j < std::size(invalid); ++j) {
            const auto& rhs = invalid[j];
            test(lhs, rhs);
        }
    }

    // Valid op invalid
    for (const auto& operand : invalid) {
        test(c.i(123), operand);
        test(c.f(123.123), operand);
    }
}

TEST_CASE("Constant evaluation should support left shift for integers", "[eval-ir]") {
    const auto lsh = BinaryOpType::LeftShift;
    ConstantPool c;

    require_constant(lsh, c.i(0), c.i(0), c.i(0));
    require_constant(lsh, c.i(0), c.i(8), c.i(0));
    require_constant(lsh, c.i(1), c.i(16), c.i(65536));
    require_constant(lsh, c.i(1), c.i(63), c.i(min_i64));
    require_constant(lsh, c.i(3), c.i(3), c.i(24));

    require_error(lsh, c.i(0), c.i(64), EvalResultType::IntegerOverflow);
    require_error(lsh, c.i(0), c.i(-1), EvalResultType::NegativeShift);
}

TEST_CASE("Constant evaluation should support right shift for integers", "[eval-ir]") {
    const auto rsh = BinaryOpType::RightShift;
    ConstantPool c;

    require_constant(rsh, c.i(0), c.i(0), c.i(0));
    require_constant(rsh, c.i(0), c.i(8), c.i(0));
    require_constant(rsh, c.i(65536), c.i(16), c.i(1));
    require_constant(rsh, c.i(65536), c.i(17), c.i(0));
    require_constant(rsh, c.i(min_i64), c.i(63), c.i(1));
    require_constant(rsh, c.i(24), c.i(3), c.i(3));

    require_error(rsh, c.i(0), c.i(64), EvalResultType::IntegerOverflow);
    require_error(rsh, c.i(0), c.i(-1), EvalResultType::NegativeShift);
}

TEST_CASE("Constant evaluation should support bitwise and for integers", "[eval-ir]") {
    const auto band = BinaryOpType::BitwiseAnd;
    ConstantPool c;

    require_constant(band, c.i(7), c.i(2), c.i(2));
    require_constant(band, c.i(-1), c.i(555), c.i(555));
    require_constant(band, c.i(0), c.i(123456), c.i(0));
    require_constant(band, c.i(1023), c.i(~512), c.i(511));
}

TEST_CASE("Constant evaluation should support bitwise or for integers", "[eval-ir]") {
    const auto bor = BinaryOpType::BitwiseOr;
    ConstantPool c;

    require_constant(bor, c.i(7), c.i(8), c.i(15));
    require_constant(bor, c.i(-1 & ~7), c.i(7), c.i(-1));
    require_constant(bor, c.i(0), c.i(9999), c.i(9999));
    require_constant(bor, c.i(7), c.i(8), c.i(15));
}

TEST_CASE("Constant evaluation should support bitwise xor for integers", "[eval-ir]") {
    const auto bxor = BinaryOpType::BitwiseXor;
    ConstantPool c;

    require_constant(bxor, c.i(123), c.i(123), c.i(0));
    require_constant(bxor, c.i(8), c.i(7), c.i(15));
    require_constant(bxor, c.i(7), c.i(5), c.i(2));
}

TEST_CASE(
    "Constant evaluation of bitwise binare operators should error on "
    "non-integer input",
    "[eval-ir]") {
    ConstantPool c;

    const auto invalid = non_integral(c);

    const BinaryOpType operators[] = {
        BinaryOpType::LeftShift,
        BinaryOpType::RightShift,
        BinaryOpType::BitwiseAnd,
        BinaryOpType::BitwiseOr,
        BinaryOpType::BitwiseXor,
    };

    auto test = [&](const auto& lhs, const auto& rhs) {
        for (auto op : operators) {
            require_error(op, lhs, rhs, EvalResultType::TypeError);
            require_error(op, rhs, lhs, EvalResultType::TypeError);
        }
    };

    // Invalid op invalid
    for (size_t i = 0; i < std::size(invalid); ++i) {
        const auto& lhs = invalid[i];
        for (size_t j = i + 1; j < std::size(invalid); ++j) {
            const auto& rhs = invalid[j];
            test(lhs, rhs);
        }
    }

    // Valid op invalid
    for (const auto& operand : invalid) {
        test(c.i(123), operand);
    }
}

TEST_CASE("Constant evaluation should support equality", "[eval-ir]") {
    ConstantPool c;

    auto test = [&](const auto& lhs, const auto& rhs, bool eq) {
        require_constant(BinaryOpType::Equals, lhs, rhs, c.b(eq));
        require_constant(BinaryOpType::Equals, rhs, lhs, c.b(eq));
        require_constant(BinaryOpType::NotEquals, lhs, rhs, c.b(!eq));
        require_constant(BinaryOpType::NotEquals, rhs, lhs, c.b(!eq));
    };

    test(c.i(123), c.i(123), true);
    test(c.i(-1), c.i(1), false);
    test(c.f(1), c.i(1), true);
    test(c.f(-1), c.i(1), false);
    test(c.f(-12312), c.i(-12312), true);
    test(c.f(-12312), c.i(12312), false);
    test(c.f(std::nan("1")), c.f(std::nan("1")), false);

    test(c.sym("foo123"), c.sym("foo123"), true);
    test(c.sym("foo1234"), c.sym("foo123"), false);
    test(c.str("foo123"), c.str("foo123"), true);
    test(c.str("foo124"), c.str("foo123"), false);
    test(c.sym("foo123"), c.str("foo123"), false);

    test(c.str("4"), c.i(4), false);
    test(c.str("4"), c.f(4), false);
    test(c.str("4.0"), c.f(4), false);
    test(c.str("true"), c.b(true), false);
    test(c.str("false"), c.b(false), false);

    test(c.n(), c.n(), true);
    test(c.n(), c.b(true), false);
    test(c.n(), c.b(false), false);
    test(c.n(), c.i(0), false);
    test(c.n(), c.f(0), false);
    test(c.n(), c.str(""), false);
    test(c.n(), c.sym(""), false);

    test(c.b(true), c.b(true), true);
    test(c.b(false), c.b(false), true);
    test(c.b(true), c.b(false), false);
}

TEST_CASE("Constant evaluation should support inequality", "[eval-ir]") {
    ConstantPool c;

    auto test = [&](const auto& lhs, const auto& rhs, int expected) {
        require_constant(BinaryOpType::Less, lhs, rhs, c.b(expected < 0));
        require_constant(BinaryOpType::Greater, rhs, lhs, c.b(expected < 0));

        require_constant(BinaryOpType::Greater, lhs, rhs, c.b(expected > 0));
        require_constant(BinaryOpType::Less, rhs, lhs, c.b(expected > 0));

        require_constant(BinaryOpType::GreaterEquals, lhs, rhs, c.b(expected >= 0));
        require_constant(BinaryOpType::LessEquals, rhs, lhs, c.b(expected >= 0));

        require_constant(BinaryOpType::LessEquals, lhs, rhs, c.b(expected <= 0));
        require_constant(BinaryOpType::GreaterEquals, rhs, lhs, c.b(expected <= 0));
    };

    test(c.i(0), c.i(0), 0);
    test(c.i(-1), c.i(11), -1);
    test(c.i(1), c.i(-11), 1);

    test(c.i(123124), c.f(123124), 0);
    test(c.i(-5), c.f(-4.99), -1);
    test(c.i(99), c.f(98.999), 1);

    test(c.f(0), c.f(0), 0);
    test(c.f(-1), c.f(11), -1);
    test(c.f(1), c.f(-11), 1);
}

TEST_CASE("Constant evaluation should support unary plus", "[eval-ir]") {
    const auto plus = UnaryOpType::Plus;
    ConstantPool c;

    require_constant(plus, c.i(0), c.i(0));
    require_constant(plus, c.i(12345), c.i(12345));
    require_constant(plus, c.f(0), c.f(0));
    require_constant(plus, c.f(12345.12345), c.f(12345.12345));
}

TEST_CASE("Constant evaluation should support unary minus", "[eval-ir]") {
    const auto minus = UnaryOpType::Minus;
    ConstantPool c;

    require_constant(minus, c.i(0), c.i(0));
    require_constant(minus, c.i(12345), c.i(-12345));
    require_constant(minus, c.f(0), c.f(0));
    require_constant(minus, c.f(-12345.12345), c.f(12345.12345));
}

TEST_CASE(
    "Constant evaluation of unary arithmetic operators should error on "
    "non-numeric input",
    "[eval-ir]") {
    ConstantPool c;
    const auto invalid = non_numeric(c);
    for (const auto& operand : invalid) {
        require_error(UnaryOpType::Plus, operand, EvalResultType::TypeError);
        require_error(UnaryOpType::Minus, operand, EvalResultType::TypeError);
    }
}

TEST_CASE("Constant evaluation should support bitwise not", "[eval-ir]") {
    const auto bnot = UnaryOpType::BitwiseNot;
    ConstantPool c;

    require_constant(bnot, c.i(0), c.i(-1));
    require_constant(bnot, c.i(-12346), c.i(12345));

    require_error(bnot, c.f(123), EvalResultType::TypeError);
    require_error(bnot, c.b(true), EvalResultType::TypeError);
}

TEST_CASE("Constant evaluation of bitwise not should error on non-integral input", "[eval-ir]") {
    ConstantPool c;
    const auto invalid = non_integral(c);

    for (const auto& operand : invalid) {
        require_error(UnaryOpType::BitwiseNot, operand, EvalResultType::TypeError);
    }
}

TEST_CASE("Constant evaluation should support logical not", "[eval-ir]") {
    const auto lnot = UnaryOpType::LogicalNot;
    ConstantPool c;

    require_constant(lnot, c.n(), c.b(true));
    require_constant(lnot, c.b(false), c.b(true));

    require_constant(lnot, c.b(true), c.b(false));
    require_constant(lnot, c.i(0), c.b(false));
    require_constant(lnot, c.i(123), c.b(false));
    require_constant(lnot, c.f(0), c.b(false));
    require_constant(lnot, c.f(-123.123), c.b(false));
    require_constant(lnot, c.str(""), c.b(false));
    require_constant(lnot, c.str("123"), c.b(false));
    require_constant(lnot, c.sym(""), c.b(false));
    require_constant(lnot, c.sym("abc"), c.b(false));
}

TEST_CASE("Constant evaluation should support string formatting", "[eval-ir]") {
    ConstantPool c;

    auto space = c.str(" ");
    std::vector<Constant> args{c.n(), space, c.b(true), space, c.b(false), space, c.sym("sym123"),
        space, c.i(-55), space, c.f(123.123), c.str("!")};

    auto expected = c.str("null true false #sym123 -55 123.123!");
    auto result = eval_format(args, c.strings());
    require_constant(result, expected);
}
