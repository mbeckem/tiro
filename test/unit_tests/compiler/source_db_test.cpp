#include <catch2/catch.hpp>

#include "compiler/source_db.hpp"

namespace tiro::test {

TEST_CASE("default constructed absolute source ranges should be invalid", "[source-db]") {
    AbsoluteSourceRange range;
    REQUIRE(!range.valid());
    REQUIRE(!range);
    REQUIRE(range.range().empty());

    StringFormatStream stream;
    stream.format("{}", range);
    REQUIRE(stream.str() == "<invalid>");
}

TEST_CASE("absolute source ranges should represent file and position", "[source-db]") {
    AbsoluteSourceRange range(SourceId(123), SourceRange::from_offset(456));
    REQUIRE(range.valid());
    REQUIRE(range);
    REQUIRE(range.id() == SourceId(123));
    REQUIRE(range.range().begin() == 456);

    StringFormatStream stream;
    stream.format("{}", range);
    REQUIRE(stream.str() == "SourceId(123):456");
}

TEST_CASE("source db should store file contents", "[source-db]") {
    SourceDb db;

    auto id = db.insert_new("foo", "bar");
    REQUIRE(id);
    REQUIRE(db.contains("foo"));
    REQUIRE(db.filename(id) == "foo");
    REQUIRE(db.content(id) == "bar");
}

TEST_CASE("source db should be able to compute cursor positions", "[source-db]") {
    SourceDb db;
    auto id = db.insert_new("foo", "hello\nworld\n");
    REQUIRE(id);

    auto pos1 = db.cursor_pos(id, 6);
    REQUIRE(pos1.line() == 2);
    REQUIRE(pos1.column() == 1);

    auto [pos2, pos3] = db.cursor_pos(AbsoluteSourceRange(id, SourceRange(3, 7)));
    REQUIRE(pos2.line() == 1);
    REQUIRE(pos2.column() == 4);
    REQUIRE(pos3.line() == 2);
    REQUIRE(pos3.column() == 2);
}

} // namespace tiro::test
