#include "./test_context.hpp"

namespace tiro::vm::test {

TEST_CASE("The current coroutine should be accessible", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test() {
            return std.current_coroutine().name();
        }
    )";

    TestContext test(source);
    test.call("test").returns_string("Coroutine-1");
}

TEST_CASE("Coroutines should support manual yield and resume", "[eval]") {
    std::string_view source = R"(
        import std;

        var coroutine = null;
        var coroutine_status = null;
        var coroutine_token = null;

        export func start_coro() {
            coroutine = std.launch(coro);
        }

        export func get_coro() {
            return coroutine;
        }

        func coro() {
            coroutine_status = "before yield";
            coroutine_token = std.coroutine_token();
            std.yield_coroutine();
            coroutine_status = "after yield";
        }

        export func get_coro_status() {
            return coroutine_status;
        }

        export func get_coro_token() {
            return coroutine_token;
        }
    )";

    TestContext test(source);
    Context& ctx = test.ctx();

    // Retrieve coroutine
    test.call("start_coro").returns_null();
    auto coro_handle = test.call("get_coro").returns_value();
    REQUIRE(coro_handle->is<Coroutine>());
    auto coro = coro_handle.must_cast<Coroutine>();

    // Invoke coroutine until yield
    test.call("get_coro_status").returns_string("before yield");
    auto token_value = test.call("get_coro_token").returns_value();
    REQUIRE(token_value->is<CoroutineToken>());
    REQUIRE(coro->state() == CoroutineState::Waiting);

    // Resume the coroutine and test relevant state
    auto token = token_value.must_cast<CoroutineToken>();
    REQUIRE(!ctx.has_ready());
    REQUIRE(token->valid());                     // Valid before resume
    REQUIRE(CoroutineToken::resume(ctx, token)); // Resume is success because coroutine is waiting
    REQUIRE(!token->valid());                    // Invalid after resume
    REQUIRE(coro->state() == CoroutineState::Ready);
    REQUIRE(ctx.has_ready());

    // Run the coroutine again, it resumes after the last yield
    ctx.run_ready();
    test.call("get_coro_status").returns_string("after yield");
    REQUIRE(coro->state() == CoroutineState::Done);
}

TEST_CASE("Coroutines should support dispatching to each other", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test() {
            var token = std.coroutine_token();
            var pending = 0;
            const done = func() {
                pending -= 1;
                if pending == 0 {
                    token?.resume();
                    token = null;
                }
            };

            var output = [];
            for var i = 1; i <= 3; i += 1 {
                std.launch(task, i, output, done);
                pending += 1;
            }

            // coroutines are cold-start, i.e. they have not run yet in launch()
            output.append("start");
            std.yield_coroutine();
            output.append("end");
            return output;
        }

        func task(id, output, done) {
            output.append("${id}-1");
            std.dispatch();
            output.append("${id}-2");
            done();
        }
    )";

    TestContext test(source);
    auto result = test.call("test").returns_value();
    REQUIRE(result->is<Array>());

    const std::vector<std::string> expected = {
        "start",
        "1-1",
        "2-1",
        "3-1",
        "1-2",
        "2-2",
        "3-2",
        "end",
    };

    auto array = result.must_cast<Array>();
    REQUIRE(array->size() == expected.size());

    Scope sc(test.ctx());
    Local item = sc.local();

    size_t index = 0;
    for (const auto& str : expected) {
        CAPTURE(index, str);

        item = array->get(index);
        REQUIRE(item->is<String>());
        REQUIRE(item.must_cast<String>()->view() == str);

        ++index;
    }
}

} // namespace tiro::vm::test
