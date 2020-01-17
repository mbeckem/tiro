#include <catch.hpp>

#include "tiro/vm/context.hpp"
#include "tiro/vm/objects/strings.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Context should be able to intern strings", "[context]") {
    Context ctx;

    Root<String> s1(ctx), s2(ctx), s3(ctx);
    s1.set(String::make(ctx, "Hello World"));
    s2.set(String::make(ctx, "Hello World"));
    s3.set(String::make(ctx, "Foobar"));

    Root<String> c(ctx);

    c.set(ctx.intern_string(s1));
    REQUIRE(c->same(*s1));
    REQUIRE(c->interned());

    c.set(ctx.intern_string(s1));
    REQUIRE(c->same(*s1));

    c.set(ctx.intern_string(s2));
    REQUIRE(c->same(*s1));
    REQUIRE(s1->interned());
    REQUIRE(!s2->interned());

    c.set(ctx.intern_string(s3));
    REQUIRE(c->same(*s3));
}
