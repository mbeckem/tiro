#include <catch.hpp>

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/array.hpp"
#include "hammer/vm/objects/classes.hpp"
#include "hammer/vm/objects/string.hpp"

#include <string>
#include <unordered_set>

using namespace hammer;
using namespace hammer::vm;

TEST_CASE("Dynamic objects", "[classes]") {
    Context ctx;

    Root obj(ctx, DynamicObject::make(ctx));
    Root propA(ctx, ctx.get_symbol("A"));
    Root propB(ctx, ctx.get_symbol("B"));
    Root value(ctx, Integer::make(ctx, 123));

    // Non-existent properties are null
    REQUIRE(obj->get(propA).is_null());

    // Values can be retrieved
    obj->set(ctx, propA, value.handle());
    {
        Value found = obj->get(propA);
        REQUIRE(found.is<Integer>());
        REQUIRE(found.as<Integer>().value() == 123);
    }

    obj->set(ctx, propB, value.handle());

    // Names can be retrieved
    Root names(ctx, obj->properties(ctx));
    REQUIRE(names->size() == 2);
    std::unordered_set<std::string> seen;
    for (size_t i = 0; i < names->size(); ++i) {
        Root sym(ctx, names->get(i));
        REQUIRE(sym->is<Symbol>());

        Root name(ctx, sym->as<Symbol>().name());
        seen.insert(std::string(name->view()));
    }

    REQUIRE(seen == std::unordered_set<std::string>{"A", "B"});
}
