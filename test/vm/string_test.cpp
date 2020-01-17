#include <catch.hpp>

#include "tiro/vm/context.hpp"
#include "tiro/vm/objects/strings.hpp"

#include "tiro/vm/objects/strings.ipp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Strings should be constructible", "[string]") {
    Context ctx;

    Root<String> str1(ctx), str2(ctx), str3(ctx);

    str1.set(String::make(ctx, "hello"));
    REQUIRE(str1->view() == "hello");

    str2.set(String::make(ctx, "hello"));
    REQUIRE(str2->view() == "hello");
    REQUIRE(str2->size() == 5);
    REQUIRE(std::memcmp(str2->data(), "hello", 5) == 0);

    REQUIRE(str1->hash() == str2->hash());
    REQUIRE(str1->equal(str2.get()));

    str3.set(String::make(ctx, ""));
    REQUIRE(str3->view() == "");
    REQUIRE(!str1->equal(str3.get()));

    REQUIRE(!str1->same(str2.get()));
    REQUIRE(!str1->same(str3.get()));
    REQUIRE(!str2->same(str3.get()));
}

TEST_CASE("Strings should maintain their flags without modifying their hash",
    "[string]") {
    Context ctx;

    Root<String> s1(ctx);

    s1.set(String::make(ctx, "Hello World"));
    REQUIRE(!s1->interned());

    s1->interned(true);
    REQUIRE(s1->interned());

    size_t hash = s1->hash();
    REQUIRE(hash != 0);
    REQUIRE((hash & String::interned_flag) == 0);
    REQUIRE(s1->interned());

    s1->interned(false);
    REQUIRE(!s1->interned());
    REQUIRE(s1->hash() == hash);
}

TEST_CASE("String builder should be able to concat strings", "[string]") {
    Context ctx;

    Root<StringBuilder> builder(ctx, StringBuilder::make(ctx));
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

    Root<String> string(ctx, builder->make_string(ctx));
    REQUIRE(string->view() == "Hello World!");

    builder->clear();
    REQUIRE(builder->size() == 0);
    REQUIRE(builder->capacity() == 64);
}

TEST_CASE(
    "String builder should support formatting with large input", "[string]") {
    Context ctx;
    Root<StringBuilder> builder(ctx, StringBuilder::make(ctx));

    fmt::memory_buffer expected_buffer;
    for (size_t i = 0; i < 10000; ++i) {
        fmt::format_to(expected_buffer, "{} {} ", i, i * 2);
        builder->format(ctx, "{} {} ", i, i * 2);
    }

    std::string expected = to_string(expected_buffer);
    REQUIRE(expected == builder->view());
    REQUIRE(builder->capacity() == ceil_pow2(expected.size()));

    Root<String> result(ctx, builder->make_string(ctx));
    REQUIRE(expected == result->view());
}
