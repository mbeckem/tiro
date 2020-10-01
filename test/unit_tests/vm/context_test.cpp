#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/handles/scope.hpp"

using namespace tiro::vm;

TEST_CASE("Context supports userdata", "[context]") {
    int foo = 1;

    Context ctx;
    REQUIRE(ctx.userdata() == nullptr);

    ctx.userdata(&foo);
    REQUIRE(ctx.userdata() == &foo);

    ctx.userdata(nullptr);
    REQUIRE(ctx.userdata() == nullptr);
}
