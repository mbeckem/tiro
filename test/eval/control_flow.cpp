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

TEST_CASE("Interpreter should allow assertions with interpolated string contents", "[eval]") {
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

TEST_CASE("Multiple variables in for loop initializer should be supported", "[eval]") {
    std::string_view source = R"RAW(
        func test() {
            const nums = [1, 2, 3, 4, 5];
            var sum = 0;

            for (var i = 0, n = nums.size(); i < n; i = i + 1) {
                sum = sum + nums[i];
            }

            return sum;
        }
    )RAW";

    TestContext test(source);
    test.call("test").returns_int(15);
}

TEST_CASE("Break can be used in nested expressions", "[eval]") {
    std::string_view source = R"(
        func test() = {
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

TEST_CASE("Return from nested expression should compile and execute", "[eval]") {
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

TEST_CASE("Optional property access should evaluate to the correct result", "[eval]") {
    std::string_view source = R"(
        func test_object(instance) {
            return instance?.foo;
        }

        func test_tuple(instance) {
            return instance?.1;
        }
    )";

    TestContext test(source);

    // Null object
    test.call("test_object", nullptr).returns_null();

    // Null tuple
    {
        Root<Value> null(test.ctx(), Value::null());
        test.call("test_tuple", null.handle()).returns_null();
    }

    // Non-null object
    {
        Root<DynamicObject> object(test.ctx(), DynamicObject::make(test.ctx()));
        Root<vm::Symbol> symbol(test.ctx(), test.ctx().get_symbol("foo"));
        object->set(test.ctx(), symbol, test.make_int(3));
        test.call("test_object", object.handle()).returns_int(3);
    }

    // Non-null tuple
    {
        Root<Tuple> tuple(test.ctx(), Tuple::make(test.ctx(), 2));
        tuple->set(0, test.make_int(5));
        tuple->set(1, test.make_int(6));
        test.call("test_tuple", tuple.handle()).returns_int(6);
    }
}

TEST_CASE("Optional element access should evaluate to the correct result", "[eval]") {
    std::string_view source = R"(
        func test_array(instance) {
            return instance?[1];
        }
    )";

    TestContext test(source);

    // Null array
    {
        Root<Value> null(test.ctx(), Value::null());
        test.call("test_array", null.handle()).returns_null();
    }

    // Non-null array
    {
        Root<Array> array(test.ctx(), Array::make(test.ctx(), 2));
        array->append(test.ctx(), test.make_string("foo"));
        array->append(test.ctx(), test.make_string("bar"));
        test.call("test_array", array.handle()).returns_string("bar");
    }
}

TEST_CASE("Optional call expressions should evaluate to the correct result", "[eval]") {
    std::string_view source = R"(
        func test_call(fn) {
            return fn?(3);
        }

        func test_method_instance(instance) {
            return instance?.foo(3);
        }

        func test_method_function(instance) {
            return instance.foo?(3);
        }

        func incr(x) {
             return x + 1;
        }
    )";

    TestContext test(source);

    auto incr = test.get_function("incr");

    // Null function
    {

        Root<Value> null(test.ctx(), Value::null());
        test.call("test_call", null.handle()).returns_null();
    }

    // Null instance
    {
        Root<Value> null(test.ctx(), Value::null());
        test.call("test_method_instance", null.handle()).returns_null();
    }

    // Null member function
    {
        auto foo = test.make_symbol("foo");
        auto null = test.make_null();
        Root<DynamicObject> object(test.ctx(), DynamicObject::make(test.ctx()));
        object->set(test.ctx(), foo.handle().strict_cast<vm::Symbol>(), null.handle());
        test.call("test_method_function", object.handle()).returns_null();
    }

    // Non-null function
    { test.call("test_call", incr.handle()).returns_int(4); }

    // Non-null member function
    {
        auto foo = test.make_symbol("foo");
        auto null = test.make_null();
        Root<DynamicObject> object(test.ctx(), DynamicObject::make(test.ctx()));
        object->set(test.ctx(), foo.handle().strict_cast<vm::Symbol>(), incr.handle());
        test.call("test_method_function", object.handle()).returns_int(4);
    }
}

TEST_CASE("Null coalescing expressions should evaluate to the correct result", "[eval]") {
    std::string_view source = R"(
        func test(value, alternative) {
            return value ?? alternative;
        }
    )";

    TestContext test(source);
    test.call("test", nullptr, test.make_int(3)).returns_int(3);
    test.call("test", 123, 4).returns_int(123);
}

TEST_CASE(
    "Regression test: code produced for short circuiting expressions does not result in "
    "unreachable code",
    "[eval]") {
    std::string_view source = R"(
        func f(x) {
            return x;
        }

        func test() {
            const x = f("World" ?? "no");
            return x;
        }
    )";

    TestContext test(source);
    test.call("test").returns_string("World");
}
