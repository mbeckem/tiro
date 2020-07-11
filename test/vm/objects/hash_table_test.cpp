#include <catch.hpp>

#include "vm/context.hpp"
#include "vm/objects/array.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/string.hpp"

#include "support/test_rng.hpp"

#include <iostream>

using namespace tiro;
using namespace tiro::vm;

static void fill_array(Context& ctx, const std::vector<std::string>& src, Handle<Array> dest) {
    Scope sc(ctx);
    Local str_obj = sc.local<String>(defer_init);

    for (const auto& str : src) {
        str_obj = String::make(ctx, str);
        dest->append(ctx, str_obj);
    }
}

TEST_CASE("Empty hash table should have well defined state", "[hash-table]") {
    Context ctx;
    Scope sc(ctx);

    Local table = sc.local(HashTable::make(ctx));
    REQUIRE(table->size() == 0);
    REQUIRE(table->empty());

    Value null = Value::null();
    REQUIRE(!table->contains(null));

    auto found = table->get(null);
    REQUIRE(!found);
}

TEST_CASE("Hash table should use size increments for capacity", "[hash-table]") {
    Context ctx;
    Scope sc(ctx);

    Local table = sc.local<HashTable>(defer_init);

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

TEST_CASE("Hash table should support initial capacity", "[hash-table]") {
    Context ctx;
    Scope sc(ctx);

    Local table = sc.local(HashTable::make(ctx, 33));
    REQUIRE(table->entry_capacity() >= 33);
    REQUIRE(table->index_capacity() == 64);
}

TEST_CASE("Hash table should support simple insertions and queries for integers", "[hash-table]") {
    Context ctx;
    Scope sc(ctx);

    Local table = sc.local(HashTable::make(ctx));
    for (int i = 0; i < 47; ++i) {
        Scope sc_inner(ctx);
        Local k = sc_inner.local(Integer::make(ctx, i));
        Local v = sc_inner.local(Value::null());

        table->set(ctx, k, v);
    }

    for (int i = 0; i < 47; ++i) {
        Scope sc_inner(ctx);
        Local k = sc_inner.local(Integer::make(ctx, i));

        auto found = table->get(*k);
        REQUIRE(found);
        REQUIRE(found->is_null());
    }
}

TEST_CASE("Hash table should support clearing", "[hash-table]") {
    Context ctx;
    Scope sc(ctx);
    Local table = sc.local(HashTable::make(ctx));

    for (int i = 0; i < 10; ++i) {
        Scope sc_inner(ctx);
        Local k = sc_inner.local(ctx.get_integer(i));
        Local v = sc_inner.local(Value::null());
        table->set(ctx, k, v);
    }
    REQUIRE(table->size() == 10);

    table->clear();
    REQUIRE(table->size() == 0);
    for (int i = 0; i < 10; ++i) {
        Scope sc_inner(ctx);
        Local k = sc_inner.local(ctx.get_integer(i));
        REQUIRE_FALSE(table->contains(*k));
    }

    for (int i = 0; i < 10; i += 3) {
        Scope sc_inner(ctx);
        Local k = sc_inner.local(ctx.get_integer(i));
        Local v = sc_inner.local(Value::null());
        table->set(ctx, k, v);
    }
    REQUIRE(table->size() == 4);
}

TEST_CASE("Hash table should support string keys", "[hash-table]") {
    std::vector<std::string> vec_in_table{"1", "foo", "129391", "-1", "Hello World", "1.2.3.4.5.6",
        "f(x, y, z)", "fizz", "buzz", "fizzbuzz"};
    std::vector<std::string> vec_not_in_table{"the", "quick", "brown", "fox"};

    Context ctx;
    Scope sc(ctx);

    Local in_table = sc.local(Array::make(ctx, 0));
    Local not_in_table = sc.local(Array::make(ctx, 0));
    Local one = sc.local(Integer::make(ctx, 1));

    fill_array(ctx, vec_in_table, in_table);
    fill_array(ctx, vec_not_in_table, not_in_table);

    Local table = sc.local(HashTable::make(ctx));
    {
        Local key_temp = sc.local();
        Local value_temp = sc.local();
        for (size_t i = 0; i < in_table->size(); ++i) {
            REQUIRE(table->size() == i);
            REQUIRE_FALSE(table->contains(in_table->get(i)));

            key_temp.set(in_table->get(i));
            table->set(ctx, key_temp, one);
            REQUIRE(table->size() == i + 1);

            auto found = table->get(*key_temp);
            REQUIRE(found);

            value_temp = *found;
            REQUIRE(equal(*value_temp, *one));
        }
    }
    REQUIRE(table->size() == in_table->size());
    REQUIRE(!table->empty());

    for (size_t i = 0; i < not_in_table->size(); ++i) {
        auto found = table->get(not_in_table->get(i));
        REQUIRE(!found);
    }
}

TEST_CASE(
    "Hash table find should return the same objects that were inserted "
    "previously",
    "[hash]") {
    Context ctx;
    Scope sc(ctx);

    Local table = sc.local(HashTable::make(ctx));

    Local k1 = sc.local(Integer::make(ctx, 1));
    Local k2 = sc.local(Integer::make(ctx, 2));
    Local k3 = sc.local(Integer::make(ctx, 1));
    Local k4 = sc.local(Integer::make(ctx, -1));
    Local v = sc.local(String::make(ctx, "Hello"));

    REQUIRE(!equal(*k1, *k2));
    REQUIRE(equal(*k1, *k3));
    REQUIRE(!k1->same(*k3));

    table->set(ctx, k1, v);
    table->set(ctx, k2, k1);

    REQUIRE(table->contains(*k1));
    REQUIRE(table->contains(*k2));
    REQUIRE(table->contains(*k3));

    // Lookup with k3 must return existing key k1 (because we used it to insert).
    {
        Local ex_k1 = sc.local();
        Local ex_v = sc.local();
        bool found = table->find(k3, ex_k1.mut(), ex_v.mut());
        REQUIRE(found);

        REQUIRE(ex_k1->same(*k1));
        REQUIRE(ex_v->same(*v));
    }

    // Lookup of non-existent key fails.
    {
        Local ex_k = sc.local();
        Local ex_v = sc.local();
        bool found = table->find(k4, ex_k.mut(), ex_v.mut());
        REQUIRE(!found);
    }
}

TEST_CASE("Elements should be able to be removed from a hash table", "[hash-table]") {
    Context ctx;
    Scope sc(ctx);

    Local table = sc.local(HashTable::make(ctx));

    auto insert_pair = [&](i64 k, i64 v) {
        CAPTURE(k, v);

        Scope sc_inner(ctx);
        Local key = sc_inner.local(Integer::make(ctx, k));
        Local value = sc_inner.local(Integer::make(ctx, v));
        table->set(ctx, key, value);
        REQUIRE(table->contains(*key));

        auto found = table->get(*key);
        REQUIRE(found);
        REQUIRE(found->must_cast<Integer>().value() == v);
    };

    auto get_value = [&](i64 k) {
        CAPTURE(k);
        Scope sc_inner(ctx);
        Local key = sc_inner.local(Integer::make(ctx, k));
        REQUIRE(table->contains(*key));

        auto found = table->get(*key);
        REQUIRE(found);
        return found->must_cast<Integer>().value();
    };

    auto remove_key = [&](i64 k) {
        CAPTURE(k);
        Scope sc_inner(ctx);
        Local key = sc_inner.local(Integer::make(ctx, k));
        table->remove(key);
        REQUIRE(!table->contains(*key));
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

TEST_CASE("Hash table should be compacted after too many removals", "[hash-table]") {
    Context ctx;
    Scope sc_outer(ctx);

    Local table = sc_outer.local(HashTable::make(ctx));

    auto insert_pair = [&](i64 k, i64 v) {
        CAPTURE(k, v);

        Scope sc_inner(ctx);
        Local key = sc_inner.local(Integer::make(ctx, k));
        Local value = sc_inner.local(Integer::make(ctx, v));
        table->set(ctx, key, value);
        REQUIRE(table->contains(*key));

        auto found = table->get(*key);
        REQUIRE(found);
        REQUIRE(found->must_cast<Integer>().value() == v);
    };

    auto remove_key = [&](i64 k) {
        CAPTURE(k);
        Scope sc_inner(ctx);
        Local key = sc_inner.local(Integer::make(ctx, k));
        table->remove(key);
        REQUIRE(!table->contains(*key));
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

TEST_CASE("Hash table should maintain iteration order", "[hash-table]") {
    Context ctx;
    Scope sc_outer(ctx);

    std::vector<std::pair<i64, i64>> pairs{
        {3, 1}, {5, 2}, {8, 3}, {13, 4}, {21, 5}, {34, 6}, {55, 6}};

    Local table = sc_outer.local(HashTable::make(ctx));

    auto insert_pair = [&](i64 k, i64 v) {
        CAPTURE(k, v);

        Scope sc_inner(ctx);
        Local key = sc_inner.local(Integer::make(ctx, k));
        Local value = sc_inner.local(Integer::make(ctx, v));
        table->set(ctx, key, value);
        REQUIRE(table->contains(*key));

        auto found = table->get(*key);
        REQUIRE(found);
        REQUIRE(found->must_cast<Integer>().value() == v);

        pairs.push_back({k, v});
    };

    auto remove_key = [&](i64 k) {
        CAPTURE(k);

        Scope sc_inner(ctx);
        Local key = sc_inner.local(Integer::make(ctx, k));
        table->remove(key);
        REQUIRE(!table->contains(*key));

        auto pair_pos = std::find_if(
            pairs.begin(), pairs.end(), [&](auto& p) { return p.first == k; });
        REQUIRE(pair_pos != pairs.end());
        pairs.erase(pair_pos);
    };

    {
        Scope sc_inner(ctx);
        Local key = sc_inner.local();
        Local value = sc_inner.local();
        for (const auto& pair : pairs) {
            key = Integer::make(ctx, pair.first);
            value = Integer::make(ctx, pair.second);
            table->set(ctx, key, value);
        }
    }

    auto check_order = [&]() {
        Scope sc_inner(ctx);
        Local key = sc_inner.local();
        Local value = sc_inner.local();
        Local current_entry = sc_inner.local();
        Local iterator = sc_inner.local(table->make_iterator(ctx));

        size_t index = 0;
        while (1) {
            current_entry = iterator->next(ctx);
            if (current_entry->same(ctx.get_stop_iteration())) {
                break;
            }

            REQUIRE(index < pairs.size());

            Handle<Tuple> pair = current_entry.must_cast<Tuple>();
            REQUIRE(pair->size() == 2);

            key.set(pair->get(0));
            value.set(pair->get(1));

            REQUIRE(key->is<Integer>());
            REQUIRE(value->is<Integer>());

            REQUIRE(key->must_cast<Integer>().value() == pairs[index].first);
            REQUIRE(value->must_cast<Integer>().value() == pairs[index].second);
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

TEST_CASE("Hash table should support a large number of insertions", "[hash-table]") {
    Context ctx;
    Scope sc(ctx);

    TestRng rng(123456);

    Local keys = sc.local(Array::make(ctx, 0));
    Local values = sc.local(Array::make(ctx, 0));

    const size_t entries = 12345;
    {
        Local key = sc.local();
        Local value = sc.local();

        for (size_t i = 0; i < entries; ++i) {
            std::string k = fmt::format("KEY_{}_{}", i, rng.next_i32());

            key = String::make(ctx, k);
            value = Integer::make(ctx, rng.next_i32());

            keys->append(ctx, key);
            values->append(ctx, value);
        }
    }

    Local table = sc.local(HashTable::make(ctx));
    {
        Local key = sc.local();
        Local value = sc.local();
        for (size_t i = 0; i < entries; ++i) {
            key = keys->get(i);
            value = values->get(i);
            table->set(ctx, key, value);
        }
    }
    REQUIRE(table->size() == entries);

    {
        Local key = sc.local();
        Local value = sc.local();
        Local found_value = sc.local();

        // Forward lookup
        for (size_t i = 0; i < entries; ++i) {
            key.set(keys->get(i));
            value.set(values->get(i));

            auto found = table->get(*key);
            if (!found)
                FAIL("Failed to find value.");
            found_value.set(*found);

            if (!equal(*value, *found_value)) {
                CAPTURE(to_string(*key));
                CAPTURE(to_string(*value));
                CAPTURE(to_string(*found_value));
                FAIL("Unexpected value");
            }
        }

        // Backward lookup
        for (size_t i = entries; i-- > 0;) {
            key.set(keys->get(i));
            value.set(values->get(i));

            auto found = table->get(*key);
            if (!found)
                FAIL("Failed to find value.");
            found_value.set(*found);

            if (!equal(*value, *found_value)) {
                CAPTURE(to_string(*key));
                CAPTURE(to_string(*value));
                CAPTURE(to_string(*found_value));
                FAIL("Unexpected value");
            }
        }
    }
}
