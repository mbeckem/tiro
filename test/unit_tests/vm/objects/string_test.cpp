#include <catch2/catch.hpp>

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

    Local string = sc.local(builder->to_string(ctx));
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

    Local result = sc.local(builder->to_string(ctx));
    REQUIRE(expected == result->view());
}

TEST_CASE("Slicing a string should return a valid string slice", "[string]") {
    Context ctx;
    Scope sc(ctx);

    Local str = sc.local(String::make(ctx, "Hello World!"));

    auto require_slice = [&](StringSlice slice, size_t offset, size_t size,
                             std::string_view expected) {
        REQUIRE(slice.type() == ValueType::StringSlice);
        REQUIRE(slice.original().same(*str));
        REQUIRE(slice.offset() == offset);
        REQUIRE(slice.size() == size);
        REQUIRE(slice.view() == expected);
    };

    Local suffix = sc.local(str->slice_last(ctx, 6));
    require_slice(*suffix, 6, 6, "World!");

    Local suffix_2 = sc.local(suffix->slice(ctx, 1, 4));
    require_slice(*suffix_2, 7, 4, "orld");

    Local prefix = sc.local(str->slice_first(ctx, 5));
    require_slice(*prefix, 0, 5, "Hello");

    Local prefix_2 = sc.local(prefix->slice(ctx, 1, 3));
    require_slice(*prefix_2, 1, 3, "ell");

    Local middle = sc.local(str->slice(ctx, 3, 2));
    require_slice(*middle, 3, 2, "lo");
}

// Undecided. Runtime error might be the better behaviour.
TEST_CASE("Slicing out of bounds truncates at max size", "[string]") {
    Context ctx;
    Scope sc(ctx);

    Local str = sc.local(String::make(ctx, "Hello World"));

    auto require_slice = [&](StringSlice slice, std::string_view expected) {
        REQUIRE(slice.original().same(*str));
        REQUIRE(slice.view() == expected);
    };

    require_slice(str->slice(ctx, 1, 99), "ello World");
    require_slice(str->slice_first(ctx, 99), "Hello World");
    require_slice(str->slice_last(ctx, 99), "Hello World");

    Local slice = sc.local(str->slice(ctx, 6, 5));
    require_slice(slice->slice(ctx, 1, 99), "orld");
    require_slice(slice->slice_first(ctx, 99), "World");
    require_slice(slice->slice_last(ctx, 99), "World");
}

TEST_CASE("Context should be able to intern strings", "[string]") {
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
