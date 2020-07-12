#include "support/test_context.hpp"

#include "vm/objects/array.hpp"

using tiro::Error;

using namespace tiro::vm;

TEST_CASE("Expression blocks should be evaluated correctly", "[eval]") {
    std::string_view source = R"(
        func identity(x) {
            return x;
        }

        export func test() {
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
        export func tick() {
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
        export func tick() {
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
        export func factorial(n) {
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
        export func factorial(n) {
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
        import std;

        export func test() {
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
        export func test() = {
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
        export func test() {
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
        export func test_object(instance) {
            return instance?.foo;
        }

        export func test_tuple(instance) {
            return instance?.1;
        }
    )";

    TestContext test(source);
    Context& ctx = test.ctx();
    Scope sc(ctx);

    // Null object
    test.call("test_object", nullptr).returns_null();

    // Null tuple
    {
        Local null = sc.local(Value::null());
        test.call("test_tuple", null).returns_null();
    }

    // Non-null object
    {
        Local object = sc.local(DynamicObject::make(test.ctx()));
        Local symbol = sc.local(test.ctx().get_symbol("foo"));
        object->set(test.ctx(), symbol, test.make_int(3));
        test.call("test_object", object).returns_int(3);
    }

    // Non-null tuple
    {
        Local tuple = sc.local(Tuple::make(test.ctx(), 2));
        tuple->set(0, *test.make_int(5));
        tuple->set(1, *test.make_int(6));
        test.call("test_tuple", tuple).returns_int(6);
    }
}

TEST_CASE("Optional element access should evaluate to the correct result", "[eval]") {
    std::string_view source = R"(
        export func test_array(instance) {
            return instance?[1];
        }
    )";

    TestContext test(source);
    Context& ctx = test.ctx();
    Scope sc(ctx);

    // Null array
    {
        Local null = sc.local(Value::null());
        test.call("test_array", null).returns_null();
    }

    // Non-null array
    {
        Local array = sc.local(Array::make(test.ctx(), 2));
        array->append(test.ctx(), test.make_string("foo"));
        array->append(test.ctx(), test.make_string("bar"));
        test.call("test_array", array).returns_string("bar");
    }
}

TEST_CASE("Optional call expressions should evaluate to the correct result", "[eval]") {
    std::string_view source = R"(
        export func test_call(fn) {
            return fn?(3);
        }

        export func test_method_instance(instance) {
            return instance?.foo(3);
        }

        export func test_method_function(instance) {
            return instance.foo?(3);
        }

        export func incr(x) {
             return x + 1;
        }
    )";

    TestContext test(source);
    Context& ctx = test.ctx();
    Scope sc(ctx);

    auto incr = test.get_export("incr");

    // Null function
    {

        Local null = sc.local(Value::null());
        test.call("test_call", null).returns_null();
    }

    // Null instance
    {
        Local null = sc.local(Value::null());
        test.call("test_method_instance", null).returns_null();
    }

    // Null member function
    {
        auto foo = test.make_symbol("foo");
        auto null = test.make_null();
        Local object = sc.local(DynamicObject::make(test.ctx()));
        object->set(test.ctx(), foo.must_cast<Symbol>(), null.handle());
        test.call("test_method_function", object).returns_null();
    }

    // Non-null function
    { test.call("test_call", incr.handle()).returns_int(4); }

    // Non-null member function
    {
        auto foo = test.make_symbol("foo");
        auto null = test.make_null();
        Local object = sc.local(DynamicObject::make(test.ctx()));
        object->set(test.ctx(), foo.must_cast<Symbol>(), incr);
        test.call("test_method_function", object).returns_int(4);
    }
}

TEST_CASE("Null coalescing expressions should evaluate to the correct result", "[eval]") {
    std::string_view source = R"(
        export func test(value, alternative) {
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

        export func test() {
            const x = f("World" ?? "no");
            return x;
        }
    )";

    TestContext test(source);
    test.call("test").returns_string("World");
}

TEST_CASE("Deferred statements should be executed correctly", "[eval]") {
    std::string_view source = R"(
        import std;

        // Normal return from function.
        export func test_simple(h, x) = {
            defer h.add("1");
            h.add("2");
            {
                defer h.add("3");
                h.add("4");
            }
            h.get();
        }

        // Normal return from function.
        export func test_conditional(h, x) = {
            defer h.add("1");
            h.add("2");
            {
                defer h.add("3");
                if (x) {
                    defer h.add("4");
                    h.add("5");
                }
            }

            defer h.add("6");
            h.add("7");           
            h.get();
        }

        // Return via early return statement.
        export func test_return(h, x) = {
            defer h.add("1");
            h.add("2");
            if (x) {
                defer h.add("3");
                return h.get();
            } 

            h.add("4");
            h.get();
        }

        // Exit scope via break / continue
        export func test_loop(h, x) = {
            defer h.add("1");

            var stopped = false;
            for (var i = 0; !stopped; i += 1) {
                defer h.add("2");
                h.add("3");
                if (i == 1) {
                    stopped = true;
                    if (x) {
                        defer h.add("4");
                        break;
                    } else {
                        defer h.add("5");
                        continue;
                    }
                }
            }

            h.get();
        }

        // Exit scope with repeated returns in deferred statements
        export func test_nested_returns(h, x) = {
            defer return h.get();
            defer h.add("1");
            defer return "<err2>";            
            defer h.add("2");

            h.add("3");
            "<err1>";
        }

        // Break loop and overwrite return (stupid code!)
        export func test_deferred_break(h, x) = {
            defer h.add("1");

            for (var i = 0; i < 1; i += 1) {
                defer break;
                h.add("2");
                return h.get();
            }

            h.add("3");
            h.get();
        }

        // Continue loop and overwrite return
        export func test_deferred_continue(h, x) = {
            defer h.add("1");

            for (var i = 0; i < 2; i += 1) {
                defer continue;
                h.add("2");
                return h.get();
            }

            h.add("3");
            h.get();    
        }

        // Nested scope with deferred statements inside a deferred statement.
        export func test_nested_defer(h, x) {
            defer h.add("1");

            defer {
                h.add("2");
                defer h.add("3");
                h.add("4");
                return h.get();
            };

            h.add("5");
            return "<err>";
        }

        export func test(fn, x) {
            const h = helper();
            const v1 = fn(h, x);
            const v2 = h.get(); 
            return "$v1-$v2";
        }

        func helper() {
            const helper = std.new_object();
            const builder = std.new_string_builder();

            helper.add = func add(str) {
                builder.append(str);
            };
            helper.get = func get() = {
                builder.to_string();
            };
            return helper;
        }
    )";

    TestContext test(source);

    {
        INFO("simple");
        auto func = test.get_export("test_simple");
        test.call("test", func, true).returns_string("243-2431");
    }

    {
        INFO("conditional");
        auto func = test.get_export("test_conditional");
        test.call("test", func, true).returns_string("25437-2543761");
        test.call("test", func, false).returns_string("237-23761");
    }

    {
        INFO("return");
        auto func = test.get_export("test_return");
        test.call("test", func, true).returns_string("2-231");
        test.call("test", func, false).returns_string("24-241");
    }

    {
        INFO("loop");
        auto func = test.get_export("test_loop");
        test.call("test", func, true).returns_string("32342-323421");
        test.call("test", func, false).returns_string("32352-323521");
    }

    {
        INFO("nested return");
        auto func = test.get_export("test_nested_returns");
        test.call("test", func, true).returns_string("321-321");
    }

    {
        INFO("deferred break");
        auto func = test.get_export("test_deferred_break");
        test.call("test", func, true).returns_string("23-231");
    }

    {
        INFO("deferred continue");
        auto func = test.get_export("test_deferred_continue");
        test.call("test", func, true).returns_string("223-2231");
    }

    {
        INFO("nested defer");
        auto func = test.get_export("test_nested_defer");
        test.call("test", func, true).returns_string("524-52431");
    }
}

TEST_CASE("Deferreds statements should be allowed with valueless expressions", "[eval]") {
    std::string_view source = R"(
        export func test(x, array) {
            defer if (x) {
                array.append(2);
            };  
            array.append(1);
        }
    )";

    TestContext test(source);
    Context& ctx = test.ctx();

    {
        INFO("true");

        Scope sc(ctx);
        Local array = sc.local(Array::make(test.ctx(), 0));
        test.call("test", true, array).returns_null();
        REQUIRE(array->size() == 2);
        REQUIRE(extract_integer(array->get(0)) == 1);
        REQUIRE(extract_integer(array->get(1)) == 2);
    }

    {
        INFO("false");

        Scope sc(ctx);
        Local array = sc.local(Array::make(test.ctx(), 0));
        test.call("test", false, array).returns_null();
        REQUIRE(array->size() == 1);
        REQUIRE(extract_integer(array->get(0)) == 1);
    }
}
