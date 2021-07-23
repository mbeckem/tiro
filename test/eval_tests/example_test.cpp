#include <catch2/catch.hpp>

#include "eval_test.hpp"

namespace tiro::eval_tests {

TEST_CASE("Interpreter should be able to run recursive fibonacci", "[example]") {
    std::string_view source = R"(
        func fibonacci_slow(i) {
            if (i <= 1) {
                return i;
            }
            return fibonacci_slow(i - 1) + fibonacci_slow(i - 2);
        }

        export func run_fib() = {
            fibonacci_slow(17);
        }
    )";

    eval_test test(source);
    test.call("run_fib").returns_int(1597);
}

TEST_CASE("Interpreter should be able to run iterative fibonacci", "[example]") {
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

        export func run_fib() = {
            fibonacci_fast(80);
        }
    )";

    eval_test test(source);
    test.call("run_fib").returns_int(23416728348467685LL);
}

TEST_CASE(
    "Interpreter should be able to run the iterative fibonacci (tuple "
    "assignment version)",
    "[example]") {
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

        export func run_fib() = {
            fibonacci_fast(80);
        }
    )";

    eval_test test(source);
    test.call("run_fib").returns_int(23416728348467685LL);
}

TEST_CASE("Interpreter should be able to run memoized fibonacci", "[example]") {
    std::string_view source = R"(
        func fibonacci_memo() {
            const m = map{};

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

        export func run_fib() {
            const fib = fibonacci_memo();
            return fib(80);
        }
    )";

    eval_test test(source);
    test.call("run_fib").returns_int(23416728348467685LL);
}

TEST_CASE("Interpreter should compute factorial using a for loop", "[example]") {
    std::string_view source = R"(
        export func factorial() {
            const n = 10;

            var fac = 1;
            for var i = 2; i <= n; i = i + 1 {
                fac = fac * i;
            }
            return fac;
        }
    )";

    eval_test test(source);
    test.call("factorial").returns_int(3'628'800);
}

} // namespace tiro::eval_tests
