#include "./eval_context.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Expression blocks should be evaluated correctly", "[eval]") {
    std::string_view source = R"(
        func identity(x) {
            return x;
        }

        func test() {
            return {
                const x = identity({
                    var foo = 4;
                    foo;
                });

                if (x) {
                    { x; }; // Intentionally stupid
                } else {
                    return -1;
                }
            };
        }
    )";

    TestContext test(source);
    test.call("test").returns_int(4);
}

TEST_CASE("Interpreter should throw an exception on assert failure", "[eval]") {
    std::string_view source = R"(
        func tick() {
            assert(false, "boom!");
        }
    )";

    TestContext test(source);
    try {
        test.call("tick").run();
        FAIL("Must throw an error.");
    } catch (const Error& e) {
        std::string msg = e.what();
        bool found = msg.find("boom!") != std::string::npos;
        REQUIRE(found);
    } catch (const std::exception& e) {
        FAIL("Unexpected exception: " << e.what());
    } catch (...) {
        FAIL("Unexpected exception type.");
    }
}

TEST_CASE(
    "Interpreter should allow assertions with interpolated string contents",
    "[eval]") {
    std::string_view source = R"(
        func tick() {
            const x = "tick tick...";
            assert(false, "${x} boom!");
        }
    )";

    TestContext test(source);
    try {
        test.call("tick").run();
        FAIL("Must throw an error.");
    } catch (const Error& e) {
        std::string msg = e.what();
        bool found = msg.find("tick tick... boom!") != std::string::npos;
        REQUIRE(found);
    } catch (const std::exception& e) {
        FAIL("Unexpected exception: " << e.what());
    } catch (...) {
        FAIL("Unexpected exception type.");
    }
}

TEST_CASE("Simple for loops should be supported", "[eval]") {
    std::string_view source = R"RAW(
        func factorial(n) {
            var result = 1;
            for (var i = 2; i <= n; i += 1) {
                result *= i;
            }
            return result;
        }
    )RAW";

    TestContext test(source);
    test.call("factorial", 7).returns_int(5040);
}

TEST_CASE("Simple while loops should be supported", "[eval]") {
    std::string_view source = R"RAW(
        func factorial(n) {
            var result = 1;
            var i = 2;
            while (i <= n) {
                result *= i;
                i += 1;
            }
            return result;
        }
    )RAW";

    TestContext test(source);
    test.call("factorial", 7).returns_int(5040);
}

TEST_CASE("Multiple variables in for loop initializer should be supported",
    "[eval]") {
    std::string_view source = R"RAW(
        func test() {
            const nums = [1, 2, 3, 4, 5];
            var sum = 0;

            for (var i = 0, n = nums.size(); i < n; i = i + 1) {
                sum = sum + nums[i];
            }
            sum;
        }
    )RAW";

    TestContext test(source);
    test.call("test").returns_int(15);
}

TEST_CASE("Break can be used in nested expressions", "[eval]") {
    std::string_view source = R"(
        func test() {
            const foo = 1 + {
                while (1) {
                    var x = 99 + (3 + break);
                }
                2;
            };
            foo;
        }
    )";

    TestContext test(source);
    test.call("test").returns_int(3);
}

TEST_CASE(
    "Return from nested expression should compile and execute", "[eval]") {
    std::string_view source = R"(
        func test() {
            const x = 1 + {
                if (condition()) {
                    return 7;
                }
                2;
            };
            return x;
        }

        func condition() {
            return true;
        }
    )";

    TestContext test(source);
    test.call("test").returns_int(7);
}
