#include <catch2/catch.hpp>

#include "common/scope.hpp"
#include "vm/context.hpp"
#include "vm/handles/scope.hpp"
#include "vm/objects/primitives.hpp"

#include <algorithm>

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("RootedStack should support be empty by default", "[scope]") {
    RootedStack stack;
    REQUIRE(stack.pages() == 0);
    REQUIRE(stack.used_slots() == 0);
    REQUIRE(stack.total_slots() == 0);
}

TEST_CASE("RootedStack should support allocation", "[scope]") {
    RootedStack stack;

    stack.allocate();
    stack.allocate();
    stack.allocate();
    REQUIRE(stack.pages() == 1);
    REQUIRE(stack.used_slots() == 3);
    REQUIRE(stack.total_slots() == RootedStack::slots_per_page);
}

TEST_CASE("RootedStack should support allocation of multiple slots", "[scope]") {
    RootedStack stack;

    auto slots_1 = stack.allocate_slots(17);
    REQUIRE(slots_1.data() != nullptr);
    REQUIRE(slots_1.size() == 17);

    auto slots_2 = stack.allocate_slots(RootedStack::max_slots_per_alloc);
    REQUIRE(slots_2.data() != nullptr);
    REQUIRE(slots_2.size() == RootedStack::max_slots_per_alloc);

    REQUIRE(stack.used_slots() == RootedStack::max_slots_per_alloc + 17);

    stack.deallocate_slots(stack.used_slots());
    REQUIRE(stack.used_slots() == 0);
}

TEST_CASE("RootedStack should throw when allocating too many slots", "[scope]") {
    RootedStack stack;
    REQUIRE_THROWS_AS(stack.allocate_slots(RootedStack::max_slots_per_alloc + 1), std::bad_alloc);
}

TEST_CASE("RootedStack should support tracing", "[scope]") {
    const size_t slot_count = (RootedStack::slots_per_page * 5) / 2;

    RootedStack stack;

    std::vector<Value*> slots;
    for (size_t i = 0; i < slot_count; ++i) {
        slots.push_back(stack.allocate());
    }

    std::vector<Value*> traced;
    stack.trace([&](Span<Value> span) {
        for (Value& v : span) {
            traced.push_back(&v);
        }
    });

    REQUIRE(std::equal(slots.begin(), slots.end(), traced.begin(), traced.end()));
}

TEST_CASE("RootedStack should remain consistent when deallocating slots", "[scope]") {
    RootedStack stack;
    size_t expected_slots = (RootedStack::slots_per_page * 5) / 2;

    // Allocate multiple pages
    for (size_t i = 0; i < expected_slots; ++i)
        stack.allocate();

    REQUIRE(stack.pages() == 3);
    REQUIRE(stack.total_slots() == stack.pages() * RootedStack::slots_per_page);
    REQUIRE(stack.used_slots() == expected_slots);

    // Small deallocations that do not cross page boundary
    stack.deallocate_slots(1);
    expected_slots -= 1;
    REQUIRE(stack.used_slots() == expected_slots);

    stack.deallocate_slots(3);
    expected_slots -= 3;
    REQUIRE(stack.used_slots() == expected_slots);

    // Large deallocation into the the previous page
    stack.deallocate_slots(RootedStack::slots_per_page);
    expected_slots -= RootedStack::slots_per_page;
    REQUIRE(stack.pages() == 3); // pages are buffered
    REQUIRE(stack.total_slots() == stack.pages() * RootedStack::slots_per_page);
    REQUIRE(stack.used_slots() == expected_slots);

    // Allocation is still possible
    for (size_t i = 0; i < RootedStack::slots_per_page * 3; ++i)
        stack.allocate();

    expected_slots += RootedStack::slots_per_page * 3;
    REQUIRE(stack.pages() == 5);
    REQUIRE(stack.total_slots() == stack.pages() * RootedStack::slots_per_page);
    REQUIRE(stack.used_slots() == expected_slots);
}

TEST_CASE("RootedStack should revert to initial state", "[scope]") {
    RootedStack stack;

    size_t slot_count = RootedStack::slots_per_page * 2;
    for (size_t i = 0; i < slot_count; ++i)
        stack.allocate();

    stack.deallocate_slots(slot_count);
    REQUIRE(stack.used_slots() == 0);

    stack.allocate();
    REQUIRE(stack.used_slots() == 1);
}

TEST_CASE("Scope should allow construction of local variables", "[scope]") {
    Context ctx;

    Scope scope(ctx);
    Local l1 = scope.local();
    Local l2 = scope.local(SmallInteger::make(123));

    REQUIRE(l1->is_null());
    REQUIRE(l2.must_cast<SmallInteger>()->value() == 123);
}

TEST_CASE("Scopes should support nesting", "[scope]") {
    Context ctx;

    auto alloc_n = [&](Scope& sc, size_t n) {
        for (size_t i = 0; i < n; ++i)
            sc.local(Value::null());
    };

    size_t n1 = RootedStack::slots_per_page * 3 + 42;
    size_t n2 = 7;
    size_t n3 = (RootedStack::slots_per_page * 3) / 2;

    {
        Scope s1(ctx);
        alloc_n(s1, n1);
        {
            Scope s2(ctx);
            alloc_n(s2, n2);
            {
                Scope s3(ctx);
                alloc_n(s3, n3);
                REQUIRE(ctx.stack().used_slots() == n1 + n2 + n3);
            }
            REQUIRE(ctx.stack().used_slots() == n1 + n2);
        }
        REQUIRE(ctx.stack().used_slots() == n1);
    }
    REQUIRE(ctx.stack().used_slots() == 0);
}

TEST_CASE("Scopes should support allocation of local arrays", "[scope]") {
    Context ctx;

    {
        Scope s1(ctx);
        s1.array(13);
        s1.array(13);
        s1.array(13);

        {
            Scope s2(ctx);
            while (ctx.stack().used_slots() < RootedStack::slots_per_page - 3)
                s2.local(Value::null());

            {
                Scope s3(ctx);
                s3.array(17);
                REQUIRE(ctx.stack().pages() == 2);
                REQUIRE(ctx.stack().used_slots() == RootedStack::slots_per_page + 14);
            }

            REQUIRE(ctx.stack().used_slots() == RootedStack::slots_per_page - 3);
        }
        REQUIRE(ctx.stack().used_slots() == 39);
    }
    REQUIRE(ctx.stack().used_slots() == 0);
}

TEST_CASE("Local arrays should support iniitial values", "[scope]") {
    Context ctx;

    Scope sc(ctx);
    LocalArray array = sc.array(13, SmallInteger::make(123));

    REQUIRE(array.size() == 13);
    for (Handle<SmallInteger> i : array) {
        REQUIRE(i->value() == 123);
    }
}

TEST_CASE("Locals should provide reference style assign-through semantics", "[scope]") {
    Context ctx;

    Scope scope(ctx);
    Local l1 = scope.local(SmallInteger::make(1));
    Local l2 = scope.local(SmallInteger::make(2));
    Local l3 = scope.local(SmallInteger::make(3));

    l2 = l3;
    REQUIRE(l2->value() == 3);

    l2 = l1;
    REQUIRE(l2->value() == 1);

    l2 = SmallInteger::make(4);
    REQUIRE(l2->value() == 4);
    REQUIRE(l3->value() == 3);
    REQUIRE(l1->value() == 1);
}

TEST_CASE("Locals should have pointer size",
    "[scope]"
#ifdef _MSC_VER
    "[!shouldfail]" // EBO support on msvc?
#endif
) {
    REQUIRE(sizeof(Local<Value>) == sizeof(void*));
}
