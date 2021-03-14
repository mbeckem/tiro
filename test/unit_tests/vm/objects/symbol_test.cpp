#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/handles/scope.hpp"
#include "vm/objects/primitives.hpp"

namespace tiro::vm::test {

TEST_CASE("Explicitly allocated symbols are not the same", "[symbols]") {
    Context ctx;
    Scope sc(ctx);

    Local str = sc.local(String::make(ctx, "foo"));
    Local s1 = sc.local(Symbol::make(ctx, str));
    Local s2 = sc.local(Symbol::make(ctx, str));

    REQUIRE_FALSE(s1->same(*s2));
    REQUIRE(s1->name().same(*str));
    REQUIRE(s2->name().same(*str));
}

TEST_CASE("Symbols created from the context are the same for string views", "[symbols]") {
    Context ctx;
    Scope sc(ctx);

    std::string name1 = "foo";
    std::string name2 = "foo";

    Local s1 = sc.local(ctx.get_symbol(name1));
    Local s2 = sc.local(ctx.get_symbol(name2));
    REQUIRE(s1->same(*s2));
    REQUIRE(s1->name().view() == "foo");
}

TEST_CASE(
    "Symbols created from the context are the same for different string instances with same "
    "content",
    "[symbols]") {
    Context ctx;
    Scope sc(ctx);

    Local name1 = sc.local(String::make(ctx, "foo"));
    Local name2 = sc.local(String::make(ctx, "foo"));
    REQUIRE_FALSE(name1->same(*name2));

    Local s1 = sc.local(ctx.get_symbol(name1));
    Local s2 = sc.local(ctx.get_symbol(name2));
    REQUIRE(s1->same(*s2));
    REQUIRE(s1->name().view() == "foo");
}

} // namespace tiro::vm::test
