#include <catch2/catch.hpp>

#include "common/adt/span.hpp"
#include "vm/context.hpp"
#include "vm/objects/array.hpp"
#include "vm/objects/record.hpp"

namespace tiro::vm::test {

static void check_keys(Context& ctx, Handle<Record> record, Span<const std::string_view> expected) {
    Scope sc(ctx);
    Local keys = sc.local(Record::keys(ctx, record));
    REQUIRE(keys->size() == expected.size());

    Local current = sc.local();
    size_t i = 0;
    for (const auto& name : expected) {
        CAPTURE(name);
        current = keys->get(i++);
        REQUIRE(current->is<Symbol>());
        REQUIRE(current.must_cast<Symbol>()->name().view() == name);
    }
}

TEST_CASE("Record templates should correctly store the configured keys", "[record]") {
    Context ctx;
    Scope sc(ctx);

    Local keys = sc.local(Array::make(ctx, 0));
    Local foo = sc.local(ctx.get_symbol("foo"));
    Local bar = sc.local(ctx.get_symbol("bar"));
    keys->append(ctx, foo);
    keys->append(ctx, bar);

    Local tmpl = sc.local(RecordTemplate::make(ctx, keys));
    REQUIRE(tmpl->size() == 2);

    Local actual_keys = sc.local(Array::make(ctx, 0));
    tmpl->for_each(ctx, [&](auto symbol) { actual_keys->append(ctx, symbol); });
    REQUIRE(actual_keys->size() == 2);
    REQUIRE(actual_keys->get(0).same(*foo));
    REQUIRE(actual_keys->get(1).same(*bar));
}

TEST_CASE("Record template construction fails for duplicate keys", "[record]") {
    Context ctx;
    Scope sc(ctx);

    Local keys = sc.local(Array::make(ctx, 0));
    Local foo = sc.local(ctx.get_symbol("foo"));
    keys->append(ctx, foo);
    keys->append(ctx, foo);

    REQUIRE_THROWS(sc.local(RecordTemplate::make(ctx, keys)));
}

TEST_CASE("Records should be constructible from an array of symbols", "[record]") {
    static constexpr std::string_view names[] = {"foo", "bar", "baz"};

    Context ctx;
    Scope sc(ctx);

    Local keys = sc.local(Array::make(ctx, 0));
    Local key = sc.local();
    for (const auto& name : names) {
        key = ctx.get_symbol(name);
        keys->append(ctx, key);
    }

    Local record = sc.local(Record::make(ctx, keys));
    check_keys(ctx, record, names);
}

TEST_CASE("Records should be constructible from a static set of handles", "[record]") {
    static constexpr std::string_view names[] = {"foo", "bar", "baz"};

    Context ctx;
    Scope sc(ctx);

    LocalArray<Symbol> keys = sc.array<Symbol>(3, defer_init);
    {
        size_t index = 0;
        for (const auto& name : names) {
            keys[index].set(ctx.get_symbol(name));
            ++index;
        }
    }

    Local record = sc.local(Record::make(ctx, keys));
    check_keys(ctx, record, names);
}

TEST_CASE("Records should be constructible from a record template", "[record]") {
    static constexpr std::string_view names[] = {"foo", "bar", "baz"};

    Context ctx;
    Scope sc(ctx);

    Local keys = sc.local(Array::make(ctx, 0));
    Local key = sc.local();
    for (const auto& name : names) {
        key = ctx.get_symbol(name);
        keys->append(ctx, key);
    }

    Local tmpl = sc.local(RecordTemplate::make(ctx, keys));
    Local record = sc.local(Record::make(ctx, tmpl));
    check_keys(ctx, record, names);
}

TEST_CASE("Record elements can be read and written", "[record]") {
    Context ctx;
    Scope sc(ctx);

    Local foo = sc.local(ctx.get_symbol("foo"));
    Local bar = sc.local(ctx.get_symbol("bar"));

    LocalArray<Symbol> keys = sc.array<Symbol>(2, defer_init);
    keys[0].set(foo);
    keys[1].set(bar);

    Local record = sc.local(Record::make(ctx, keys));

    SECTION("Elements are null by default") {
        auto bar_value = record->get(*bar);
        REQUIRE(bar_value);
        REQUIRE(bar_value->is_null());
    }

    SECTION("Elements can be altered") {
        Local new_value = sc.local(String::make(ctx, "Hello World"));
        bool success = Record::set(ctx, record, foo, new_value);
        REQUIRE(success);

        auto foo_value = record->get(*foo);
        REQUIRE(foo_value);
        REQUIRE(foo_value->is<String>());
        REQUIRE(foo_value->must_cast<String>().view() == "Hello World");
    }

    SECTION("Reading non-existant elements fails") {
        Local sym = sc.local(ctx.get_symbol("sym"));
        auto sym_result = record->get(*sym);
        REQUIRE(!sym_result);
    }

    SECTION("Writing non-existant elements fails") {
        Local sym = sc.local(ctx.get_symbol("sym"));
        Local new_value = sc.local(SmallInteger::make(123));
        bool success = Record::set(ctx, record, sym, new_value);
        REQUIRE(!success);
    }
}

} // namespace tiro::vm::test
