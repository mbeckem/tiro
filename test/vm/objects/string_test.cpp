#include <catch.hpp>

#include "vm/context.hpp"
#include "vm/objects/string.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Strings should be constructible", "[string]") {
    Context ctx;
    Scope sc(ctx);

    Local str1 = sc.local(Nullable<String>());
    Local str2 = sc.local(Nullable<String>());
    Local str3 = sc.local(Nullable<String>());

    str1.set(String::make(ctx, "hello"));
    REQUIRE(str1->value().view() == "hello");

    str2.set(String::make(ctx, "hello"));
    REQUIRE(str2->value().view() == "hello");
    REQUIRE(str2->value().size() == 5);
    REQUIRE(std::memcmp(str2->value().data(), "hello", 5) == 0);

    REQUIRE(str1->value().hash() == str2->value().hash());
    REQUIRE(str1->value().equal(str2->value()));

    str3.set(String::make(ctx, ""));
    REQUIRE(str3->value().view() == "");
    REQUIRE(!str1->value().equal(str3->value()));

    REQUIRE(!str1->same(*str2));
    REQUIRE(!str1->same(*str3));
    REQUIRE(!str2->same(*str3));
}

TEST_CASE("Strings should maintain their flags without modifying their hash", "[string]") {
    Context ctx;
    Scope sc(ctx);

    Local s1 = sc.local(Nullable<String>());

    s1.set(String::make(ctx, "Hello World"));
    REQUIRE(!s1->value().interned());

    s1->value().interned(true);
    REQUIRE(s1->value().interned());

    size_t hash = s1->value().hash();
    REQUIRE(hash != 0);
    REQUIRE((hash & String::interned_flag) == 0);
    REQUIRE(s1->value().interned());

    s1->value().interned(false);
    REQUIRE(!s1->value().interned());
    REQUIRE(s1->value().hash() == hash);
}

TEST_CASE("String builder should be able to concat strings", "[string]") {
    Context ctx;
    Scope sc(ctx);

    Local builder = sc.local(StringBuilder::make(ctx));
    REQUIRE(builder->size() == 0);
    REQUIRE(builder->capacity() == 0);
    REQUIRE(builder->data() == nullptr);
    REQUIRE(builder->view() == "");

    builder->append(ctx, "Hello");
    REQUIRE(builder->size() == 5);
    REQUIRE(builder->view() == "Hello");

    builder->append(ctx, " World!");
    REQUIRE(builder->view() == "Hello World!");
    REQUIRE(builder->size() == 12);
    REQUIRE(builder->capacity() == 64);

    Local string = sc.local(builder->make_string(ctx));
    REQUIRE(string->view() == "Hello World!");

    builder->clear();
    REQUIRE(builder->size() == 0);
    REQUIRE(builder->capacity() == 64);
}

TEST_CASE("String builder should support formatting with large input", "[string]") {
    Context ctx;
    Scope sc(ctx);
    Local builder = sc.local(StringBuilder::make(ctx));

    fmt::memory_buffer expected_buffer;
    for (size_t i = 0; i < 10000; ++i) {
        fmt::format_to(expected_buffer, "{} {} ", i, i * 2);
        builder->format(ctx, "{} {} ", i, i * 2);
    }

    std::string expected = to_string(expected_buffer);
    REQUIRE(expected == builder->view());
    REQUIRE(builder->capacity() == ceil_pow2(expected.size()));

    Local result = sc.local(builder->make_string(ctx));
    REQUIRE(expected == result->view());
}

TEST_CASE("Context should be able to intern strings", "[context]") {
    Context ctx;
    Scope sc(ctx);

    Local s1 = sc.local(String::make(ctx, "Hello World"));
    Local s2 = sc.local(String::make(ctx, "Hello World"));
    Local s3 = sc.local(String::make(ctx, "Foobar"));

    Local c = sc.local(Nullable<String>());

    c = ctx.get_interned_string(s1);
    REQUIRE(c->same(*s1));
    REQUIRE(c->value().interned());

    c = ctx.get_interned_string(s1);
    REQUIRE(c->same(*s1));

    c = ctx.get_interned_string(s2);
    REQUIRE(c->same(*s1));
    REQUIRE(s1->interned());
    REQUIRE(!s2->interned());

    c = ctx.get_interned_string(s3);
    REQUIRE(c->same(*s3));
}
