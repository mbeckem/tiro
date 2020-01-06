#include <catch.hpp>

#include "hammer/compiler/source_map.hpp"

using namespace hammer;
using namespace compiler;

TEST_CASE(
    "SourceMap should return the correct cursor position for a byte offset",
    "[source-map]") {
    struct Test {
        // 0 based
        size_t byte_offset;

        // 1 based
        u32 expected_line;
        u32 expected_column;
    };

    StringTable strings;
    InternedString filename = strings.insert("Test.file");
    std::string_view source = "Hello\nWorld\n\n!123";
    SourceMap map(filename, source);

    Test tests[] = {
        {0, 1, 1},  // H
        {1, 1, 2},  // e
        {5, 1, 6},  // 1. \n
        {6, 2, 1},  // W
        {11, 2, 6}, // 2. \n
        {16, 4, 4}, // "3"
    };

    size_t index = 0;
    for (const auto& test : tests) {
        CAPTURE(
            index, test.byte_offset, test.expected_line, test.expected_column);

        CursorPosition pos = map.cursor_pos(
            SourceReference(filename, test.byte_offset, test.byte_offset + 1));
        REQUIRE(pos);
        REQUIRE(pos.line() == test.expected_line);
        REQUIRE(pos.column() == test.expected_column);

        ++index;
    }
}
