#include <catch2/catch.hpp>

#include "compiler/source_map.hpp"

using namespace tiro;

TEST_CASE("SourceMap should return the correct cursor position for a byte offset", "[source-map]") {
    struct Test {
        // 0 based
        size_t byte_offset;

        // 1 based
        u32 expected_line;
        u32 expected_column;
    };

    StringTable strings;
    InternedString filename = strings.insert("Test.file");
    std::string_view source =
        "Hello\n"   // 6 byte
        "World\n\n" // 7 byte
        "世界!123"; // 10 byte, 世 and 界 3 bytes each
    SourceMap map(filename, source);

    Test tests[] = {
        {0, 1, 1},  // H
        {1, 1, 2},  // e
        {5, 1, 6},  // 1. \n
        {6, 2, 1},  // W
        {11, 2, 6}, // 2. \n
        {22, 4, 6}, // "3"
    };

    size_t index = 0;
    for (const auto& test : tests) {
        CAPTURE(index, test.byte_offset, test.expected_line, test.expected_column);

        CursorPosition pos = map.cursor_pos(test.byte_offset);
        REQUIRE(pos);
        REQUIRE(pos.line() == test.expected_line);
        REQUIRE(pos.column() == test.expected_column);

        ++index;
    }
}
