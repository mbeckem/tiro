#include <catch.hpp>

#include "vm/context.hpp"
#include "vm/objects/array.hpp"
#include "vm/objects/class.hpp"
#include "vm/objects/string.hpp"

#include <string>
#include <unordered_set>

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Dynamic objects should support dynamic properties", "[classes]") {
    Context ctx;
    Scope sc(ctx);

    Local obj = sc.local(DynamicObject::make(ctx));
    Local propA = sc.local(ctx.get_symbol("A"));
    Local propB = sc.local(ctx.get_symbol("B"));
    Local value = sc.local(Integer::make(ctx, 123));

    // Non-existent properties are null
    REQUIRE(obj->get(propA).is_null());

    // Values can be retrieved
    obj->set(ctx, propA, value);
    {
        Value found = obj->get(propA);
        REQUIRE(found.is<Integer>());
        REQUIRE(found.must_cast<Integer>().value() == 123);
    }

    obj->set(ctx, propB, value);

    // Names can be retrieved
    Local names = sc.local(obj->names(ctx));
    Local sym = sc.local();
    Local name = sc.local<String>(defer_init);
    REQUIRE(names->size() == 2);
    std::unordered_set<std::string> seen;
    for (size_t i = 0; i < names->size(); ++i) {
        sym = names->get(i);
        REQUIRE(sym->is<Symbol>());

        name = sym->must_cast<Symbol>().name();
        seen.insert(std::string(name->view()));
    }

    REQUIRE(seen == std::unordered_set<std::string>{"A", "B"});
}
