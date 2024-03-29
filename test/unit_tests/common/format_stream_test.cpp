#include <catch2/catch.hpp>

#include "common/format.hpp"

namespace tiro::test {

namespace {

struct TypeWithMemberFormat {
    int x;
    int y;

    void format(FormatStream& stream) const { stream.format("memberformat{{{}, {}}}", x, y); }
};

struct TypeWithFreeFormat {
    int x;
    int y;
};

void format(const TypeWithFreeFormat& obj, FormatStream& stream) {
    stream.format("freeformat{{{}, {}}}", obj.x, obj.y);
}

struct TypeWithToString {};

std::string_view to_string(const TypeWithToString&) {
    return "tostring";
}

} // namespace

TEST_CASE("Format stream should support custom types", "[format-stream]") {
    std::string message;
    OutputIteratorStream stream(std::back_inserter(message));

    SECTION("member format") {
        stream.format("1: {}", TypeWithMemberFormat{1, 2});
        REQUIRE(message == "1: memberformat{1, 2}");
    }

    SECTION("free format") {
        stream.format("2: {}", TypeWithFreeFormat{1, 2});
        REQUIRE(message == "2: freeformat{1, 2}");
    }

    SECTION("free to_string") {
        stream.format("3: {}", TypeWithToString{});
        REQUIRE(message == "3: tostring");
    }
}

TEST_CASE("Indent stream should indent output properly", "[format-stream]") {
    std::string message;
    OutputIteratorStream base(std::back_inserter(message));
    IndentStream stream(base, 2);

    stream.format("Hello\nWorld");
    stream.format("!");
    stream.format("\n\nEOF\n");

    std::string_view expected =
        "  Hello\n"
        "  World!\n"
        "  \n"
        "  EOF\n";
    REQUIRE(message == expected);
}

TEST_CASE("StringFormatStream formats into an std::string", "[format-stream]") {
    StringFormatStream stream;
    stream.format("Hello {}!", "world");
    REQUIRE(stream.str() == "Hello world!");
}

} // namespace tiro::test

TIRO_ENABLE_MEMBER_FORMAT(tiro::test::TypeWithMemberFormat)
TIRO_ENABLE_FREE_FORMAT(tiro::test::TypeWithFreeFormat)
TIRO_ENABLE_FREE_TO_STRING(tiro::test::TypeWithToString)
