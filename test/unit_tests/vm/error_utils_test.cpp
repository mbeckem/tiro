#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/error_utils.hpp"
#include "vm/objects/exception.hpp"

namespace tiro::vm::test {

namespace {

struct DummyFrame {
    bool panicked = false;
    void panic(...) { panicked = true; }
};

}; // namespace

static Fallible<int> may_fail(Context& ctx, bool fail) {
    if (fail) {
        return TIRO_FORMAT_EXCEPTION(ctx, "Nope!");
    }
    return 2;
}

static Fallible<void> may_fail_void(Context& ctx, bool fail) {
    if (fail) {
        return TIRO_FORMAT_EXCEPTION(ctx, "Nope!");
    }
    return {};
}

TEST_CASE("TIRO_TRY should return the expected result", "[error_utils]") {
    Context ctx;

    auto test = [&](bool fail) -> Fallible<int> {
        TIRO_TRY(result, may_fail(ctx, fail));
        return result * 2;
    };

    SECTION("when an exception is thrown") {
        auto result = test(true);
        REQUIRE(result.has_exception());
    }

    SECTION("when no exception is thrown") {
        auto result = test(false);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 4);
    }
}

TEST_CASE("TIRO_TRY_VOID should return the expected result", "[error_utils]") {
    Context ctx;

    auto test = [&](bool fail) -> Fallible<int> {
        TIRO_TRY_VOID(may_fail_void(ctx, fail));
        return 1;
    };

    SECTION("when an exception is thrown") {
        auto result = test(true);
        REQUIRE(result.has_exception());
    }

    SECTION("when no exception is thrown") {
        auto result = test(false);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1);
    }
}

TEST_CASE("TIRO_FRAME_TRY should panic on errors", "[error_utils]") {
    Context ctx;

    auto test = [&](DummyFrame& frame, bool fail, int& output) -> void {
        TIRO_FRAME_TRY(result, may_fail(ctx, fail));
        output = result * 2;
    };

    SECTION("when an exception is thrown") {
        DummyFrame frame;
        int output = 0;
        test(frame, true, output);
        REQUIRE(frame.panicked);
        REQUIRE(output == 0);
    }

    SECTION("when no exception is thrown") {
        DummyFrame frame;
        int output = 0;
        test(frame, false, output);
        REQUIRE_FALSE(frame.panicked);
        REQUIRE(output == 4);
    }
}

TEST_CASE("TIRO_FRAME_TRY_VOID should panic on errors", "[error_utils]") {
    Context ctx;

    auto test = [&](DummyFrame& frame, bool fail, int& output) -> void {
        TIRO_FRAME_TRY_VOID(may_fail_void(ctx, fail));
        output = 123;
    };

    SECTION("when an exception is thrown") {
        DummyFrame frame;
        int output = 0;
        test(frame, true, output);
        REQUIRE(frame.panicked);
        REQUIRE(output == 0);
    }

    SECTION("when no exception is thrown") {
        DummyFrame frame;
        int output = 0;
        test(frame, false, output);
        REQUIRE_FALSE(frame.panicked);
        REQUIRE(output == 123);
    }
}

} // namespace tiro::vm::test