#include <catch.hpp>

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/array.hpp"
#include "hammer/vm/objects/hash_table.hpp"
#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/objects/string.hpp"

#include "../test_rng.hpp"

#include <iostream>

using namespace hammer;
using namespace hammer::vm;

static void fill_array(
    Context& ctx, const std::vector<std::string>& src, Handle<Array> dest) {
    Root<String> root(ctx);

    for (const auto& str : src) {
        root.set(String::make(ctx, str));
        dest->append(ctx, root.handle());
    }
}

TEST_CASE("empty hash table", "[hash-table]") {
    Context ctx;

    Root<HashTable> table(ctx, HashTable::make(ctx));
    REQUIRE(table->size() == 0);
    REQUIRE(table->empty());

    Value null = Value::null();
    REQUIRE(!table->contains(null));
    REQUIRE(table->get(null)->same(null));
}

TEST_CASE("hash table capacities", "[hash-table]") {
    Context ctx;

    Root<HashTable> table(ctx);

    auto init = [&](size_t size) { table.set(HashTable::make(ctx, size)); };

    init(0);
    REQUIRE(table->entry_capacity() == 0);
    REQUIRE(table->index_capacity() == 0);

    init(1);
    REQUIRE(table->entry_capacity() == 6);
    REQUIRE(table->index_capacity() == 8);

    init(6);
    REQUIRE(table->entry_capacity() == 6);
    REQUIRE(table->index_capacity() == 8);

    init(7);
    REQUIRE(table->entry_capacity() == 12);
    REQUIRE(table->index_capacity() == 16);

    init(99);
    REQUIRE(table->entry_capacity() == 192);
    REQUIRE(table->index_capacity() == 256);

    init(192);
    REQUIRE(table->entry_capacity() == 192);
    REQUIRE(table->index_capacity() == 256);

    init(193);
    REQUIRE(table->entry_capacity() == 384);
    REQUIRE(table->index_capacity() == 512);
}

TEST_CASE("hash table with initial capacity", "[hash-table]") {
    Context ctx;

    Root<HashTable> table(ctx, HashTable::make(ctx, 33));
    REQUIRE(table->entry_capacity() >= 33);
    REQUIRE(table->index_capacity() == 64);
}

TEST_CASE("hash table of numbers", "[hash-table]") {
    Context ctx;

    Root table(ctx, HashTable::make(ctx));
    for (int i = 0; i < 47; ++i) {
        Root k(ctx, Integer::make(ctx, i));
        Root v(ctx, Value::null());

        table->set(ctx, k.handle(), v.handle());
    }

    for (int i = 0; i < 47; ++i) {
        Root k(ctx, Integer::make(ctx, i));

        auto found = table->get(k.handle());
        REQUIRE(found);
        REQUIRE(found->is_null());
    }
}

TEST_CASE("basic hash table insertion", "[hash-table]") {
    std::vector<std::string> vec_in_table{"1", "foo", "129391", "-1",
        "Hello World", "1.2.3.4.5.6", "f(x, y, z)", "fizz", "buzz", "fizzbuzz"};
    std::vector<std::string> vec_not_in_table{"the", "quick", "brown", "fox"};

    Context ctx;

    Root<Array> in_table(ctx, Array::make(ctx, 0));
    Root<Array> not_in_table(ctx, Array::make(ctx, 0));
    Root<Integer> one(ctx, Integer::make(ctx, 1));

    fill_array(ctx, vec_in_table, in_table);
    fill_array(ctx, vec_not_in_table, not_in_table);

    Root<HashTable> table(ctx, HashTable::make(ctx));
    {
        Root<Value> key_temp(ctx);
        Root<Value> val_temp(ctx);
        for (size_t i = 0; i < in_table->size(); ++i) {
            REQUIRE(table->size() == i);
            REQUIRE_FALSE(table->contains(in_table->get(i)));

            key_temp.set(in_table->get(i));
            table->set(ctx, key_temp.handle(), one.handle());
            REQUIRE(table->size() == i + 1);

            auto found = table->get(key_temp.handle());
            REQUIRE(found);

            val_temp.set(*found);
            REQUIRE(equal(val_temp.get(), one.get()));
        }
    }
    REQUIRE(table->size() == in_table->size());
    REQUIRE(!table->empty());

    for (size_t i = 0; i < not_in_table->size(); ++i) {
        auto found = table->get(not_in_table->get(i));
        REQUIRE(!found);
    }
}

TEST_CASE("hash table find returns same objects", "[hash]") {
    Context ctx;

    Root<HashTable> table(ctx, HashTable::make(ctx));

    Root k1(ctx, Integer::make(ctx, 1));
    Root k2(ctx, Integer::make(ctx, 2));
    Root k3(ctx, Integer::make(ctx, 1));
    Root k4(ctx, Integer::make(ctx, -1));
    Root v(ctx, String::make(ctx, "Hello"));

    REQUIRE(!equal(*k1, *k2));
    REQUIRE(equal(*k1, *k3));
    REQUIRE(!k1->same(*k3));

    table->set(ctx, k1.handle(), v.handle());
    table->set(ctx, k2.handle(), k1.handle());

    REQUIRE(table->contains(k1.handle()));
    REQUIRE(table->contains(k2.handle()));
    REQUIRE(table->contains(k3.handle()));

    // Lookup with k3 must return existing key k1 (because we used it to insert).
    {
        Root<Value> ex_k1(ctx);
        Root<Value> ex_v(ctx);
        bool found = table->find(
            k3.handle(), ex_k1.mut_handle(), ex_v.mut_handle());
        REQUIRE(found);

        REQUIRE(ex_k1->same(*k1));
        REQUIRE(ex_v->same(*v));
    }

    // Lookup of non-existent key fails.
    {
        Root<Value> ex_k(ctx);
        Root<Value> ex_v(ctx);
        bool found = table->find(
            k4.handle(), ex_k.mut_handle(), ex_v.mut_handle());
        REQUIRE(!found);
    }
}

TEST_CASE("remove from hash table", "[hash-table]") {
    Context ctx;

    Root<HashTable> table(ctx, HashTable::make(ctx));

    auto insert_pair = [&](i64 k, i64 v) {
        CAPTURE(k, v);
        Root<Integer> key(ctx, Integer::make(ctx, k));
        Root<Integer> value(ctx, Integer::make(ctx, v));
        table->set(ctx, key.handle(), value.handle());
        REQUIRE(table->contains(key.get()));

        auto found = table->get(key.get());
        REQUIRE(found);
        REQUIRE(found->as_strict<Integer>().value() == v);
    };

    auto get_value = [&](i64 k) {
        CAPTURE(k);
        Root<Integer> key(ctx, Integer::make(ctx, k));
        REQUIRE(table->contains(key.get()));

        auto found = table->get(key.get());
        REQUIRE(found);
        return found->as_strict<Integer>().value();
    };

    auto remove_key = [&](i64 k) {
        CAPTURE(k);
        Root<Integer> key(ctx, Integer::make(ctx, k));
        table->remove(ctx, key.handle());
        REQUIRE(!table->contains(key.get()));
    };

    insert_pair(1, 2);
    insert_pair(2, 3);
    insert_pair(3, 4);
    insert_pair(4, 5);
    insert_pair(5, 6);
    insert_pair(6, 7);
    insert_pair(7, 8);
    insert_pair(8, 9);
    insert_pair(9, 10);

    remove_key(1);
    remove_key(3);
    remove_key(9);
    remove_key(4);

    REQUIRE(table->size() == 5);
    REQUIRE(get_value(2) == 3);
    REQUIRE(get_value(5) == 6);
    REQUIRE(get_value(6) == 7);
    REQUIRE(get_value(7) == 8);
    REQUIRE(get_value(8) == 9);

    remove_key(-1);
    remove_key(99);

    REQUIRE(table->size() == 5);
    REQUIRE(get_value(2) == 3);
    REQUIRE(get_value(5) == 6);
    REQUIRE(get_value(6) == 7);
    REQUIRE(get_value(7) == 8);
    REQUIRE(get_value(8) == 9);

    remove_key(5);
    remove_key(6);
    remove_key(8);
    remove_key(7);
    remove_key(2);
    REQUIRE(table->size() == 0);
}

TEST_CASE("hash table gets compacted after too many removals", "[hash-table]") {
    Context ctx;

    Root<HashTable> table(ctx, HashTable::make(ctx));

    auto insert_pair = [&](i64 k, i64 v) {
        CAPTURE(k, v);
        Root<Integer> key(ctx, Integer::make(ctx, k));
        Root<Integer> value(ctx, Integer::make(ctx, v));
        table->set(ctx, key.handle(), value.handle());
        REQUIRE(table->contains(key.get()));

        auto found = table->get(key.get());
        REQUIRE(found);
        REQUIRE(found->as_strict<Integer>().value() == v);
    };

    auto remove_key = [&](i64 k) {
        CAPTURE(k);
        Root<Integer> key(ctx, Integer::make(ctx, k));
        table->remove(ctx, key.handle());
        REQUIRE(!table->contains(key.get()));
    };

    insert_pair(1, 2);
    insert_pair(3, 4);
    insert_pair(5, 6);
    insert_pair(7, 8);
    insert_pair(9, 10);
    insert_pair(11, 12);
    insert_pair(13, 14);
    REQUIRE(table->size() == 7);
    REQUIRE(table->entry_capacity() == 12);
    REQUIRE(table->is_packed());

    // Delete last key
    remove_key(13);
    REQUIRE(table->is_packed());

    // Remove in the middle
    remove_key(5);
    REQUIRE(!table->is_packed());
    remove_key(3);
    REQUIRE(!table->is_packed());

    // Size / ValueCount <= 50% -> Compact
    remove_key(9);
    REQUIRE(table->size() == 3);
    REQUIRE(table->is_packed());
}

TEST_CASE("hash table maintains iteration order", "[hash-table]") {
    Context ctx;

    std::vector<std::pair<i64, i64>> pairs{
        {3, 1}, {5, 2}, {8, 3}, {13, 4}, {21, 5}, {34, 6}, {55, 6}};

    Root<HashTable> table(ctx, HashTable::make(ctx));

    auto insert_pair = [&](i64 k, i64 v) {
        CAPTURE(k, v);
        Root<Integer> key(ctx, Integer::make(ctx, k));
        Root<Integer> value(ctx, Integer::make(ctx, v));
        table->set(ctx, key.handle(), value.handle());
        REQUIRE(table->contains(key.get()));

        auto found = table->get(key.get());
        REQUIRE(found);
        REQUIRE(found->as_strict<Integer>().value() == v);

        pairs.push_back({k, v});
    };

    auto remove_key = [&](i64 k) {
        CAPTURE(k);
        Root<Integer> key(ctx, Integer::make(ctx, k));
        table->remove(ctx, key.handle());
        REQUIRE(!table->contains(key.get()));

        auto pair_pos = std::find_if(
            pairs.begin(), pairs.end(), [&](auto& p) { return p.first == k; });
        REQUIRE(pair_pos != pairs.end());
        pairs.erase(pair_pos);
    };

    {
        Root<Integer> key(ctx);
        Root<Integer> value(ctx);
        for (const auto& pair : pairs) {
            key.set(Integer::make(ctx, pair.first));
            value.set(Integer::make(ctx, pair.second));
            table->set(ctx, key.handle(), value.handle());
        }
    }

    auto check_order = [&]() {
        Root<Value> key(ctx);
        Root<Value> value(ctx);
        Root<Value> current_entry(ctx);
        Root<HashTableIterator> iterator(ctx, table->make_iterator(ctx));

        size_t index = 0;
        while (1) {
            current_entry.set(iterator->next(ctx));
            if (current_entry.get().same(ctx.get_stop_iteration())) {
                break;
            }

            REQUIRE(index < pairs.size());

            Handle<Tuple> pair = current_entry.handle().strict_cast<Tuple>();
            REQUIRE(pair->size() == 2);

            key.set(pair->get(0));
            value.set(pair->get(1));

            REQUIRE(key->is<Integer>());
            REQUIRE(value->is<Integer>());

            REQUIRE(key->as<Integer>().value() == pairs[index].first);
            REQUIRE(value->as<Integer>().value() == pairs[index].second);
            ++index;
        }
    };

    check_order();

    remove_key(8);
    remove_key(34);
    check_order();

    insert_pair(8, 99);
    check_order();
}

TEST_CASE("insert a large number of values into the table", "[hash-table]") {
    Context ctx;

    TestRng rng(123456);

    Root<Array> keys(ctx, Array::make(ctx, 0));
    Root<Array> values(ctx, Array::make(ctx, 0));

    const size_t entries = 12345;
    {
        Root<String> key(ctx);
        Root<Integer> value(ctx);

        for (size_t i = 0; i < entries; ++i) {
            std::string k = fmt::format("KEY_{}_{}", i, rng.next_i32());

            key.set(String::make(ctx, k));
            value.set(Integer::make(ctx, rng.next_i32()));

            keys->append(ctx, key.handle());
            values->append(ctx, value.handle());
        }
    }

    Root<HashTable> table(ctx, HashTable::make(ctx));
    {
        Root<Value> key(ctx);
        Root<Value> value(ctx);

        for (size_t i = 0; i < entries; ++i) {
            key.set(keys->get(i));
            value.set(values->get(i));
            table->set(ctx, key.handle(), value.handle());
        }
    }
    REQUIRE(table->size() == entries);

    {
        Root<Value> key(ctx);
        Root<Value> value(ctx);
        Root<Value> found_value(ctx);

        // Forward lookup
        for (size_t i = 0; i < entries; ++i) {
            key.set(keys->get(i));
            value.set(values->get(i));

            auto found = table->get(key.get());
            if (!found)
                FAIL("Failed to find value.");
            found_value.set(*found);

            if (!equal(value.get(), found_value.get())) {
                CAPTURE(to_string(key.get()));
                CAPTURE(to_string(value.get()));
                CAPTURE(to_string(found_value.get()));
                FAIL("Unexpected value");
            }
        }

        // Backward lookup
        for (size_t i = entries; i-- > 0;) {
            key.set(keys->get(i));
            value.set(values->get(i));

            auto found = table->get(key.get());
            if (!found)
                FAIL("Failed to find value.");
            found_value.set(*found);

            if (!equal(value.get(), found_value.get())) {
                CAPTURE(to_string(key.get()));
                CAPTURE(to_string(value.get()));
                CAPTURE(to_string(found_value.get()));
                FAIL("Unexpected value");
            }
        }
    }
}
