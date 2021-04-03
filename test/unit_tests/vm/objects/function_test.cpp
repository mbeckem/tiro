#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/handles/scope.hpp"
#include "vm/objects/function.hpp"

namespace tiro::vm::test {

TEST_CASE("Handler tables should return the correct entry", "[function]") {
    std::vector<HandlerTable::Entry> entries{
        {1, 3, 10000},
        {3, 3, 20000},
        {3, 5, 30000},
        {10, 20, 40000},
    };

    Context ctx;
    Scope sc(ctx);
    Local handlers = sc.local(HandlerTable::make(ctx, entries));
    {
        auto raw_entries = handlers->view();
        REQUIRE(std::equal(entries.begin(), entries.end(), raw_entries.begin(), raw_entries.end()));
    }

    auto lookup = [&](u32 pc, const HandlerTable::Entry& expected) {
        CAPTURE(pc);
        auto found = handlers->find_entry(pc);
        CHECK(found);
        if (!found)
            return;

        CAPTURE(expected.from, expected.to, expected.target);
        CAPTURE(found->from, found->to, found->target);
        CHECK(*found == expected);
    };

    auto lookup_fail = [&](u32 pc) {
        CAPTURE(pc);
        auto found = handlers->find_entry(pc);
        CHECK(!found);
    };

    lookup_fail(0);
    lookup(1, entries[0]);
    lookup(2, entries[0]);

    lookup(3, entries[2]);
    lookup_fail(5);

    lookup_fail(9);
    lookup(10, entries[3]);
    lookup(15, entries[3]);
    lookup(19, entries[3]);
    lookup_fail(20);

    lookup_fail(9999);
}

} // namespace tiro::vm::test
