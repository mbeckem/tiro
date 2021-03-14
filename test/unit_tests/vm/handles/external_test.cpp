#include <catch2/catch.hpp>

#include "vm/handles/external.hpp"
#include "vm/objects/all.hpp"

namespace tiro::vm::test {

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

TEST_CASE("UniqueExternal should free externals on destruction", "[external]") {
    ExternalStorage storage;

    {
        UniqueExternal ext(storage, storage.allocate(Value::null()));
        REQUIRE(storage.used_slots() == 1);
        REQUIRE(ext);
        REQUIRE(ext.valid());
        REQUIRE(ext->is<Null>());
    }
    REQUIRE(storage.used_slots() == 0);
}

TEST_CASE("UniqueExternal should be invalid by default", "[external]") {
    ExternalStorage storage;
    UniqueExternal<Value> ext(storage);
    REQUIRE_FALSE(ext);
    REQUIRE_FALSE(ext.valid());
}

TEST_CASE("Moving UniqueExternals should transfer ownership", "[external]") {
    ExternalStorage storage;
    {
        UniqueExternal a(storage, storage.allocate(SmallInteger::make(123)));
        REQUIRE(a.valid());
        REQUIRE(a->value() == 123);

        UniqueExternal b = std::move(a);
        REQUIRE_FALSE(a.valid());
        REQUIRE(b.valid());
        REQUIRE(b->value() == 123);
    }
    REQUIRE(storage.used_slots() == 0);
}

TEST_CASE("Releasing a UniqueExternal should make it invalid", "[external]") {
    ExternalStorage storage;
    {
        UniqueExternal ext(storage, storage.allocate(Value::null()));
        REQUIRE(storage.used_slots() == 1);

        External released = ext.release();
        REQUIRE_FALSE(ext.valid());
        REQUIRE(released->is_null());
        REQUIRE(storage.used_slots() == 1);

        storage.free(released);
    }
}

} // namespace tiro::vm::test
