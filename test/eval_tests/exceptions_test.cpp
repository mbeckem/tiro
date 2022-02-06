#include <catch2/catch.hpp>

#include "eval_test.hpp"

namespace tiro::eval_tests {

TEST_CASE("User defined code should be able to panic", "[exceptions]") {
    std::string_view source = R"RAW(
        import std;

        export func test(value) {
            std.panic(value);
        }
    )RAW";

    eval_test test(source);
    auto ex = test.call("test", "my error message").panics().as<exception>();
    REQUIRE(ex.message().view() == "my error message");
}

TEST_CASE("Defer statements should run when a function panics", "[exceptions]") {
    std::string_view source = R"RAW(
        import std;

        export func test(tuple) {
            defer tuple[1] = 2;
            defer tuple[0] = 1;
            std.panic("help!");
        }
    )RAW";

    eval_test test(source);
    auto& vm = test.get_vm();
    auto tuple = make_tuple(vm, 2);
    tuple.set(0, make_integer(vm, 0));
    tuple.set(1, make_integer(vm, 0));

    test.call("test", tuple).panics();
    REQUIRE(tuple.get(0).as<integer>().value() == 1);
    REQUIRE(tuple.get(1).as<integer>().value() == 2);
}

TEST_CASE(
    "Defer statements should observe variable assignments when a function panics", "[exceptions]") {
    std::string_view source = R"RAW(
        import std;

        export func test(tuple) {
            var x = 1;
            defer tuple[0] = x;
            no_throw();
            x = 2;
            std.panic("help!");
        }

        func no_throw() {}
    )RAW";

    eval_test test(source);
    auto& vm = test.get_vm();
    auto tuple = make_tuple(vm, 1);
    tuple.set(0, make_integer(vm, 0));
    test.call("test", tuple).panics();
    REQUIRE(tuple.get(0).as<integer>().value() == 2);
}

TEST_CASE("Defer statements in callers should be executed when a callee panics", "[exceptions]") {
    std::string_view source = R"RAW(
        import std;

        export func test(array) {
            defer array.append("test1");
            defer array.append("test2");
            a(array);
        }

        func a(array) {
            defer array.append("a");
            b(array);

            defer array.append("NEVER_REACHED (a)");
        }

        func b(array) {
            // b does not have a handler
            c(array);
        }

        func c(array) {
            defer array.append("c");
            std.panic("help!");

            defer array.append("NEVER_REACHED (c)");
        }
    )RAW";

    eval_test test(source);
    auto& vm = test.get_vm();
    auto array = make_array(vm);
    test.call("test", array).panics();
    REQUIRE(array.size() == 4);

    auto test_string = [&](size_t index, std::string_view expected) {
        CAPTURE(index);
        auto value = array.get(index).as<string>();
        REQUIRE(value.view() == expected);
    };
    test_string(0, "c");
    test_string(1, "a");
    test_string(2, "test2");
    test_string(3, "test1");
}

TEST_CASE("Panics should be registered as secondary exceptions if another exception is in flight",
    "[exceptions]") {
    /* TODO: secondary exceptions
    std::string_view source = R"RAW(
        import std;

        export func test() {
            defer std.panic("test-secondary-1");
            defer nested();
            defer std.panic("test-secondary-2");

            std.panic("test-panic");
        }

        func nested() {
            defer std.panic("nested-secondary-1");
            std.panic("nested-panic");
        }
    )RAW";

    eval_test test(source);

    auto require_message = [&](Value ex, std::string_view expected) {
        REQUIRE(ex.is<Exception>());
        std::string_view message = ex.must_cast<Exception>().message().view();
        CAPTURE(message, expected);

        bool contains_message = message.find(expected) != std::string_view::npos;
        REQUIRE(contains_message);
    };

    Scope sc(test.ctx());

    Local root_exception = sc.local(test.call("test").panics());
    require_message(*root_exception, "test-panic");
    REQUIRE(root_exception->secondary().has_value());

    Local root_secondaries = sc.local(root_exception->secondary().value());
    REQUIRE(root_secondaries->size() == 3);
    require_message(root_secondaries->get(0), "test-secondary-2");
    require_message(root_secondaries->get(1), "nested-panic");
    require_message(root_secondaries->get(2), "test-secondary-1");

    Local nested_exception = sc.local(root_secondaries->get(1).must_cast<Exception>());
    REQUIRE(nested_exception->secondary().has_value());

    Local nested_secondaries = sc.local(nested_exception->secondary().value());
    REQUIRE(nested_secondaries->size() == 1);
    require_message(nested_secondaries->get(0), "nested-secondary-1");

    */
}

TEST_CASE("catch_panic should forward normal returns as successful results", "[exceptions]") {
    std::string_view source = R"RAW(
        import std;

        export func test() {
            return std.catch_panic(func() = 123);
        }
    )RAW";

    eval_test test(source);
    auto res = test.call("test").returns_value().as<result>();
    REQUIRE(res.is_success());
    REQUIRE(res.value().as<integer>().value() == 123);
}

TEST_CASE("catch_panic should forward panics as failed results", "[exceptions]") {
    std::string_view source = R"RAW(
        import std;

        export func test() {
            return std.catch_panic(do_panic);
        }

        func do_panic() {
            std.panic("help!");
        }
    )RAW";

    eval_test test(source);
    auto res = test.call("test").returns_value().as<result>();
    REQUIRE(res.is_error());

    auto ex = res.error().as<exception>();
    REQUIRE(ex.message().view() == "help!");
}

TEST_CASE("panic should be able to rethrow existing exceptions", "[exceptions]") {
    std::string_view source = R"RAW(
        import std;

        // Cannot create exceptions using the API yet
        export func make_exception() {
            std.panic("help!");
        }

        export func test(ex) {
            std.panic(ex);
        }
    )RAW";

    eval_test test(source);
    auto ex = test.call("make_exception").panics().as<exception>();
    auto res = test.call("test", ex).panics();
    REQUIRE(same(test.get_vm(), ex, res));
}

TEST_CASE("panic should not capture a stack trace by default", "[exceptions]") {
    std::string_view source = R"RAW(
        import std;

        export func test() {
            nested(3);
        }

        func nested(n) {
            if n == 0 {
                std.panic("help!");
            }
            nested(n - 1);
        }
    )RAW";

    eval_test test(source);
    auto ex = test.call("test").panics().as<exception>();
    auto trace = ex.trace();
    REQUIRE(trace.is<null>());
}

TEST_CASE("panic should capture a stack trace when enabled", "[exceptions]") {
    std::string_view source = R"RAW(
        import std;

        export func test() {
            nested(2);
        }

        func nested(n) {
            if n == 0 {
                std.panic("help!");
            }
            nested(n - 1);
        }
    )RAW";

    eval_test test(source, eval_test::enable_panic_stack_traces);
    auto ex = test.call("test").panics().as<exception>();
    auto trace = ex.trace().as<string>();
    std::string_view expected = "Coroutine-1:\n  - nested\n  - nested\n  - nested\n  - test";
    REQUIRE(trace.value() == expected);
}

TEST_CASE("invalid usage of builtin operators should panic instead of throwing c++ exceptions",
    "[exceptions]") {
    std::string_view source = R"RAW(
        import std;

        export func catch_missing_method() = panic_helper(func() {
            const record = (:);
            record.foo(1, 2, 3);
        });

        export func catch_missing_args_in_free_func() = panic_helper(func() {
            const fn = func(a, b, c) = a + b + c;
            fn(1, 2);
        });

        export func catch_missing_args_in_method() = panic_helper(func() {
            const obj = (
                method: func(a, b) {
                    return a + b;
                }
            );
            obj.method(1);
        });

        export func catch_object_not_callable() = panic_helper(func() {
            const obj = 4;
            obj();
        });

        export func catch_array_index_not_an_integer() = panic_helper(func() {
            const array = [];
            return array["foo"];
        });

        export func catch_array_get_index_out_of_bounds() = panic_helper(func() {
            const array = [1, 2];
            return array[2];
        });

        export func catch_array_set_index_out_of_bounds() = panic_helper(func() {
            const array = [1, 2];
            array[2] = 3;
        });

        export func catch_tuple_index_not_an_integer() = panic_helper(func() {
            const tuple = ();
            return tuple["foo"];
        });

        export func catch_tuple_get_index_out_of_bounds() = panic_helper(func() {
            const tuple = (1, 2);
            return tuple[2];
        });

        export func catch_tuple_set_index_out_of_bounds() = panic_helper(func() {
            const tuple = (1, 2);
            tuple[2] = 3;
        });

        export func catch_get_index_not_supported() = panic_helper(func() {
            const obj = null;
            obj[1];
        });

        export func catch_set_index_not_supported() = panic_helper(func() {
            const obj = null;
            obj[1] = 1;
        });

        export func catch_module_member_not_found() = panic_helper(func() {
            const foo = std.does_not_exist;
        });

        export func catch_type_member_not_found() = panic_helper(func() {
            const foo = std.Integer.does_not_exist;
        });

        export func catch_instance_member_not_found() = panic_helper(func() {
            const record = (foo: 4);
            const bar = record.bar;
        });

        export func catch_member_assignment_not_supported() = panic_helper(func() {
            const foo = null;
            foo.bar = "baz";
        });

        export func catch_store_member_not_found() = panic_helper(func() {
            const record = (foo: 3);
            record.bar = 4;
        });

        export func catch_module_function_not_found() = panic_helper(func() {
            std.does_not_exist();
        });

        export func catch_method_not_found() = panic_helper(func() {
            null.does_not_exist();
        });

        export func catch_non_iterable() = panic_helper(func() {
            for foo in true {
                std.print(foo);
            }
        });

        func panic_helper(fn) {
            const result = std.catch_panic(fn);
            assert(result.is_error(), "function must have panicked");
            return true;
        }
    )RAW";

    eval_test test(source);

    const char* tests[] = {
        // Function calls
        "catch_missing_method",
        "catch_missing_args_in_free_func",
        "catch_missing_args_in_method",
        "catch_object_not_callable",

        // Index operations (TODO: Buffer)
        "catch_array_index_not_an_integer",
        "catch_array_get_index_out_of_bounds",
        "catch_array_set_index_out_of_bounds",
        "catch_tuple_index_not_an_integer",
        "catch_tuple_get_index_out_of_bounds",
        "catch_tuple_set_index_out_of_bounds",
        "catch_get_index_not_supported",
        "catch_set_index_not_supported",

        // Members
        "catch_module_member_not_found",
        "catch_type_member_not_found",
        "catch_instance_member_not_found",
        "catch_member_assignment_not_supported",
        "catch_store_member_not_found",

        // Methods
        "catch_module_function_not_found",
        "catch_method_not_found",

        // Iteration support
        "catch_non_iterable",
    };

    for (const auto& test_name : tests) {
        CAPTURE(test_name);
        test.call(test_name).returns_bool(true);
    }
}

} // namespace tiro::eval_tests
