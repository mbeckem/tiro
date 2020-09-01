#include <catch2/catch.hpp>

#include "vm/handles/external.hpp"
#include "vm/objects/all.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("ExternalStorage should be empty by default", "[external]") {
    ExternalStorage storage;
    REQUIRE(storage.used_slots() == 0);
    REQUIRE(storage.free_slots() == 0);
    REQUIRE(storage.total_slots() == 0);
}

TEST_CASE("ExternalStorage should count allocated handles", "[external]") {
    ExternalStorage storage;

    External a = storage.allocate();
    External b = storage.allocate();
    REQUIRE(storage.used_slots() == 2);
    REQUIRE(storage.total_slots() >= 2);

    storage.free(a);
    storage.free(b);
    REQUIRE(storage.used_slots() == 0);
    REQUIRE(storage.free_slots() >= 2);
}

TEST_CASE("ExternalStorage should reuse freed handles", "[external]") {
    ExternalStorage storage;

    Value* old_slot = nullptr;
    {
        External a = storage.allocate();
        old_slot = get_valid_slot(a);
        storage.free(a);
    }

    {
        size_t old_free = storage.free_slots();
        REQUIRE(old_free >= 1);

        External b = storage.allocate();
        REQUIRE(storage.free_slots() == old_free - 1);

        // Test free list behaviour. Might fail if free list stuff gets more advanced.
        Value* new_slot = get_valid_slot(b);
        REQUIRE(new_slot == old_slot);
    }
}

TEST_CASE("ExternalStorage should support tracing", "[external]") {
    const size_t slot_count = 12345;

    ExternalStorage storage;

    std::unordered_set<Value*> slots;
    for (size_t i = 0; i < slot_count; ++i) {
        slots.insert(get_valid_slot(storage.allocate()));
    }
    REQUIRE(slots.size() == slot_count);

    std::unordered_set<Value*> traced;
    storage.trace([&](Value& slot) { traced.insert(&slot); });
    REQUIRE(traced.size() == slot_count);

    REQUIRE(slots == traced);
}

TEST_CASE("Allocation of external handles should succeed", "[external]") {
    ExternalStorage storage;

    External a = storage.allocate(Value::null());
    REQUIRE(a->is_null());

    External b = storage.allocate(SmallInteger::make(123));
    REQUIRE(b->is<SmallInteger>());
    REQUIRE(b.must_cast<SmallInteger>()->value() == 123);
}
