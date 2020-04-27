#include "./eval_context.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Interpreter should be able to run recursive fibonacci", "[eval]") {
    std::string_view source = R"(
        func fibonacci_slow(i) {
            if (i <= 1) {
                return i;
            }
            return fibonacci_slow(i - 1) + fibonacci_slow(i - 2);
        }

        func run_fib() {
            fibonacci_slow(20);
        }
    )";

    TestContext test(source);
    auto result = test.run("run_fib");
    REQUIRE(extract_integer(result) == 6765);
}

TEST_CASE("Interpreter should be able to run iterative fibonacci", "[eval]") {
    std::string_view source = R"(
        func fibonacci_fast(i) {
            if (i <= 1) {
                return i;
            }

            var a = 0;
            var b = 1;
            while (i >= 2) {
                var c = a + b;
                a = b;
                b = c;
                i = i - 1;
            }
            return b;
        }

        func run_fib() {
            fibonacci_fast(80);
        }
    )";

    TestContext test(source);
    auto result = test.run("run_fib");
    REQUIRE(extract_integer(result) == 23416728348467685);
}

TEST_CASE(
    "Interpreter should be able to run the iterative fibonacci (tuple "
    "assignment version)",
    "[eval]") {
    std::string_view source = R"(
        func fibonacci_fast(i) {
            if (i <= 1) {
                return i;
            }

            var a = 0;
            var b = 1;
            while (i >= 2) {
                (a, b) = (b, a + b);
                i = i - 1;
            }
            return b;
        }

        func run_fib() {
            fibonacci_fast(80);
        }
    )";

    TestContext test(source);
    auto result = test.run("run_fib");
    REQUIRE(extract_integer(result) == 23416728348467685);
}

TEST_CASE("Interpreter should be able to run memoized fibonacci", "[eval]") {
    std::string_view source = R"(
        func fibonacci_memo() {
            const m = Map{};

            var fib;
            fib = func(i) {
                if (m.contains(i)) {
                    return m[i];
                }

                const result = if (i <= 1) {
                    i;
                } else {
                    fib(i - 1) + fib(i - 2);
                };
                return m[i] = result;
            };
            return fib;
        }

        func run_fib() {
            const fib = fibonacci_memo();
            return fib(80);
        }
    )";

    TestContext test(source);
    auto result = test.run("run_fib");
    REQUIRE(extract_integer(result) == 23416728348467685);
}

TEST_CASE("Interpreter should compute factorial using a for loop", "[eval]") {
    std::string_view source = R"(
        func factorial() {
            const n = 10;

            var fac = 1;
            for (var i = 2; i <= n; i = i + 1) {
                fac = fac * i;
            }
            return fac;
        }
    )";

    TestContext test(source);
    auto result = test.run("factorial");
    REQUIRE(extract_integer(result) == 3'628'800);
}
